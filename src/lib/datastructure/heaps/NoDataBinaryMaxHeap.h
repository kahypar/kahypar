/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#ifndef SRC_LIB_DATASTRUCTURE_HEAPS_NODATABINARYMAXHEAP_H_
#define SRC_LIB_DATASTRUCTURE_HEAPS_NODATABINARYMAXHEAP_H_

#include <algorithm>
#include <limits>
#include <map>
#include <unordered_map>
#include <vector>

#include "external/binary_heap/QueueStorages.hpp"
#include "external/binary_heap/exception.h"

#include "lib/macros.h"

using external::ArrayStorage;

namespace datastructure {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
// id_slot : ID Type for stored elements
// key_slot: type of key_slot used
// Meta key slot: min/max values for key_slot accessible via static functions ::max() / ::min()
template <typename id_slot, typename key_slot,
          typename meta_key_slot = std::numeric_limits<key_slot>,
          typename storage_slot = ArrayStorage<id_slot> >
class NoDataBinaryMaxHeap {
 private:
  using Storage = storage_slot;

 protected:
  struct HeapElement {
    HeapElement(const key_slot& key_ = meta_key_slot::max(), const id_slot& id_ = 0) :
      key(key_),
      id(id_)
    { }

    key_slot key;
    id_slot id;
  };

  std::vector<HeapElement> heap;
  Storage handles;

  unsigned int next_slot;
  size_t max_size;

 public:
  using value_type = id_slot;
  using key_type = key_slot;
  using meta_key_type = meta_key_slot;
  using data_type = void;

  explicit NoDataBinaryMaxHeap(const id_slot& storage_initializer,
                               const key_slot UNUSED(unused) = 0) noexcept :
    handles(storage_initializer),
    max_size(storage_initializer + 1) {
    next_slot = 0;
    heap.reserve(max_size);
    heap[next_slot] = HeapElement(meta_key_slot::max());  // insert the sentinel element
    ++next_slot;
  }

  NoDataBinaryMaxHeap(const NoDataBinaryMaxHeap&) = delete;
  NoDataBinaryMaxHeap& operator= (const NoDataBinaryMaxHeap&) = delete;

  NoDataBinaryMaxHeap(NoDataBinaryMaxHeap&&) = default;
  NoDataBinaryMaxHeap& operator= (NoDataBinaryMaxHeap&&) = default;

  void swap(NoDataBinaryMaxHeap& other) noexcept {
    using std::swap;
    swap(heap, other.heap);
    swap(handles, other.handles);
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
  inline void push(const id_slot& id, const key_slot& key) noexcept {
    ASSERT(!contains(id), "pushing already contained element" << id);
    ASSERT(next_slot + 1 <= max_size, "heap size overflow");

    const size_t handle = next_slot++;
    heap[handle] = HeapElement(key, id);
    handles[id] = handle;
    upHeap(handle);
    ASSERT(heap[handles[id]].key == key, "Push failed - wrong key:"
           << heap[handles[id]].key << "!=" << key);
    ASSERT(heap[handles[id]].id == id, "Push failed - wrong id:"
           << heap[handles[id]].id << "!=" << id);
  }

  inline void deleteMax() noexcept {
    ASSERT(!empty(), "Deleting from empty heap");

    const size_t max_handle = handles[heap[1].id];
    const size_t swap_handle = next_slot - 1;
    handles[heap[swap_handle].id] = 1;
    handles[heap[max_handle].id] = 0;
    heap[1] = heap[swap_handle];
    --next_slot;
    if (!empty()) {
      downHeap(1);
    }
    GUARANTEE2(checkHeapProperty(), std::runtime_error,
               "[error] BinaryHeap::deleteMin - Heap property not fulfilled after deleteMin()")
  }

  inline void deleteNode(const id_slot& id) noexcept {
    ASSERT(contains(id), "trying to delete element not in heap: " << id);

    const size_t node_handle = handles[id];
    const size_t swap_handle = next_slot - 1;
    if (node_handle != swap_handle) {
      const key_slot node_key = heap[node_handle].key;
      handles[heap[swap_handle].id] = node_handle;
      handles[id] = 0;
      heap[node_handle] = heap[swap_handle];
      --next_slot;
      if (heap[node_handle].key > node_key) {
        upHeap(node_handle);
      } else if (heap[node_handle].key < node_key) {
        downHeap(node_handle);
      }
    } else {
      --next_slot;
      handles[id] = 0;
    }
  }

  inline const id_slot & getMax() const noexcept {
    ASSERT(!empty(), "Requesting minimum of empty heap");
    return heap[1].id;
  }

  inline const key_slot & getMaxKey() const noexcept {
    ASSERT(!empty(), "Requesting minimum key of empty heap");
    return heap[1].key;
  }

  inline const key_slot & getCurrentKey(const id_slot& id) const noexcept {
    ASSERT(isReached(id), "Accessing invalid element:" << id);
    return heap[handles[id]].key;
  }

  inline const key_slot & getKey(const id_slot& id) const noexcept {
    ASSERT(isReached(id), "Accessing invalid element:" << id);
    return heap[handles[id]].key;
  }

  inline bool isReached(const id_slot& id) const noexcept {
    const size_t handle = handles[id];
    return handle < next_slot && id == heap[handle].id;
  }

  inline bool contains(const id_slot& id) const noexcept {
    const size_t handle = handles[id];
    return isReachedIndexed(id, handle) && handle != 0;
  }

  inline void decreaseKey(const id_slot& id, const key_slot& new_key) noexcept {
    ASSERT(contains(id), "Calling decreaseKey for element not contained in Queue:" << id);

    const size_t handle = handles[id];
    heap[handle].key = new_key;
    downHeap(handle);
    ASSERT(heap[handles[id]].key == new_key, "decreaseKey failed - wrong key:" <<
           heap[handles[id]].key << "!=" << new_key);
    ASSERT(heap[handles[id]].id == id, "decreaseKey failed - wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void increaseKey(const id_slot& id, const key_slot& new_key) noexcept {
    ASSERT(contains(id), "Calling increaseKey for element not contained in Queue:" << id);

    const size_t handle = handles[id];
    heap[handle].key = new_key;
    upHeap(handle);
    ASSERT(heap[handles[id]].key == new_key, "increaseKey failed - wrong key:" <<
           heap[handles[id]].key << "!=" << new_key);
    ASSERT(heap[handles[id]].id == id, "increaseKey failed - wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void updateKey(const id_slot& id, const key_slot& new_key) noexcept {
    ASSERT(contains(id), "Calling updateKey for element not contained in Queue:" << id);

    const size_t handle = handles[id];
    if (new_key < heap[handle].key) {
      heap[handle].key = new_key;
      downHeap(handle);
    } else {
      heap[handle].key = new_key;
      upHeap(handle);
    }
    ASSERT(heap[handles[id]].key == new_key, "updateKey failed - wrong key:" <<
           heap[handles[id]].key << "!=" << new_key);
    ASSERT(heap[handles[id]].id == id, "updateKey failed - wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void decreaseKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    ASSERT(contains(id), "Calling decreaseKeyBy for element not contained in Queue:" << id);

#ifndef NDEBUG
    const key_slot old_key = heap[handles[id]].key;
#endif
    const size_t handle = handles[id];
    heap[handle].key -= key_delta;
    downHeap(handle);
    ASSERT(heap[handles[id]].key == old_key - key_delta, "wrong key:" <<
           heap[handles[id]].key << "!=" << old_key + key_delta);
    ASSERT(heap[handles[id]].id == id, "wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void increaseKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    ASSERT(contains(id), "Calling increaseKeyBy for element not contained in Queue:" << id);

#ifndef NDEBUG
    const key_slot old_key = heap[handles[id]].key;
#endif
    const size_t handle = handles[id];
    heap[handle].key += key_delta;
    upHeap(handle);
    ASSERT(heap[handles[id]].key == old_key + key_delta, "wrong key:" <<
           heap[handles[id]].key << "!=" << old_key + key_delta);
    ASSERT(heap[handles[id]].id == id, "wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void updateKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    ASSERT(contains(id), "Calling updateKeyBy for element not contained in Queue:" << id);

#ifndef NDEBUG
    const key_slot old_key = heap[handles[id]].key;
#endif
    const size_t handle = handles[id];
    heap[handle].key += key_delta;
    if (key_delta < 0) {
      downHeap(handle);
    } else {
      upHeap(handle);
    }
    ASSERT(heap[handles[id]].key == old_key + key_delta, "wrong key:" <<
           heap[handles[id]].key << "!=" << old_key + key_delta);
    ASSERT(heap[handles[id]].id == id, "wrong id" <<
           heap[handles[id]].id << "!=" << id);
  }

  inline void clear() noexcept {
    handles.clear();
    next_slot = 1;  // remove anything but the sentinel
    ASSERT(heap[0].key == meta_key_slot::max(), "Sentinel element missing");
  }

  inline void clearHeap() noexcept {
    clear();
  }

 protected:
  inline bool isReachedIndexed(const id_slot& id, const size_t& handle) const noexcept  {
    return handle < next_slot && id == heap[handle].id;
  }

  inline void upHeap(size_t heap_position) noexcept {
    ASSERT(0 != heap_position, "calling upHeap for the sentinel");
    ASSERT(next_slot > heap_position, "position specified larger than heap size");

    const key_slot rising_key = heap[heap_position].key;
    const id_slot rising_id = heap[heap_position].id;
    size_t next_position = heap_position >> 1;
    while (heap[next_position].key < rising_key) {
      ASSERT(0 != next_position, "swapping the sentinel, wrong meta key supplied");
      heap[heap_position] = heap[next_position];
      handles[heap[heap_position].id] = heap_position;
      heap_position = next_position;
      next_position >>= 1;
    }

    heap[heap_position].key = rising_key;
    heap[heap_position].id = rising_id;
    handles[rising_id] = heap_position;
    GUARANTEE2(checkHeapProperty(), std::runtime_error,
               "[error] BinaryHeap::upHeap - Heap property not fulfilled after upHeap()")
  }

  inline void downHeap(size_t heap_position) noexcept {
    ASSERT(0 != heap_position, "calling downHeap for the sentinel");
    ASSERT(next_slot > heap_position, "position specified larger than heap size");
    const key_slot dropping_key = heap[heap_position].key;
    const id_slot dropping_id = heap[heap_position].id;
    const size_t heap_size = next_slot;
    size_t compare_position = heap_position << 1 | 1;  // heap_position * 2 + 1
    while (compare_position<heap_size&&
                            heap[compare_position = compare_position -
                                                    (heap[compare_position - 1].key > heap[compare_position].key)].key> dropping_key) {
      heap[heap_position] = heap[compare_position];
      handles[heap[heap_position].id] = heap_position;
      heap_position = compare_position;
      compare_position = compare_position << 1 | 1;
    }

    // check for possible last single child
    if ((heap_size == compare_position) && heap[compare_position -= 1].key > dropping_key) {
      heap[heap_position] = heap[compare_position];
      handles[heap[heap_position].id] = heap_position;
      heap_position = compare_position;
    }

    heap[heap_position].key = dropping_key;
    heap[heap_position].id = dropping_id;
    handles[dropping_id] = heap_position;
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
      if (heap[i / 2].key < heap[i].key)
        return false;
    }
    return true;
  }
};
#pragma GCC diagnostic pop

template <typename id_slot, typename key_slot, typename meta_key_slot, typename storage_slot = ArrayStorage<id_slot> >
void swap(NoDataBinaryMaxHeap<id_slot, key_slot, meta_key_slot, storage_slot>& a,
          NoDataBinaryMaxHeap<id_slot, key_slot, meta_key_slot, storage_slot>& b) noexcept {
  a.swap(b);
}
}  // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_HEAPS_NODATABINARYMAXHEAP_H_
