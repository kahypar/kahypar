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

#pragma once

#include <algorithm>
#include <limits>
#include <queue>
#include <atomic>
#include <shared_mutex>

#include "kahypar/macros.h"

namespace kahypar {
namespace ds {
template < typename Value >
class ConcurrentBatchQueue {
 private:
  using Mutex = std::shared_timed_mutex;

 public:
  explicit ConcurrentBatchQueue(const size_t num_threads,
                                const size_t batch_size) :
    _num_threads(num_threads),
    _batch_size(batch_size),
    _round(0),
    _size(0),
    _current_batch_size(0),
    _thread_queue(num_threads, std::vector<Value>()),
    _batches_mutex(),
    _batches(),
    _tmp_batches_mutex(),
    _tmp_batches() { }

  ConcurrentBatchQueue(const ConcurrentBatchQueue&) = delete;
  ConcurrentBatchQueue& operator= (const ConcurrentBatchQueue&) = delete;

  ConcurrentBatchQueue(ConcurrentBatchQueue&& other) :
    _num_threads(std::move(other._num_threads)),
    _batch_size(std::move(other._batch_size)),
    _round(std::move(other._round)),
    _size(other._size.load()),
    _current_batch_size(other._current_batch_size.load()),
    _thread_queue(std::move(other._thread_queue)),
    _batches_mutex(),
    _batches(std::move(other._batches)),
    _tmp_batches_mutex(),
    _tmp_batches(std::move(other._tmp_batches)) {}

  ConcurrentBatchQueue& operator= (ConcurrentBatchQueue&&) = delete;

  ~ConcurrentBatchQueue() = default;

  void push(const size_t thread_id, const Value& value) {
    ASSERT(thread_id < _num_threads);
    std::shared_lock<Mutex> lock(_tmp_batches_mutex);
    _thread_queue[thread_id].emplace_back(value);
    ++_current_batch_size;
    ++_size;

    // In that case number of values in local queues is
    // greater than the batch size, which will trigger the
    // batch enqueue operation that will insert a batch
    // into a temporary queue.
    if ( _current_batch_size > _batch_size ) {
      lock.unlock();
      std::unique_lock<Mutex> tmp_lock(_tmp_batches_mutex);
      enqueue_batch();
    }
  }

  std::vector<Value> pop_batch() {
    std::unique_lock<Mutex> lock(_batches_mutex);
    if ( !_batches.empty() ) {
      // If there are still batches left just return it
      std::vector<Value> batch = std::move(_batches.front());
      _batches.pop();
      _size -= batch.size();
      return batch;
    } 
    
    std::unique_lock<Mutex> tmp_lock(_tmp_batches_mutex);
    // In case _batches is empty we are forcing to enqueue
    // all values and swap the temporary queue with the
    // global queue (_batches).
    if ( _tmp_batches.size() == 0 ) {
      enqueue_batch(true);
    }
    std::swap(_batches, _tmp_batches);
    _tmp_batches = std::queue<std::vector<Value>>();
    ++_round;

    if ( !_batches.empty() ) {
      std::vector<Value> batch = std::move(_batches.front());
      _batches.pop();
      _size -= batch.size();
      return batch; 
    } else {
      // If batches is still empty return empty vector
      return std::vector<Value>();
    }
  }

  void update_batch_size(const size_t batch_size) {
    _batch_size = batch_size;
  }

  size_t round() const {
    return _round;
  }

  bool empty() const {
    return _size == 0;
  }

  size_t unsafe_size() const {
    return _size;
  }

 private:

  void enqueue_batch(bool force = false) {
    if ( force || _current_batch_size > _batch_size ) {
      std::vector<Value> new_batch;
      for ( size_t thread_id = 0; thread_id < _num_threads; ++thread_id ) {
        new_batch.insert(new_batch.end(), _thread_queue[thread_id].begin(), _thread_queue[thread_id].end());
        _thread_queue[thread_id].clear();
      }

      if ( new_batch.size() > 0 ) {
        _tmp_batches.emplace(std::move(new_batch));
        _current_batch_size = 0;
      }
    }
  }

  const size_t _num_threads;
  size_t _batch_size;
  size_t _round;

  // Each thread contains its own batch queues
  // Once the batch size threshold is reached all
  // hypernodes from all batch queues are inserted 
  // into _tmp_batches
  std::atomic<size_t> _size;
  std::atomic<Value> _current_batch_size;
  std::vector<std::vector<Value>> _thread_queue;

  // Batch Queue - Contains values for pop batch
  Mutex _batches_mutex;
  std::queue<std::vector<Value>> _batches;

  // Batch Queue - Contains values inserted by push
  // If _batches becomes empty and someone calls pop_batch,
  // than _tmp_batches and _batches are swapped
  Mutex _tmp_batches_mutex;
  std::queue<std::vector<Value>> _tmp_batches;
};

}  // namespace ds
}  // namespace kahypar
