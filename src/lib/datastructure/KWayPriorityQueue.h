/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
#define SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_

#include "external/binary_heap/NoDataBinaryHeap.h"
#include "external/binary_heap/QueueStorages.hpp"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/macros.h"

using defs::PartitionID;
using external::NoDataBinaryHeap;
using external::NullData;
using external::ArrayStorage;

namespace datastructure {

template <typename IDType = Mandatory,
          typename KeyType = Mandatory,
          typename MetaKey = Mandatory,
          typename Storage = ArrayStorage<IDType> >
class KWayPriorityQueue {
  typedef NoDataBinaryHeap<IDType, KeyType, MetaKey, Storage> Heap;

  struct BufferElement {
    BufferElement() :
        key(MetaKey::min()),
        id(-1)
    {}
    KeyType key;
    IDType id;
  };

  public:
  KWayPriorityQueue(IDType size, PartitionID k) :
      _heaps(k, Heap(size)),
      _buf(k),
      _num_entries(0){ }

  size_t size(PartitionID part) const {
    ASSERT(part < _heaps.size(), "Invalid Part");
    return _heaps[part].size();
  }

  size_t size() const {
    size_t size = 0;
    for (PartitionID i = 0; i < _heaps.size(); ++i) {
      size += _heaps[i].size();
    }
    return size;
  }

  bool empty(PartitionID part) const {
    return _heaps[part].size() == 0;
  }

  bool empty() const {
    return _num_entries == 0;
  }

  void insert(IDType id, PartitionID part, KeyType key) {
    ASSERT(part < _buf.size(),"");
    DBG(false, "Insert: (" << id << "," << part << "," << key << ")");
    if (_buf[part].key < key) {
      _buf[part].key = key;
      _buf[part].id = id;
    }
    _heaps[part].push(id, -key);
    ASSERT([&](){
        ASSERT(_buf[part].key == -_heaps[part].getMinKey(),
               "buf.key=" << _buf[part].key << "!=" << -_heaps[part].getMinKey());
        ASSERT(_buf[part].id == _heaps[part].getMin(),
               "buf.id=" << _buf[part].id << "!=" << _heaps[part].getMin());
        return true;
      }(),
      "Error");
    ++_num_entries;
  }

  // void reInsert(IDType id,PartitionID part, KeyType key) {
  //   DBG(false,"reInsert: (" << id << "," << part << "," << key << ")");
  //   if (_buf[part].key < key) {
  //     _buf[part].key = key;
  //     _buf[part].id = id;
  //   }
  //   _heaps[part].update(id, -key);
  //   ASSERT([&](){
  //       ASSERT(_buf[part].key == -_heaps[part].getMinKey(),
  //              "buf.key=" << _buf[part].key << "!=" << -_heaps[part].getMinKey());
  //       ASSERT(_buf[part].id == _heaps[part].getMin(),
  //              "buf.id=" << _buf[part].id << "!=" << _heaps[part].getMin());
  //       return true;
  //     }(),
  //     "Error");
  //   ++_num_entries;
  // }

  void deleteMax(IDType& id, KeyType& key, PartitionID& part) {
    PartitionID max_part = -1;
    KeyType max_key = MetaKey::min();
    for (size_t i = 0; i < _buf.size(); ++i) {
      if (_buf[i].key > max_key) {
        max_part = i;
        max_key = _buf[i].key;
      }
    }

    ASSERT(max_key != MetaKey::min(), "No max element found");
    ASSERT(_heaps[max_part].size() != 0, "Trying to delete from empty heap");
    ASSERT(_buf[max_part].key == -_heaps[max_part].getMinKey(), "p=" << max_part << "Wrong Key:"
           << _buf[max_part].key << "!=" << -_heaps[max_part].getMinKey());
    ASSERT(_buf[max_part].id == _heaps[max_part].getMin(), "p=" << max_part << " Wrong ID:" <<
           _buf[max_part].id << "!=" << _heaps[max_part].getMin());

    id = _buf[max_part].id;
    key = _buf[max_part].key;
    part = max_part;

    _heaps[max_part].deleteMin();
    if (!_heaps[max_part].empty()) {
      _buf[max_part].key = -_heaps[max_part].getMinKey();
      _buf[max_part].id = _heaps[max_part].getMin();
    } else {
      _buf[max_part].key = MetaKey::min();
      _buf[max_part].id = -1;
    }

    --_num_entries;
  }

  KeyType key(IDType id, PartitionID part) const {
    return -_heaps[part].getKey(id);
  }

  bool contains(IDType id, PartitionID part) const {
    return _heaps[part].contains(id);
  }

  void updateKey(IDType id,PartitionID part, KeyType key) {
    DBG(false, "Update: (" << id << "," << part << "," << key << ")");
    DBG(false, "Buffer state: key=" <<  _buf[part].key << ", id=" << _buf[part].id);
    if (key > _buf[part].key) {
      _buf[part].key = key;
      _buf[part].id = id;
      _heaps[part].updateKey(id, -key);
    } else {
      if (id == _buf[part].id) {
        _heaps[part].updateKey(id, -key);
        _buf[part].key = -_heaps[part].getMinKey();
        _buf[part].id = _heaps[part].getMin();
      } else {
        _heaps[part].updateKey(id, -key);
      }
    }
    ASSERT([&](){
        ASSERT(_buf[part].key == -_heaps[part].getMinKey(),
               "buf.key=" << _buf[part].key << "!=" << -_heaps[part].getMinKey());
        ASSERT(_buf[part].id == _heaps[part].getMin(),
               "buf.id=" << _buf[part].id << "!=" << _heaps[part].getMin());
        return true;
      }(),
      "Error");
  }

  void remove(IDType id, PartitionID part) {
    _heaps[part].deleteNode(id);
    if (_buf[part].id == id) {
      if (!_heaps[part].empty()) {
        _buf[part].key = -_heaps[part].getMinKey();
        _buf[part].id = _heaps[part].getMin();
      } else {
        _buf[part].key = MetaKey::min();
        _buf[part].id = -1;
      }
    }
    --_num_entries;
  }

  void clear() {
    for (size_t i = 0; i < _heaps.size(); ++i) {
      _heaps[i].clear();
      _buf[i].key = MetaKey::min();
      _buf[i].id = -1;
    }
    _num_entries = 0;
  }

  private:
  void deleteMax(PartitionID max_part) {
    ASSERT(_buf[max_part].key == -_heaps[max_part].getMinKey(), "p=" << max_part << "Wrong Key:"
           << _buf[max_part].key << "!=" << -_heaps[max_part].getMinKey());
    ASSERT(_buf[max_part].id == _heaps[max_part].getMin(), "p=" << max_part << " Wrong ID:" <<
           _buf[max_part].id << "!=" << _heaps[max_part].getMin());
    _heaps[max_part].deleteMin();
    if (!_heaps[max_part].empty()) {
      _buf[max_part].key = -_heaps[max_part].getMinKey();
      _buf[max_part].id = _heaps[max_part].getMin();
    } else {
      _buf[max_part].key = MetaKey::min();
      _buf[max_part].id = -1;
    }
    --_num_entries;
  }

  IDType max() const {
    PartitionID max_part = -1;
    KeyType max_key = MetaKey::min();
    for (size_t i = 0; i < _buf.size(); ++i) {
      if (_buf[i].key > max_key) {
        max_part = i;
        max_key = _buf[i].key;
      }
    }
    // LOG("max=" << _buf[max_part].id);
    return _buf[max_part].id;
  }

  KeyType maxKey() const {
    PartitionID max_part = -1;
    KeyType max_key = MetaKey::min();
    for (size_t i = 0; i < _buf.size(); ++i) {
      if (_buf[i].key > max_key) {
        max_part = i;
        max_key = _buf[i].key;
      }
    }
    // LOG("maxKey=" << _buf[max_part].key);
    return _buf[max_part].key;
  }

  PartitionID maxPart() const {
    PartitionID max_part = -1;
    KeyType max_key = MetaKey::min();
    for (size_t i = 0; i < _buf.size(); ++i) {
      if (_buf[i].key > max_key) {
        max_part = i;
        max_key = _buf[i].key;
      }
    }
    // LOG("maxPart=" << max_part);
    return max_part;
  }

  std::vector<Heap> _heaps;
  std::vector<BufferElement> _buf;
  size_t _num_entries;
  DISALLOW_COPY_AND_ASSIGN(KWayPriorityQueue);
};
} // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_KWAYPRIORITYQUEUE_H_
