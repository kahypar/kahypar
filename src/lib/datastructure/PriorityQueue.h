/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
#define SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_

#include "lib/core/Mandatory.h"
#include "lib/macros.h"

namespace datastructure {
template <class BinaryHeap>
class PriorityQueue {
 private:
  using IDType = typename BinaryHeap::value_type;
  using KeyType = typename BinaryHeap::key_type;
  using DataType = typename BinaryHeap::data_type;

 public:
  explicit PriorityQueue(IDType size) noexcept :
    _heap(size) { }

  PriorityQueue(const PriorityQueue&) = delete;
  PriorityQueue& operator= (const PriorityQueue&) = delete;

  PriorityQueue(PriorityQueue&&) = default;
  PriorityQueue& operator= (PriorityQueue&&) = delete;

  size_t size() const noexcept {
    return _heap.size();
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  void insert(const IDType id, const KeyType key) noexcept {
    _heap.push(id, key);
  }

  void deleteMax() noexcept {
    _heap.deleteMax();
  }

  IDType max() const noexcept {
    return _heap.getMax();
  }

  KeyType maxKey() const noexcept {
    return _heap.getMaxKey();
  }

  KeyType key(const IDType id) const noexcept {
    return _heap.getKey(id);
  }

  bool contains(const IDType id) const noexcept {
    return _heap.contains(id);
  }

  void update(const IDType id, const KeyType key) noexcept {
    _heap.update(id, key);
  }

  void updateKey(const IDType id, const KeyType key) noexcept {
    _heap.updateKey(id, key);
  }

  void increaseKey(const IDType id, const KeyType key) noexcept {
    _heap.increaseKey(id, key);
  }

  void decreaseKey(const IDType id, const KeyType key) noexcept {
    _heap.decreaseKey(id, key);
  }

  void remove(const IDType id) noexcept {
    _heap.deleteNode(id);
  }

  template <typename T = DataType>
  typename std::enable_if<std::is_same<DataType, T>::value, T&>::type data(IDType id) noexcept {
    return _heap.getUserData(id);
  }

  void clear() noexcept {
    _heap.clear();
  }

 private:
  BinaryHeap _heap;
};
}  // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
