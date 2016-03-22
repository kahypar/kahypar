/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_CONNECTIVITYSET_H_
#define SRC_LIB_DATASTRUCTURE_CONNECTIVITYSET_H_

#include <memory>
#include <limits>

#include "lib/definitions.h"

using defs::PartitionID;

namespace datastructure {
class ConnectivitySet {
 public:
  explicit ConnectivitySet(const PartitionID k) :
      _size(nullptr),
      _sparse(nullptr),
    _dense(nullptr) {
    PartitionID* raw = static_cast<PartitionID*>(malloc(((2*k) + 1) * sizeof(PartitionID)));
    new(raw) PartitionID(0);
    for (PartitionID i = 1; i < 2*k+1; ++i) {
      new(raw+i) PartitionID(std::numeric_limits<PartitionID>::max());
    }
    _size = raw;
    _sparse = raw+1;
    _dense = raw+k+1;
  }

  ~ConnectivitySet() {
    free(_size);
  }

  ConnectivitySet(const ConnectivitySet&) = delete;
  ConnectivitySet& operator= (const ConnectivitySet&) = delete;

  ConnectivitySet(ConnectivitySet&&) = default;
  ConnectivitySet& operator= (ConnectivitySet&&) = default;

  bool contains(const PartitionID value) const {
    const PartitionID index = _sparse[value];
    return index < *_size && _dense[index] == value;
  }

  void add(const PartitionID value) {
    const PartitionID index = _sparse[value];
    const PartitionID n = *_size;
    if (index >= n || _dense[index] != value) {
      _sparse[value] = n;
      _dense[n] = value;
      ++*_size;
    }
  }

  void remove(const PartitionID value) {
    const PartitionID index = _sparse[value];
    if (index <= (*_size) - 1 && _dense[index] == value) {
      const PartitionID e = _dense[--(*_size)];
      _dense[index] = e;
      _sparse[e] = index;
    }
  }

  PartitionID size() const {
    return *_size;
  }

  auto begin() const {
    return _dense;
  }

  auto end() const {
    return &_dense[*_size];
  }

  void clear() {
    *_size = 0;
  }

 private:
  PartitionID* _size;
  PartitionID* _sparse;
  PartitionID* _dense;

};

}

#endif // SRC_LIB_DATASTRUCTURE_CONNECTIVITYSET_H_
