/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
#define SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_

#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "external/binary_heap/QueueStorages.hpp"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/macros.h"

using defs::PartitionID;
using external::NoDataBinaryMaxHeap;
using external::ArrayStorage;

namespace datastructure {

template <typename IDType = Mandatory,
          typename KeyType = Mandatory,
          typename MetaKey = Mandatory,
          typename Storage = ArrayStorage<IDType> >
class KWayPriorityQueue {
  typedef NoDataBinaryMaxHeap<IDType, KeyType, MetaKey, Storage> Heap;

  static const size_t kInvalidIndex = std::numeric_limits<size_t>::max();
  static const PartitionID kInvalidPart = std::numeric_limits<PartitionID>::max();

 public:
  KWayPriorityQueue(const IDType size, const PartitionID k) :
      _heaps(k, Heap(size)),
      _index(k, kInvalidIndex),
      _part(k, kInvalidPart),
      _num_entries(0),
      _num_nonempty_pqs(0) { }

  size_t size(const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    ASSERT(_index[part] != kInvalidIndex, V(part));
    return _heaps[_index[part]].size();
  }

  size_t size() const {
    return _num_entries;
  }

  bool empty(const PartitionID part) const {
    ASSERT(_index[part] != kInvalidIndex, V(part));
    return _heaps[_index[part]].size() == 0;
  }

  bool empty() const {
    return _num_entries == 0;
  }

  void insert(const IDType id, const PartitionID part, const KeyType key) {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    DBG(false, "Insert: (" << id << "," << part << "," << key << ")");
    if (_index[part] == kInvalidIndex) {
      ASSERT(_part[_num_nonempty_pqs] == kInvalidPart, V(_part[_num_nonempty_pqs]));
      _part[_num_nonempty_pqs] = part;
      _heaps[_num_nonempty_pqs].clear(); // lazy clear
      _index[part] = _num_nonempty_pqs++;
    }
    _heaps[_index[part]].push(id, key);
    ++_num_entries;
  }

  void deleteMax(IDType& max_id, KeyType& max_key, PartitionID& max_part) {
    size_t max_index = maxIndex();
    max_part = _part[max_index];
    max_id = _heaps[max_index].getMax();
    max_key = _heaps[max_index].getMaxKey();

    ASSERT(max_index != kInvalidIndex,V(max_index));
    ASSERT(max_key != MetaKey::max(), V(max_key));
    ASSERT(max_part != kInvalidPart, V(max_part) << V(max_id));

    _heaps[max_index].deleteMax();
    if (_heaps[max_index].empty()) {
      --_num_nonempty_pqs;
      swap(max_index, _num_nonempty_pqs);
      _part[_num_nonempty_pqs] = kInvalidPart;
      _index[max_part] = kInvalidIndex;

    }
    --_num_entries;
  }

  void swap(const size_t index_a, const size_t index_b) {
    using std::swap;
    swap(_heaps[index_a], _heaps[index_b]);
    swap(_part[index_a], _part[index_b]);
    swap(_index[_part[index_a]], _index[_part[index_b]]);
    ASSERT(_index[_part[index_a]] == index_a
           && _index[_part[index_b]] ==index_b, "Swap failed");
  }

  IDType max() const {
    LOG("Should only be used for testing");
    return _heaps[maxIndex()].getMax();
  }

  KeyType maxKey() const {
    LOG("Should only be used for testing");
    return _heaps[maxIndex()].getMaxKey();;
  }

  KeyType key(const IDType id, const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    return _heaps[_index[part]].getKey(id);
  }

  bool contains(const IDType id, const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    return _index[part] < _num_nonempty_pqs && _heaps[_index[part]].contains(id);
  }

  // Should be used only for assertions
  bool contains(const IDType id) const {
    for (size_t i = 0; i < _num_nonempty_pqs; ++i) {
      if (_heaps[i].contains(id)) {
        return true;
      }
    }
      return false;
  }

  void updateKey(const IDType id, const PartitionID part, const KeyType key) {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    _heaps[_index[part]].updateKey(id, key);
  }

  void remove(const IDType id, const PartitionID part) {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    ASSERT(_index[part] < _num_nonempty_pqs, V(part));
    ASSERT(_heaps[_index[part]].contains(id), V(id) << V(part));
    _heaps[_index[part]].deleteNode(id);
    if (_heaps[_index[part]].empty()) {
      --_num_nonempty_pqs;
      swap(_index[part], _num_nonempty_pqs);
      _part[_num_nonempty_pqs] = kInvalidPart;
      _index[part] = kInvalidIndex;
    }
    --_num_entries;
  }

  void clear() {
    for (size_t i = 0; i < _heaps.size(); ++i) {
      _index[i] = kInvalidIndex;
      _part[i] = kInvalidPart;
    }
    _num_entries = 0;
    _num_nonempty_pqs = 0;
  }

 private:
  size_t maxIndex() const {
    size_t max_index = kInvalidIndex;
    KeyType max_key = std::numeric_limits<KeyType>::min();
    for (size_t index = 0; index < _num_nonempty_pqs; ++index) {
      ASSERT(!_heaps[index].empty(), V(index));
      const KeyType key = _heaps[index].getMaxKey();
      if (key > max_key) {
        max_key = key;
        max_index = index;
      }
    }
    ASSERT(max_index != kInvalidIndex,V(max_index));
    return max_index;
  }

  std::vector<Heap> _heaps;
  std::vector<size_t> _index; // part to index mapping
  std::vector<PartitionID> _part; // index to part mapping
  size_t _num_entries;
  size_t _num_nonempty_pqs;
  DISALLOW_COPY_AND_ASSIGN(KWayPriorityQueue);
};

template <typename IDType,
          typename KeyType,
          typename MetaKey,
          class Storage >
constexpr size_t KWayPriorityQueue<IDType,KeyType,MetaKey,Storage>::kInvalidIndex;
template <typename IDType,
          typename KeyType,
          typename MetaKey,
          class Storage >
constexpr PartitionID KWayPriorityQueue<IDType,KeyType,MetaKey,Storage>::kInvalidPart;
} // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
