#ifndef LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
#define LIB_DATASTRUCTURE_PRIORITYQUEUE_H_

#include "../../external/BinaryHeap.hpp"

#include "Hypergraph.h"
#include "../macros.h"

using utility::datastructure::BinaryHeap;

typedef hgr::HypergraphType::HypernodeID HypernodeID;

// ToDo: We need a more robust solution for min and max values!
struct MetaKey {
  static double max() {
    return 12345.6789;
  }
  static double min() {
    return -12345.6789;
  }
};

template <typename KeyType>
class PriorityQueue{
  typedef BinaryHeap<HypernodeID, KeyType, MetaKey> Heap;
  
 public:
  PriorityQueue(HypernodeID size, size_t reserve_size) :
      _heap(size, reserve_size) { }

  size_t size() const {
    return _heap.size();
  }

  bool empty() const {
    return size() == 0;
  }

  void insert(HypernodeID hn, KeyType key) {
    _heap.push(hn, -key);
  }

  void deleteMax() {
    _heap.deleteMin();
  }

  HypernodeID max() const {
    return _heap.getMin();
  }

  KeyType maxKey() const {
    return -_heap.getMinKey();
  }

  void update(HypernodeID hn, KeyType key) {
    _heap.updateKey(hn, -key);
  }

  void remove(HypernodeID hn) {
    _heap.deleteNode(hn);
  }
  
 private:
  Heap _heap;
  DISALLOW_COPY_AND_ASSIGN(PriorityQueue);
};

#endif  // LIB_DATASTRUCTURE_PRIORITYQUEUE_H_
