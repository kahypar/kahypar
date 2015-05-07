/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_BUCKETQUEUE_H_
#define SRC_LIB_DATASTRUCTURE_BUCKETQUEUE_H_

#include <algorithm>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lib/core/Mandatory.h"
#include "lib/macros.h"

namespace datastructure {
template <typename id_slot = Mandatory,
          typename key_slot = Mandatory>
class BucketQueue {
 public:
  explicit BucketQueue(const key_slot& key_span) noexcept :
    _elements(0),
    _key_span(key_span),
    _max_idx(0),
    _queue_index(),
    _buckets(std::make_unique<std::vector<id_slot>[]>(2 * _key_span + 1)) { }

  explicit BucketQueue(const BucketQueue& other) noexcept :
    _elements(other._elements),
    _key_span(other._key_span),
    _max_idx(other._max_idx),
    _queue_index(),
    _buckets(std::make_unique<std::vector<id_slot>[]>(2 * _key_span + 1)) { }

  ~BucketQueue() { }

  id_slot size() const noexcept {
    return _elements;
  }

  bool empty() const noexcept {
    return _elements == 0;
  }

  void swap(BucketQueue& other) noexcept {
    using std::swap;
    swap(_elements, other._elements);
    swap(_key_span, other._key_span);
    swap(_max_idx, other._max_idx);
    swap(_queue_index, other._queue_index);
    swap(_buckets, other._buckets);
  }

  key_slot getKey(const id_slot element) const noexcept {
    ASSERT(_queue_index.find(element) != _queue_index.end(),
           " Element " << element << " not contained in PQ");
    return _queue_index.find(element)->second.second;
  }

  void push(const id_slot id, const key_slot key) noexcept {
    const key_slot address = key + _key_span;
    if (address > _max_idx) {
      _max_idx = address;
    }

    _buckets[address].push_back(id);
    _queue_index[id].first = _buckets[address].size() - 1;  //store position
    _queue_index[id].second = key;

    _elements++;
  }

  //  only to temporarily satisfy PQ interface
  void reinsertingPush(const id_slot id, const key_slot key) noexcept  {
    push(id, key);
  }

  void clear() noexcept {
    for (key_slot i = 0; i < 2 * _key_span + 1; ++i) {
      _buckets[i].clear();
    }
    _elements = 0;
    _max_idx = 0;
    _queue_index.clear();
  }

  key_slot getMaxKey() const noexcept {
    ASSERT(!empty(), "BucketQueue is empty");
    //   DBG(true, "---->" << _queue_index[_buckets[_max_idx].back()].second);
    return _max_idx - _key_span;
  }

  id_slot getMax() const noexcept {
    ASSERT(!_buckets[_max_idx].empty(),
           "max-Bucket " << _max_idx << " is empty");
    return _buckets[_max_idx].back();
  }

  void deleteMax() noexcept {
    ASSERT(!_buckets[_max_idx].empty(),
           "max-Bucket " << _max_idx << " is empty");
    _queue_index.erase(_buckets[_max_idx].back());
    _buckets[_max_idx].pop_back();

    if (_buckets[_max_idx].size() == 0) {
      searchNewMax();
    }

    --_elements;
  }

  void decreaseKey(const id_slot id, const key_slot newkey_slot) noexcept {
    updateKey(id, newkey_slot);
  }
  void increaseKey(const id_slot id, const key_slot newkey_slot) noexcept {
    updateKey(id, newkey_slot);
  }

  void updateKey(const id_slot id, const key_slot new_key) noexcept {
    deleteNode(id);
    push(id, new_key);
  }

  void deleteNode(const id_slot id) noexcept {
    ASSERT(_queue_index.find(id) != _queue_index.end(),
           "Hyperid " << id << " not in PQ");
    size_t in_bucket_idx, old_key;
    std::tie(in_bucket_idx, old_key) = _queue_index[id];
    const key_slot address = old_key + _key_span;

    if (_buckets[address].size() > 1) {
      //swap current element with last element and pop_back
      _queue_index[_buckets[address].back()].first = in_bucket_idx; // update helper structure
      std::swap(_buckets[address][in_bucket_idx], _buckets[address].back());
      _buckets[address].pop_back();
    } else {
      //size is 1
      _buckets[address].pop_back();
      if (address == _max_idx) {
        searchNewMax();
      }
    }

    --_elements;
    _queue_index.erase(id);
  }

  bool contains(const id_slot id) const noexcept {
    return _queue_index.find(id) != _queue_index.end();
  }

 private:
  void searchNewMax() noexcept {
    while (_max_idx != 0 && _buckets[_max_idx].empty()) {
      --_max_idx;
    }
  }


  id_slot _elements;
  key_slot _key_span;
  key_slot _max_idx; //points to the non-empty bucket with the largest key

  std::unordered_map<id_slot, std::pair<size_t, key_slot> > _queue_index;
  std::unique_ptr<std::vector<id_slot>[]> _buckets;
};

template <typename id_slot, typename key_slot>
void swap(BucketQueue<id_slot, key_slot>& a,
          BucketQueue<id_slot, key_slot>& b) noexcept {
  a.swap(b);
}
} // namespace datastructure
#endif  // SRC_LIB_DATASTRUCTURE_BUCKETQUEUE_H_
