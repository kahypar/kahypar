/*******************************************************************************
 * tlx/thread_pool.hpp
 *
 * Part of tlx - http://panthema.net/tlx
 *
 * Copyright (C) 2015 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the Boost Software License, Version 1.0
 ******************************************************************************/

#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
#include <sched.h>

#include "kahypar/partition/context.h"

namespace kahypar {
namespace parallel {

/*!
 * ThreadPool starts a fixed number p of std::threads which process Jobs that
 * are \ref enqueue "enqueued" into a concurrent job queue. The jobs
 * themselves can enqueue more jobs that will be processed when a thread is
 * ready.
 *
 * The ThreadPool can either run until
 *
 * 1. all jobs are done AND all threads are idle, when called with
 * loop_until_empty(), or
 *
 * 2. until Terminate() is called when run with loop_until_terminate().
 *
 * Jobs are plain tlx::Delegate<void()> objects, hence the pool user must pass
 * in ALL CONTEXT himself. The best method to pass parameters to Jobs is to use
 * lambda captures. Alternatively, old-school objects implementing operator(),
 * or std::binds can be used.
 *
 * The ThreadPool uses a condition variable to wait for new jobs and does not
 * remain busy waiting.
 *
 * Note that the threads in the pool start **before** the two loop functions are
 * called. In case of loop_until_empty() the threads continue to be idle
 * afterwards, and can be reused, until the ThreadPool is destroyed.

\code
ThreadPool pool(4); // pool with 4 threads

int value = 0;
pool.enqueue([&value]() {
  // increment value in another thread.
  ++value;
});

pool.loop_until_empty();
\endcode

 */
class ThreadPool
{
public:
    using Job = std::function<void ()>;

private:
    //! Deque of scheduled jobs.
    std::deque<Job> jobs_;

    //! Mutex used to access the queue of scheduled jobs.
    std::mutex mutex_;

    //! threads in pool
    std::vector<std::thread> threads_;

    //! Condition variable used to notify that a new job has been inserted in
    //! the queue.
    std::condition_variable cv_jobs_;
    //! Condition variable to signal when a jobs finishes.
    std::condition_variable cv_finished_;

    //! Counter for number of threads busy.
    std::atomic<size_t> busy_;
    //! Counter for number of idle threads waiting for a job.
    std::atomic<size_t> idle_;
    //! Counter for total number of jobs executed
    std::atomic<size_t> done_;

    //! Flag whether to terminate
    std::atomic<bool> terminate_;

public:
    //! Construct running thread pool of num_threads
    explicit ThreadPool(const kahypar::Context& context)
      : jobs_(),
        mutex_(),
        threads_(std::max(context.shared_memory.num_threads, static_cast<size_t>(1))),
        cv_jobs_(),
        cv_finished_(),
        busy_(0),
        idle_(0),
        done_(0),
        terminate_(false) {
      // immediately construct worker threads
      size_t hardware_threads = std::thread::hardware_concurrency();
      size_t num_threads = threads_.size();
      for (size_t i = 0; i < num_threads; ++i) {
        threads_[i] = std::thread(&ThreadPool::worker, this);
        // Set CPU affinity to exactly ONE CPU
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(i % hardware_threads, &cpu_set);
        pthread_setaffinity_np(threads_[i].native_handle(), sizeof(cpu_set), &cpu_set);
      }
    }

    ThreadPool(const ThreadPool&) = default;
    ThreadPool& operator = (const ThreadPool&) = default;

    //! Stop processing jobs, terminate threads.
    ~ThreadPool() {
      std::unique_lock<std::mutex> lock(mutex_);
      // set stop-condition
      terminate_ = true;
      cv_jobs_.notify_all();
      lock.unlock();

      // all threads terminate, then we're done
      for (size_t i = 0; i < threads_.size(); ++i) {
          threads_[i].join();
      }
    }

    //! enqueue a Job, pass parameters in capture
    template<class F>
    auto enqueue(F&& func)
      -> std::future<typename std::result_of<F()>::type> {
      return enqueue_job(std::forward<F>(func));
    }

    template<class F, class T>
    auto parallel_for(F&& func, const T& start, const T& end, const T& step_threshold = 1)
      -> std::vector<std::future<typename std::result_of<F(T, T)>::type>> {
      using return_type = typename std::result_of<F(T, T)>::type;

      T step = (end - start) / size();
      std::vector<std::future<return_type>> results;
      if ( step >= step_threshold && size() != 1 ) {
        for ( size_t i = 0; i < size(); ++i ) {
          T tmp_start = i * step;
          T tmp_end = (i == size() - 1) ? end : (i + 1) * step;
          results.push_back(enqueue_job(func, tmp_start, tmp_end));
        }
      } else {
        results.push_back(enqueue_job(func, start, end));
      }

      return results;
    }

    //! Loop until no more jobs are in the queue AND all threads are idle. When
    //! this occurs, this method exits, however, the threads remain active.
    void loop_until_empty() {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_finished_.wait(lock, [this]() { return jobs_.empty() && (busy_ == 0); });
      std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    //! Loop until terminate flag was set.
    void loop_until_terminate() {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_finished_.wait(lock, [this]() { return terminate_ && (busy_ == 0); });
      std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    //! Terminate thread pool gracefully, wait until currently running jobs
    //! finish and then exit. This should be called from within one of the
    //! enqueue jobs or from an outside thread.
    void terminate() {
      std::unique_lock<std::mutex> lock(mutex_);
      // flag termination
      terminate_ = true;
      // wake up all worker threads and let them terminate.
      cv_jobs_.notify_all();
      // notify LoopUntilTerminate in case all threads are idle.
      cv_finished_.notify_one();
    }

    //! Return number of jobs currently completed.
    size_t done() const {
      return done_;
    }

    //! Return number of threads in pool
    size_t size() const {
      return threads_.size();
    }

    //! return number of idle threads in pool
    size_t idle() const {
      return idle_;
    }

    //! true if any thread is idle (= waiting for jobs)
    bool has_idle() const {
      return (idle_.load(std::memory_order_relaxed) != 0);
    }

    //! Return thread handle to thread i
    std::thread& thread(size_t i) {
      assert(i < threads_.size());
      return threads_[i];
    }

private:
     //! enqueue a Job, pass parameters in capture
    template<class F, class... Args>
    auto enqueue_job(F&& func, Args&&... args)
      -> std::future<typename std::result_of<F(Args...)>::type> {
      using return_type = typename std::result_of<F(Args...)>::type;

      auto job = std::make_shared< std::packaged_task<return_type()> >(
        std::bind(std::forward<F>(func), std::forward<Args>(args)...)
      );

      std::future<return_type> res = job->get_future();
      {
        std::unique_lock<std::mutex> lock(mutex_);
        jobs_.emplace_back([job]() { (*job)(); });
      }
      cv_jobs_.notify_one();
      return res;
    }

    //! Worker function, one per thread is started.
    void worker() {
      // lock mutex, it is released during condition waits
      std::unique_lock<std::mutex> lock(mutex_);

      while (true) {
          // wait on condition variable until job arrives, frees lock
          if (!terminate_ && jobs_.empty()) {
              ++idle_;
              cv_jobs_.wait(
                  lock, [this]() { return terminate_ || !jobs_.empty(); });
              --idle_;
          }

          if (terminate_)
              break;

          if (!jobs_.empty()) {
              // got work. set busy.
              ++busy_;

              {
                  // pull job.
                  Job job = std::move(jobs_.front());
                  jobs_.pop_front();

                  // release lock.
                  lock.unlock();

                  // execute job.
                  try {
                      job();
                  }
                  catch (std::exception& e) {
                      std::cerr << "EXCEPTION: " << e.what() << std::endl;
                  }
                  // destroy job by closing scope
              }

              // release memory the Job changed
              std::atomic_thread_fence(std::memory_order_seq_cst);

              ++done_;
              --busy_;

              // relock mutex before signaling condition.
              lock.lock();
              cv_finished_.notify_one();
          }
      }
    }
};

} // namespace parallel
} // namespace kahypar