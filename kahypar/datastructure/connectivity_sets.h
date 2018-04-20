/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <limits>
#include <memory>
#include <utility>

#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"

namespace kahypar {
namespace ds {
template <typename PartitionID = Mandatory,
          typename HyperedgeID = Mandatory>
class ConnectivitySets final {
 private:
  using Byte = char;

 public:
  // Internal structure for connectivity sets.
  // Each contains the size of the connectivity set as  the header
  // and afterwards the _dense and sparse arrays and the partition pin counts.
  // This memory is allocated outside the structure using a memory arena.
  class ConnectivitySet {
 public:
    explicit ConnectivitySet(const PartitionID k) :
      _k(k),
      _size(0) {
      for (PartitionID i = 0; i < _k; ++i) {
        new(&_size + i + 1)PartitionID(std::numeric_limits<PartitionID>::max());
      }
    }

    ConnectivitySet(const ConnectivitySet&) = delete;
    ConnectivitySet& operator= (const ConnectivitySet&) = delete;

    ConnectivitySet(ConnectivitySet&& other) = delete;
    ConnectivitySet& operator= (ConnectivitySet&&) = delete;

    ~ConnectivitySet() = default;

    const PartitionID* begin()  const {
      return &_size + 1;
    }

    const PartitionID* end() const {
      return &_size + 1 + _size;
    }

    bool contains(const PartitionID value) const {
      const PartitionID* start = &_size + 1;
      for (PartitionID i = 0; i < _k; ++i) {
        const PartitionID k = *(start + i);
        if (k == value) {
          return true;
        } else if (k == std::numeric_limits<PartitionID>::max()) {
          return false;
        }
      }
    }

    void add(const PartitionID value) {
      *(&_size + 1 + _size) = value;
      ++_size;
    }

    void remove(const PartitionID value) {
      PartitionID* start = &_size + 1;
      for (PartitionID i = 0; i < _k; ++i) {
        const PartitionID k = *(start + i);
        if (k == value) {
          *(start + i) = *(start + _size - 1);
          *(start + _size - 1) = std::numeric_limits<PartitionID>::max();
          --_size;
        }
      }
    }

    void clear() {
      _size = 0;
      for (PartitionID i = 0; i < _k; ++i) {
        new(&_size + i + 1)PartitionID(std::numeric_limits<PartitionID>::max());
      }
    }

    PartitionID size() const {
      return _size;
    }

 private:
    const PartitionID _k;
    PartitionID _size;
    // After _size are the _dense and _sparse arrays.
  };


  explicit ConnectivitySets(const HyperedgeID num_hyperedges, const PartitionID k) :
    _k(k),
    _connectivity_sets(nullptr) {
    initialize(num_hyperedges, _k);
  }

  ConnectivitySets() :
    _k(0),
    _connectivity_sets(nullptr) { }


  ~ConnectivitySets() = default;

  ConnectivitySets(const ConnectivitySets&) = delete;
  ConnectivitySets& operator= (const ConnectivitySets&) = delete;

  ConnectivitySets(ConnectivitySets&& other) noexcept :
    _k(other._k),
    _connectivity_sets(std::move(other._connectivity_sets)) {
    other._k = 0;
    other._connectivity_sets = nullptr;
  }

  ConnectivitySets& operator= (ConnectivitySets&& other) noexcept {
    _k = other._k;
    _connectivity_sets = std::move(other._connectivity_sets);
    other._k = 0;
    other._connectivity_sets = nullptr;
    return *this;
  }

  void initialize(const HyperedgeID num_hyperedges, const PartitionID k) {
    _k = k;
    _connectivity_sets = std::make_unique<Byte[]>(num_hyperedges * sizeOfConnectivitySet());
    for (HyperedgeID i = 0; i < num_hyperedges; ++i) {
      new(get(i))ConnectivitySet(_k);
    }
  }

  void resize(const HyperedgeID num_hyperedges, const PartitionID k) {
    initialize(num_hyperedges, k);
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
    return reinterpret_cast<ConnectivitySet*>(_connectivity_sets.get() +
                                              he * sizeOfConnectivitySet());
  }

  // To avoid code duplication we implement non-const version in terms of const version
  ConnectivitySet* get(const HyperedgeID he) {
    return const_cast<ConnectivitySet*>(static_cast<const ConnectivitySets&>(*this).get(he));
  }

  constexpr size_t sizeOfConnectivitySet() const {
    return (sizeof(ConnectivitySet) + _k * sizeof(PartitionID));
  }

  PartitionID _k;
  std::unique_ptr<Byte[]> _connectivity_sets;
};
}  // namespace ds
}  // namespace kahypar
