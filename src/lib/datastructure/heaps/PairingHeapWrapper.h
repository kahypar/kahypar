/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#ifndef SRC_LIB_DATASTRUCTURE_HEAPS_PAIRINGHEAPWRAPPER_H_
#define SRC_LIB_DATASTRUCTURE_HEAPS_PAIRINGHEAPWRAPPER_H_

#include <functional>
#include <ext/pb_ds/priority_queue.hpp>
#include <vector>

#include "lib/macros.h"
#include "lib/datastructure/SparseMap.h"

namespace datastructure {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
// id_slot : ID Type for stored elements
// key_slot: type of key_slot used
// Meta key slot: min/max values for key_slot accessible via static functions ::max() / ::min()
template <typename id_slot, typename key_slot,
          typename meta_key_slot = std::numeric_limits<key_slot> >
class PairingHeapWrapper {
 private:
  using HeapElement = std::pair<id_slot,key_slot>;

  struct LessBy2nd {
    bool operator()(const HeapElement& left, const HeapElement& right) const {
      return left.second < right.second; };
  };

  using Heap = __gnu_pbds::priority_queue<HeapElement,
                                          LessBy2nd,
                                          __gnu_pbds::pairing_heap_tag>;

  Heap _heap;
  SparseMap<id_slot, typename Heap::point_iterator> _map;

 public:
  using value_type = id_slot;
  using key_type = key_slot;
  using meta_key_type = meta_key_slot;

  explicit PairingHeapWrapper(const id_slot& storage_initializer,
                              const key_slot UNUSED(unused) = 0) noexcept :
    _heap(),
    _map(storage_initializer) { }

  PairingHeapWrapper(const PairingHeapWrapper&) = delete;
  PairingHeapWrapper& operator= (const PairingHeapWrapper&) = delete;

  PairingHeapWrapper(PairingHeapWrapper&&) = default;
  PairingHeapWrapper& operator= (PairingHeapWrapper&&) = default;

  void swap(PairingHeapWrapper& other) noexcept {
    using std::swap;
    // necessary to call member swap
    _heap.swap(other._heap);
    swap(_map, other._map);
  }

  size_t size() const noexcept {
    return _heap.size();
  }

  bool empty() const noexcept {
    return _heap.empty();
  }

  // push an element onto the heap
  inline void push(const id_slot& id, const key_slot& key) noexcept {
    ASSERT(!contains(id), V(id));
    _map[id] = _heap.push(HeapElement(id, key));
  }

  inline void deleteMax() noexcept {
    ASSERT(_map.contains(_heap.top().first), V(_heap.top().first));
    _map.remove(_heap.top().first);
    _heap.pop();
  }

  inline void deleteNode(const id_slot& id) noexcept {
    ASSERT(_map.contains(id), V(id));
    _heap.erase(_map.get(id));
    _map.remove(id);
  }

  inline const id_slot & getMax() const noexcept {
    return _heap.top().first;
  }

  inline const key_slot & getMaxKey() const noexcept {
    return _heap.top().second;
  }

  inline const key_slot & getKey(const id_slot& id) const noexcept {
    ASSERT(_map.contains(id), V(id));
    ASSERT(_map.get(id)->first == id, V(id));
    return _map.get(id)->second;
  }

  inline bool contains(const id_slot& id) const noexcept {
    return _map.contains(id);
  }

  inline void decreaseKey(const id_slot& id, const key_slot& new_key) noexcept {
    _heap.modify(_map.get(id), HeapElement(id, new_key));
  }

  inline void increaseKey(const id_slot& id, const key_slot& new_key) noexcept {
    _heap.modify(_map.get(id), HeapElement(id, new_key));
  }

  inline void updateKey(const id_slot& id, const key_slot& new_key) noexcept {
    _heap.modify(_map.get(id), HeapElement(id,new_key));
  }

  inline void decreaseKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    _heap.modify(_map.get(id), HeapElement(id, _map.get(id)->second - key_delta));
  }

  inline void increaseKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    _heap.modify(_map.get(id), HeapElement(id, _map.get(id)->second + key_delta));
  }

  inline void updateKeyBy(const id_slot& id, const key_slot& key_delta) noexcept {
    _heap.modify(_map.get(id), HeapElement(id,_map.get(id)->second + key_delta));
  }

  inline void clear() noexcept {
    _heap.clear();
    _map.clear();
  }

  inline void clearHeap() noexcept {
    clear();
  }

};
#pragma GCC diagnostic pop

template <typename id_slot, typename key_slot, typename meta_key_slot>
void swap(PairingHeapWrapper<id_slot, key_slot, meta_key_slot>& a,
          PairingHeapWrapper<id_slot, key_slot, meta_key_slot>& b) noexcept {
  a.swap(b);
}
}  // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_HEAPS_PAIRINGHEAPWRAPPER_H_
