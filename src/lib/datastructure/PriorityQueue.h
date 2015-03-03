/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
#define SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_

#include "lib/core/Mandatory.h"
#include "lib/macros.h"

namespace datastructure {
// ToDo: We need a more robust solution for min and max values!
struct MetaKeyDouble {
  static double max() {
    return 12345.6789;
  }
  static double min() {
    return -12345.6789;
  }
};

template <class BinaryHeap>
class PriorityQueue {
  typedef typename BinaryHeap::value_type IDType;
  typedef typename BinaryHeap::key_type KeyType;
  typedef typename BinaryHeap::data_type DataType;

  public:
  PriorityQueue(const PriorityQueue&) = delete;
  PriorityQueue(PriorityQueue&&) = delete;
  PriorityQueue& operator = (const PriorityQueue&) = delete;
  PriorityQueue& operator = (PriorityQueue&&) = delete;

  explicit PriorityQueue(IDType size) :
    _heap(size) { }

  size_t size() const {
    return _heap.size();
  }

  bool empty() const {
    return size() == 0;
  }

  void insert(const IDType id, const KeyType key) {
    _heap.push(id, key);
  }

  void reInsert(const IDType id, const KeyType key) {
    _heap.reinsertingPush(id, key);
  }

  template <typename T>
  void reInsert(const IDType id, const KeyType key, T data,
                typename std::enable_if<std::is_same<T, DataType>::value>::type* = 0) {
    _heap.reinsertingPush(id, key, data);
  }

  void deleteMax() {
    _heap.deleteMax();
  }

  IDType max() const {
    return _heap.getMax();
  }

  KeyType maxKey() const {
    return _heap.getMaxKey();
  }

  KeyType key(const IDType id) const {
    return _heap.getKey(id);
  }

  bool contains(const IDType id) const {
    return _heap.contains(id);
  }

  void update(const IDType id, const KeyType key) {
    _heap.update(id, key);
  }

  void updateKey(const IDType id, const KeyType key) {
    _heap.updateKey(id, key);
  }

  void increaseKey(const IDType id, const KeyType key) {
    _heap.increaseKey(id, key);
  }

  void decreaseKey(const IDType id, const KeyType key) {
    _heap.decreaseKey(id, key);
  }

  void remove(const IDType id) {
    _heap.deleteNode(id);
  }

  template <typename T = DataType>
  typename std::enable_if<std::is_same<DataType, T>::value, T&>::type data(IDType id) {
    return _heap.getUserData(id);
  }

  void clear() {
    _heap.clear();
  }

  private:
  BinaryHeap _heap;
};
} // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
