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
  typedef NoDataBinaryMaxHeap<PartitionID,KeyType,MetaKey,Storage> TopHeap;

  public:
  KWayPriorityQueue(const IDType size, const PartitionID k) :
      _buf(k),
      _heaps(k, Heap(size)),
      _num_entries(0) {
    // initialize top heap buffer with sentinel elements
    // that will be updated continuously
    for (PartitionID part = 0; part < k; ++part) {
      _buf.push(part, MetaKey::min());
    }
  }

  size_t size(const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    return _heaps[part].size();
  }

  size_t size() const {
    return _num_entries;
  }

  bool empty(const PartitionID part) const {
    return _heaps[part].size() == 0;
  }

  bool empty() const {
    return _num_entries == 0;
  }

  void insert(const IDType id, const PartitionID part, const KeyType key) {
    ASSERT(static_cast<unsigned int>(part) < _buf.size(), "Invalid " << V(part));
    DBG(false, "Insert: (" << id << "," << part << "," << key << ")");
    if (key > _buf.getKey(part)) {
      _buf.increaseKey(part, key);
    }
    _heaps[part].push(id, key);
    ASSERT(_buf.getKey(part) == _heaps[part].getMaxKey(),
           "buf.key=" << _buf.getKey(part) << "!=" << _heaps[part].getMaxKey());
    ++_num_entries;
  }

  void deleteMax(IDType& id, KeyType& key, PartitionID& part) {
    ASSERT(_buf.getKey(_buf.getMax()) != MetaKey::min(), "Invalid bufkey");
    ASSERT(_heaps[_buf.getMax()].size() != 0, "Trying to delete from empty heap");
    ASSERT(_buf.getKey(_buf.getMax()) == _heaps[_buf.getMax()].getMaxKey(),
           "p=" << _buf.getMax() << "Wrong Key:"<< _buf.getKey(_buf.getMax())
           << "!=" << _heaps[_buf.getMax()].getMaxKey());

    part = _buf.getMax();
    key = _buf.getMaxKey();
    id = _heaps[part].getMax();
    _heaps[part].deleteMax();

    if (!_heaps[part].empty()) {
      _buf.decreaseKey(part, _heaps[part].getMaxKey());
    } else {
      _buf.decreaseKey(part, MetaKey::min());
    }
    --_num_entries;
  }

  IDType max() const {
    return _heaps[_buf.getMax()].getMax();
  }

  KeyType maxKey() const {
    return _buf.getMaxKey();
  }

  KeyType key(const IDType id, const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    return _heaps[part].getKey(id);
  }

  bool contains(const IDType id, const PartitionID part) const {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    return _heaps[part].contains(id);
  }

  // Should be used only for assertions
  bool contains(const IDType id) const {
    for (size_t i = 0; i < _heaps.size(); ++i) {
      if (_heaps[i].contains(id)) {
        return true;
      }
    }
      return false;
  }

  void updateKey(const IDType id, const PartitionID part, const KeyType key) {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    DBG(false, "Update: (" << id << "," << part << "," << key << ")");
    DBG(false, "Buffer state: key=" <<  _buf.getKey(part));
    DBG(false, "Heap state: key=" << _heaps[part].getMaxKey() << " id=" << _heaps[part].getMax());
    _heaps[part].updateKey(id, key);
    _buf.updateKey(part, _heaps[part].getMaxKey());
    //ASSERT(!_enabled[part] || _buf.getKey(part) == _heaps[part].getMaxKey(),
               //"buf.key=" << _buf.getKey(part) << "!=" << _heaps[part].getMaxKey());
  }

  void remove(const IDType id, const PartitionID part) {
    ASSERT(static_cast<unsigned int>(part) < _heaps.size(), "Invalid " << V(part));
    _heaps[part].deleteNode(id);
    if (!_heaps[part].empty()) {
      _buf.updateKey(part, _heaps[part].getMaxKey());
    } else {
      _buf.decreaseKey(part, MetaKey::min());
    }
    --_num_entries;
  }

  void clear() {
    for (size_t i = 0; i < _heaps.size(); ++i) {
      _heaps[i].clear();
      _buf.decreaseKey(i, MetaKey::min());
    }
    _num_entries = 0;
  }

 private:
  TopHeap _buf;
  std::vector<Heap> _heaps;
  size_t _num_entries;
  DISALLOW_COPY_AND_ASSIGN(KWayPriorityQueue);
};
} // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
