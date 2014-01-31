#ifndef LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
#define LIB_DATASTRUCTURE_PRIORITYQUEUE_H_

#include "external/BinaryHeap.hpp"
#include "lib/macros.h"

namespace datastructure {
using external::BinaryHeap;

// ToDo: We need a more robust solution for min and max values!
struct MetaKeyDouble {
  static double max() {
    return 12345.6789;
  }
  static double min() {
    return -12345.6789;
  }
};

template <typename IDType, typename KeyType, typename MetaKey>
class PriorityQueue {
  typedef BinaryHeap<IDType, KeyType, MetaKey> Heap;

  public:
  PriorityQueue(IDType size, size_t reserve_size = 0) :
    _heap(size, reserve_size) { }

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

  void clear() {
    _heap.clear();
  }

  private:
  Heap _heap;
  DISALLOW_COPY_AND_ASSIGN(PriorityQueue);
};
} // namespace datastructure

#endif  // LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
