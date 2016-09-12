/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "external/binary_heap/exception.h"
#include "macros.h"

#include <limits>
#include <vector>
#include <algorithm>
#include <cstring>
#include <limits>

namespace datastructure {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
// key_slot: type of key_slot used
// Meta key slot: min/max values for key_slot accessible via static functions ::max() / ::min()
template <typename key_slot,
          typename meta_key_slot = std::numeric_limits<key_slot> >
class NonAddressableMaxHeap {
  std::vector<key_slot> heap;

  unsigned int next_slot;
  size_t max_size;

 public:
  using value_type = key_slot;
  using key_type = key_slot;
  using meta_key_type = meta_key_slot;
  using data_type = void;

  explicit NonAddressableMaxHeap(const size_t storage_initializer) noexcept :
    max_size(storage_initializer + 1) {
    next_slot = 0;
    heap.reserve(max_size);
    heap[next_slot] = meta_key_slot::max();  // insert the sentinel element
    ++next_slot;
  }

  NonAddressableMaxHeap(const NonAddressableMaxHeap&) = delete;
  NonAddressableMaxHeap& operator= (const NonAddressableMaxHeap&) = delete;

  NonAddressableMaxHeap(NonAddressableMaxHeap&&) = default;
  NonAddressableMaxHeap& operator= (NonAddressableMaxHeap&&) = default;

  void swap(NonAddressableMaxHeap& other) noexcept {
    using std::swap;
    swap(heap, other.heap);
    swap(next_slot, other.next_slot);
    swap(max_size, other.max_size);
  }

  size_t size() const noexcept {
    return next_slot - 1;
  }

  bool empty() const noexcept {
    return size() == 0;
  }

  // push an element onto the heap
  inline void push(const key_slot& key) noexcept {
    GUARANTEE(next_slot + 1 <= max_size,
              std::runtime_error, "[error] BinaryHeap::push - heap size overflow")
    const size_t handle = next_slot++;
    heap[handle] = key;
    upHeap(handle);
  }

  //  only to temporarily satisfy PQ interface
  inline void reinsertingPush(const key_slot& key) noexcept {
    push(key);
  }

  inline void deleteMax() noexcept {
    GUARANTEE(!empty(), std::runtime_error,
              "[error] BinaryHeap::deleteMin - Deleting from empty heap")
    heap[1] = heap[next_slot - 1];
    --next_slot;
    if (!empty()) {
      downHeap(1);
    }
    GUARANTEE2(checkHeapProperty(), std::runtime_error,
               "[error] BinaryHeap::deleteMin - Heap property not fulfilled after deleteMin()")
  }

  inline const key_slot & getMaxKey() const noexcept {
    GUARANTEE(!empty(), std::runtime_error,
              "[error] BinaryHeap::getMinKey() - Requesting minimum key of empty heap")
    return heap[1];
  }

  inline void clear() noexcept {
    next_slot = 1;  // remove anything but the sentinel
    GUARANTEE(heap[0] == meta_key_slot::max(),
              std::runtime_error, "[error] BinaryHeap::clear missing sentinel")
  }

  inline void clearHeap() noexcept {
    clear();
  }

 protected:
  inline void upHeap(size_t heap_position) noexcept {
    GUARANTEE(0 != heap_position, std::runtime_error,
              "[error] BinaryHeap::upHeap - calling upHeap for the sentinel")
    GUARANTEE(next_slot > heap_position, std::runtime_error,
              "[error] BinaryHeap::upHeap - position specified larger than heap size")
    const key_slot rising_key = heap[heap_position];
    size_t next_position = heap_position >> 1;
    while (heap[next_position] < rising_key) {
      GUARANTEE(0 != next_position, std::runtime_error,
                "[error] BinaryHeap::upHeap - swapping the sentinel, wrong meta key supplied")
      heap[heap_position] = heap[next_position];
      heap_position = next_position;
      next_position >>= 1;
    }
    heap[heap_position] = rising_key;
    GUARANTEE2(checkHeapProperty(), std::runtime_error,
               "[error] BinaryHeap::upHeap - Heap property not fulfilled after upHeap()")
  }

  inline void downHeap(size_t heap_position) noexcept {
    GUARANTEE(0 != heap_position, std::runtime_error,
              "[error] BinaryHeap::downHeap - calling downHeap for the sentinel")
    GUARANTEE(next_slot > heap_position, std::runtime_error,
              "[error] BinaryHeap::downHeap - position specified larger than heap size")
    const key_slot dropping_key = heap[heap_position];
    const size_t heap_size = next_slot;
    size_t compare_position = heap_position << 1 | 1;  // heap_position * 2 + 1
    while (compare_position<heap_size&&
                            heap[compare_position = compare_position -
                                                    (heap[compare_position - 1] >
                                                     heap[compare_position])]> dropping_key) {
      heap[heap_position] = heap[compare_position];
      heap_position = compare_position;
      compare_position = compare_position << 1 | 1;
    }

    // check for possible last single child
    if ((heap_size == compare_position) && heap[compare_position -= 1] > dropping_key) {
      heap[heap_position] = heap[compare_position];
      heap_position = compare_position;
    }

    heap[heap_position] = dropping_key;
    GUARANTEE2(checkHeapProperty(), std::runtime_error,
               "[error] BinaryHeap::downHeap - Heap property not fulfilled after downHeap()")
  }

  /*
   * checkHeapProperty()
   * check whether the heap property is fulfilled.
   * This is the case if every parent of a node is at least as small as the current element
   */
  bool checkHeapProperty() noexcept {
    for (size_t i = 1; i < next_slot; ++i) {
      if (heap[i / 2] < heap[i])
        return false;
    }
    return true;
  }
};
#pragma GCC diagnostic pop

template <typename key_slot, typename meta_key_slot>
void swap(NonAddressableMaxHeap<key_slot, meta_key_slot>& a,
          NonAddressableMaxHeap<key_slot, meta_key_slot>& b) noexcept {
  a.swap(b);
}
}  // namespace datastructure
