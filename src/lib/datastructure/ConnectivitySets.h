/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_CONNECTIVITYSETS_H_
#define SRC_LIB_DATASTRUCTURE_CONNECTIVITYSETS_H_

#include <limits>
#include <memory>
#include <utility>

#include "lib/core/Mandatory.h"
#include "lib/macros.h"

namespace datastructure {
<<<<<<< HEAD
template <typename PartitionID = Mandatory,
          typename HyperedgeID = Mandatory,
class ConnectivitySets final {
 private:
  using Byte = char;

 public:
  // Internal structure for connectivity sets.
  // Each contains the size of the connectivity set as  the header
  // and afterwards the _dense and sparse arrays and the partition pin counts.
  // This memory is allocated outside the structure using a memory arena.
  struct ConnectivitySet {
    const PartitionID _k;
    PartitionID _size;
    // After _size are the _dense and _sparse arrays.

    explicit ConnectivitySet(const PartitionID k) :
      _k(k),
      _size(0) {
      for (PartitionID i = 0; i < 2 * _k; ++i) {
        new(&_size + i + 1)PartitionID(std::numeric_limits<PartitionID>::max());
      }
    }

    ConnectivitySet(const ConnectivitySet&) = delete;
    ConnectivitySet& operator= (const ConnectivitySet&) = delete;

    ConnectivitySet(ConnectivitySet&& other) = delete;
    ConnectivitySet& operator= (ConnectivitySet&&) = delete;

    PartitionID* dense() {
      return &_size + 1;
    }

    PartitionID* sparse() {
      return &_size + 1 + _k;
    }

    const PartitionID* begin()  const {
      return &_size + 1;
    }

    const PartitionID* end() const {
      return &_size + 1 + _size;
    }

    bool contains(const PartitionID value) const {
      const PartitionID index = sparse()[value];
      return index < _size && dense()[index] == value;
    }

    void add(const PartitionID value) {
      ASSERT((sparse()[value] >= _size || dense()[sparse()[value]] != value), V(value));
      sparse()[value] = _size;
      dense()[_size++] = value;
    }

    void remove(const PartitionID value) {
      const PartitionID index = sparse()[value];
      ASSERT(index < _size && dense()[index] == value, V(value));
      const PartitionID e = dense()[--_size];
      dense()[index] = e;
      sparse()[e] = index;
    }

    void clear() {
      _size = 0;
    }
  };


  explicit ConnectivitySets(const HyperedgeID num_hyperedges, const PartitionID k) :
    _k(k),
    _connectivity_sets(nullptr) {
    initialize(num_hyperedges, _k);
  }

  ConnectivitySets() :
    _k(0),
    _connectivity_sets(nullptr) { }


  ~ConnectivitySets() {
    // Since ConnectivitySet only contains PartitionIDs and these are PODs,
    // we do not need to call destructors of ConnectivitySet get(i)->~ConnectivitySet();
    static_assert(std::is_pod<PartitionID>::value, "PartitionID is not a POD");
    static_assert(std::is_pod<HypernodeID>::value, "HypernodeID is not a POD");
    free(_connectivity_sets);
  }

  ConnectivitySets(const ConnectivitySets&) = delete;
  ConnectivitySets& operator= (const ConnectivitySets&) = delete;

  ConnectivitySets(ConnectivitySets&& other) :
    _k(other._k),
    _connectivity_sets(other._connectivity_sets) {
    other._k = 0;
    other._connectivity_sets = nullptr;
  }

  ConnectivitySets& operator= (ConnectivitySets&&) = delete;

  void initialize(const HyperedgeID num_hyperedges, const PartitionID k) {
    _k = k;
    _connectivity_sets = static_cast<Byte*>(malloc(num_hyperedges * sizeOfConnectivitySet()));
    for (HyperedgeID i = 0; i < num_hyperedges; ++i) {
      new(get(i))ConnectivitySet(_k);
    }
  }


  const ConnectivitySet& operator[] (const HyperedgeID he) const {
    return *get(he);
  }

  // To avoid code duplication we implement non-const version in terms of const version
  ConnectivitySet& operator[] (const HyperedgeID he) {
    return const_cast<ConnectivitySet&>(static_cast<const ConnectivitySets&>(*this).operator[] (he));
  }

 private:
  const ConnectivitySet* get(const HyperedgeID he) const {
    return reinterpret_cast<ConnectivitySet*>(_connectivity_sets + he * sizeOfConnectivitySet());
  }

  // To avoid code duplication we implement non-const version in terms of const version
  ConnectivitySet* get(const HyperedgeID he) {
    return const_cast<ConnectivitySet*>(static_cast<const ConnectivitySets&>(*this).get(he));
  }

  Byte sizeOfConnectivitySet() const {
    return (sizeof(ConnectivitySet) + 2 * _k * sizeof(PartitionID));
  }

  PartitionID _k;
  Byte* _connectivity_sets;
};
}  // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_CONNECTIVITYSETS_H_
