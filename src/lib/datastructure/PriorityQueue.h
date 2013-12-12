#ifndef LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
#define LIB_DATASTRUCTURE_PRIORITYQUEUE_H_

#include "../../external/BinaryHeap.hpp"

#include "Hypergraph.h"
#include "../macros.h"

namespace datastructure {
using external::BinaryHeap;

// ToDo: We need a more robust solution for min and max values!
struct MetaKey {
  static double max() {
    return 12345.6789;
  }
  static double min() {
    return -12345.6789;
  }
};

template <typename IDType, typename KeyType>
class PriorityQueue{
  typedef BinaryHeap<IDType, KeyType, MetaKey> Heap;
  
 public:
  PriorityQueue(IDType size, size_t reserve_size) :
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

  void deleteMax() {
    _heap.deleteMin();
  }

  IDType max() const {
    return _heap.getMin();
  }

  KeyType maxKey() const {
    return -_heap.getMinKey();
  }

  bool contains(IDType id) const {
    return _heap.contains(id);
  }

  void update(IDType id, KeyType key) {
    _heap.updateKey(id, -key);
  }

  void remove(IDType id) {
    _heap.deleteNode(id);
  }
  
 private:
  Heap _heap;
  DISALLOW_COPY_AND_ASSIGN(PriorityQueue);
};

} // namespace datastructure

#endif  // LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
