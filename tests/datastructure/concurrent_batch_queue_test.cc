/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <iostream>

#include "gmock/gmock.h"

#include "kahypar/datastructure/concurrent_batch_queue.h"
#include "kahypar/utils/thread_pool.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
namespace ds {

class AConcurrentBatchQueue : public ::testing::TestWithParam<size_t> {
 private:
  using ThreadPool = kahypar::parallel::ThreadPool;

 public:
  AConcurrentBatchQueue() :
    context(),
    queue(GetParam(), 5 /* batch size */),
    pool(nullptr),
    current_thread_id(0) { 
    context.shared_memory.num_threads = GetParam();
    context.shared_memory.working_packages = GetParam();
    pool = std::make_unique<ThreadPool>(context);
  }

  void concurrently_push(size_t num_values) {
    pool->parallel_for([this](const size_t start, const size_t end) {
      size_t thread_id = current_thread_id++;
      while ( current_thread_id < GetParam() ) { }
      for ( size_t i = start; i < end; ++i ) {
        this->queue.push(thread_id, i);
      }
    }, 0UL, num_values);
    pool->loop_until_empty();
  }

  void verify_content(size_t num_values) {
    ASSERT_THAT(queue.unsafe_size(), Eq(num_values));
    std::vector<bool> contains(num_values, false);
    size_t size = 0;
    while(!queue.empty()) {
      std::vector<size_t> batch = queue.pop_batch();
      for ( size_t value : batch ) {
        size++;
        ASSERT_THAT(contains[value], Eq(false));
        contains[value] = true;
      }
    }
    ASSERT_THAT(size, Eq(num_values));
  }


  Context context;
  ConcurrentBatchQueue<size_t> queue;
  std::unique_ptr<ThreadPool> pool;
  std::atomic<size_t> current_thread_id;
};


INSTANTIATE_TEST_CASE_P(NumThreads,
                        AConcurrentBatchQueue,
                        ::testing::Values(1, 2, 4, 8));

TEST_P(AConcurrentBatchQueue, ConcurrentlyPushIntoQueue) {
  concurrently_push(50);
  verify_content(50);
}

TEST_P(AConcurrentBatchQueue, ConcurrentlyPopBatchFromQueue) {
  concurrently_push(50);

  std::vector<std::future<size_t>> results;
  for ( size_t thread_id = 0; thread_id < GetParam(); ++thread_id ) {
    results.emplace_back(pool->enqueue([this]() {
      current_thread_id++;
      while ( current_thread_id < GetParam() ) { }

      size_t size = 0;
      while ( !this->queue.empty() ) {
        std::vector<size_t> batch = this->queue.pop_batch();
        size += batch.size();
      }
      return size;
    }));
  }
  pool->loop_until_empty();

  size_t size = 0;
  for ( auto& fut : results ) {
    size += fut.get();
  }
  ASSERT_THAT(size, Eq(50));
}

TEST_P(AConcurrentBatchQueue, ConcurrentlyPushAndPopBatchFromQueue) {
  concurrently_push(50);

  current_thread_id = 0;
  std::vector<std::future<std::pair<size_t, size_t>>> results;
  for ( size_t thread_id = 0; thread_id < GetParam(); ++thread_id ) {
    results.emplace_back(pool->enqueue([this]() {
      size_t thread_id = current_thread_id++;
      while ( current_thread_id < GetParam() ) { }

      size_t size_push = 0;
      size_t size_pop = 0;
      for ( size_t i = 0; i < 100; ++i ) {
        bool push = Randomize::instance().flipCoin();
        if ( push ) {
          for ( size_t i = 0; i < 10; ++i ) {
            this->queue.push(thread_id, i);
            size_push++;
          }
        } else {
          std::vector<size_t> batch = this->queue.pop_batch();
          size_pop += batch.size();
        }
      }
      return std::make_pair(size_push, size_pop);
    }));
  }
  pool->loop_until_empty();

  size_t size_push = 50;
  size_t size_pop = 0;
  for ( auto& fut : results ) {
    auto res = fut.get();
    size_push += res.first;
    size_pop += res.second;
  }

  ASSERT_THAT(size_pop + queue.unsafe_size(), Eq(size_push));
}


} // namespace ds
} // namespace kahypar