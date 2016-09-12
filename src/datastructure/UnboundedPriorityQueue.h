/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <sparsehash/dense_hash_map>

#include <limits>

#include "external/binary_heap/UnboundedBinaryHeap.h"
#include "meta/Mandatory.h"
#include "macros.h"

using external::UnboundedBinaryHeap;
using external::NullData;
using external::ArrayStorage;

namespace datastructure {
template <typename KeyType = Mandatory,
          typename MetaKey = Mandatory,
          typename ElementType = NullData>
class UnboundedPriorityQueue {
  typedef UnboundedBinaryHeap<KeyType, MetaKey, ElementType> Heap;
  typedef typename Heap::handle HeapHandle;

  typedef google::dense_hash_map<ElementType, HeapHandle> HeapHandles;

 public:
  UnboundedPriorityQueue(const UnboundedPriorityQueue&) = delete;
  UnboundedPriorityQueue(UnboundedPriorityQueue&&) = delete;
  UnboundedPriorityQueue& operator= (const UnboundedPriorityQueue&) = delete;
  UnboundedPriorityQueue& operator= (UnboundedPriorityQueue&&) = delete;

  explicit UnboundedPriorityQueue(size_t reserve_size = 0) :
    _heap(reserve_size),
    _handles(reserve_size) {
    _handles.set_empty_key(std::numeric_limits<ElementType>::max());
    _handles.set_deleted_key(std::numeric_limits<ElementType>::max() - 1);
  }

  size_t size() const {
    return _heap.size();
  }

  bool empty() const {
    return size() == 0;
  }

  void insert(ElementType element, KeyType key) {
    _handles[element] = _heap.push(-key, element);
  }

  void reInsert(ElementType element, KeyType key) {
    _handles[element] = _heap.push(-key, element);
  }

  void deleteMax() {
    _handles.erase(_heap.getMin());
    _heap.deleteMin();
  }

  ElementType max() const {
    return _heap.getMin();
  }

  KeyType maxKey() const {
    return -_heap.getMinKey();
  }

  KeyType key(ElementType element) const {
    ASSERT(_handles.find(element) != _handles.end(),
           "Element " << element << " not contained in PQ");
    return -_heap.getKey(_handles.find(element)->second);
  }

  bool contains(ElementType element) const {
    auto handle_it = _handles.find(element);
    if (handle_it != _handles.end()) {
      ASSERT(_heap.contains(handle_it->second), "Error");
      return true;
    }
    return false;
  }

  void updateKey(ElementType element, KeyType key) {
    ASSERT(_handles.find(element) != _handles.end(),
           "Element " << element << " not contained in PQ");
    _heap.updateKey(_handles[element], -key);
  }

  void increaseKey(ElementType element, KeyType key) {
    ASSERT(_handles.find(element) != _handles.end(),
           "Element " << element << " not contained in PQ");
    _heap.increaseKey(_handles[element], -key);
  }

  void decreaseKey(ElementType element, KeyType key) {
    ASSERT(_handles.find(element) != _handles.end(),
           "Element " << element << " not contained in PQ");
    _heap.decreaseKey(_handles[element], -key);
  }

  void remove(ElementType element) {
    ASSERT(_handles.find(element) != _handles.end(),
           "Element " << element << " not contained in PQ");
    _heap.remove(_handles[element]);
    _handles.erase(element);
  }

  void clear() {
    _heap.clear();
    _handles.clear_no_resize();
  }

 private:
  Heap _heap;
  HeapHandles _handles;
};
}  // namespace datastructure
