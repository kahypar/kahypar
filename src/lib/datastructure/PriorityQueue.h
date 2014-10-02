/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
#define SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_

#include "external/binary_heap/BinaryHeap.hpp"
#include "external/binary_heap/QueueStorages.hpp"
#include "lib/core/Mandatory.h"
#include "lib/macros.h"

using external::BinaryHeap;
using external::NullData;
using external::ArrayStorage;

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

template <typename IDType = Mandatory,
          typename KeyType = Mandatory,
          typename MetaKey = Mandatory,
          typename DataType = NullData,
          typename Storage = ArrayStorage<IDType> >
class PriorityQueue {
  typedef BinaryHeap<IDType, KeyType, MetaKey, DataType, Storage> Heap;

  public:
  explicit PriorityQueue(IDType size) :
    _heap(size) { }

  size_t size() const {
    return _heap.size();
  }

  bool empty() const {
    return size() == 0;
  }

  void insert(IDType id, KeyType key) {
    _heap.push(id, -key);
  }

  void reInsert(IDType id, KeyType key) {
    _heap.reinsertingPush(id, -key);
  }

  void reInsert(IDType id, KeyType key, DataType data) {
    _heap.reinsertingPush(id, -key, data);
  }

  void deleteMax() {
    _heap.deleteMin();
  }

  IDType max() const {
    return _heap.getMin();
  }

  KeyType maxKey() const {
    return -_heap.getMinKey();
  }

  KeyType key(IDType id) const {
    return -_heap.getKey(id);
  }

  bool contains(IDType id) const {
    return _heap.contains(id);
  }

  void update(IDType id, KeyType key) {
    _heap.update(id, -key);
  }

  void updateKey(IDType id, KeyType key) {
    _heap.updateKey(id, -key);
  }

  void increaseKey(IDType id, KeyType key) {
    _heap.increaseKey(id, -key);
  }

  void decreaseKey(IDType id, KeyType key) {
    _heap.decreaseKey(id, -key);
  }

  void remove(IDType id) {
    _heap.deleteNode(id);
  }

  DataType & data(IDType id) {
    return _heap.getUserData(id);
  }

  void clear() {
    _heap.clear();
  }

  private:
  Heap _heap;
  DISALLOW_COPY_AND_ASSIGN(PriorityQueue);
};
} // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
