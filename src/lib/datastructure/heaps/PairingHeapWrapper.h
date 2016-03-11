/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#ifndef SRC_LIB_DATASTRUCTURE_HEAPS_PAIRINGHEAPWRAPPER_H_
#define SRC_LIB_DATASTRUCTURE_HEAPS_PAIRINGHEAPWRAPPER_H_

#include <algorithm>
#include <ext/pb_ds/priority_queue.hpp>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include "lib/macros.h"

namespace datastructure {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
// id_slot : ID Type for stored elements
// key_slot: type of key_slot used
// Meta key slot: unused for wrapper
template <typename id_slot, typename key_slot,
          typename meta_key_slot = std::numeric_limits<key_slot> >
class PairingHeapWrapper {
 private:
  using HeapElement = std::pair<id_slot, key_slot>;

  struct LessBy2nd {
    bool operator() (const HeapElement& left, const HeapElement& right) const {
      return left.second < right.second;
    }
  };

  using Heap = __gnu_pbds::priority_queue<HeapElement,
                                          LessBy2nd,
                                          __gnu_pbds::pairing_heap_tag>;

  using HeapIterator = typename Heap::point_iterator;

 public:
  using value_type = id_slot;
  using key_type = key_slot;
  using meta_key_type = meta_key_slot;

  explicit PairingHeapWrapper(const id_slot& storage_initializer,
                              const key_slot UNUSED(unused) = 0) noexcept :
    _heap(),
    _map(storage_initializer, nullptr),
    _updates() { }

  PairingHeapWrapper(const PairingHeapWrapper&) = delete;
  PairingHeapWrapper& operator= (const PairingHeapWrapper&) = delete;

  PairingHeapWrapper(PairingHeapWrapper&&) = default;
  PairingHeapWrapper& operator= (PairingHeapWrapper&&) = default;

  void swap(PairingHeapWrapper& other) noexcept {
    using std::swap;
    // necessary to call member swap
    _heap.swap(other._heap);
    swap(_map, other._map);
    swap(_updates, other._updates);
  }

  size_t size() const noexcept {
    return _heap.size();
  }

  bool empty() const noexcept {
    return _heap.empty();
  }

  inline void push(const id_slot& id, const key_slot& key) noexcept {
    ASSERT(!contains(id), V(id));
    _map[id] = _heap.push(HeapElement(id, key));
    _updates.push_back(id);
  }

  inline void deleteMax() noexcept {
    ASSERT(contains(_heap.top().first), V(_heap.top().first));
    _map[_heap.top().first] = nullptr;
    _heap.pop();
  }

  inline void deleteNode(const id_slot& id) noexcept {
    ASSERT(contains(id), V(id));
    _heap.erase(_map[id]);
    _map[id] = nullptr;
  }

  inline const id_slot & getMax() const noexcept {
    return _heap.top().first;
  }

  inline const key_slot & getMaxKey() const noexcept {
    return _heap.top().second;
  }

  inline const key_slot & getKey(const id_slot& id) const noexcept {
    ASSERT(contains(id), V(id));
    return _map[id]->second;
  }

  inline bool contains(const id_slot& id) const noexcept {
    return _map[id] != nullptr;
  }

  inline void decreaseKey(const id_slot& id, const key_slot& new_key) noexcept {
    ASSERT(contains(id), V(id));
    _heap.modify(_map[id], HeapElement(id, new_key));
  }

  inline void increaseKey(const id_slot& id, const key_slot& new_key) noexcept {
    ASSERT(contains(id), V(id));
    _heap.modify(_map[id], HeapElement(id, new_key));
  }

  inline void updateKey(const id_slot& id, const key_slot& new_key) noexcept {
    ASSERT(contains(id), V(id));
    _heap.modify(_map[id], HeapElement(id, new_key));
  }

  inline void decreaseKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    ASSERT(contains(id), V(id));
    _heap.modify(_map[id], HeapElement(id, _map[id]->second - key_delta));
  }

  inline void increaseKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    ASSERT(contains(id), V(id));
    _heap.modify(_map[id], HeapElement(id, _map[id]->second + key_delta));
  }

  inline void updateKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    ASSERT(contains(id), V(id));
    _heap.modify(_map[id], HeapElement(id, _map[id]->second + key_delta));
  }

  inline void clear() noexcept {
    _heap.clear();
    for (const auto id : _updates) {
      _map[id] = nullptr;
    }
    _updates.clear();
  }

  inline void clearHeap() noexcept {
    clear();
  }

  inline void merge(PairingHeapWrapper& other) {
    _heap.join(other._heap);
    for (const auto& new_element : other._updates) {
      _map[new_element] = other._map[new_element];
      _updates.push_back(new_element);
    }
    other.clear();
  }

 private:
  Heap _heap;
  std::vector<HeapIterator> _map;
  std::vector<id_slot> _updates;
};
#pragma GCC diagnostic pop

template <typename id_slot, typename key_slot, typename meta_key_slot>
void swap(PairingHeapWrapper<id_slot, key_slot, meta_key_slot>& a,
          PairingHeapWrapper<id_slot, key_slot, meta_key_slot>& b) noexcept {
  a.swap(b);
}
}  // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_HEAPS_PAIRINGHEAPWRAPPER_H_
