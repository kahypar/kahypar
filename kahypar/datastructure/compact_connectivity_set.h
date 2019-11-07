/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <utility>
#include <vector>

#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/utils/bit_magic.h"

namespace kahypar {
namespace ds {
class CompactConnectivitySet {
  static constexpr bool debug = false;

  using Bitset = uint8_t;
  using PartitionID = int32_t;

 public:
  class CompactConnectivitySetIterator :
    public std::iterator<std::forward_iterator_tag,  // iterator_category
                         PartitionID,                // value_type
                         std::ptrdiff_t,             // difference_type
                         const PartitionID*,         // pointer
                         PartitionID>{               // reference
 public:
    CompactConnectivitySetIterator() = delete;

    CompactConnectivitySetIterator(const CompactConnectivitySetIterator& other) = default;
    CompactConnectivitySetIterator& operator= (const CompactConnectivitySetIterator& other) = delete;

    CompactConnectivitySetIterator(CompactConnectivitySetIterator&& other) = default;
    CompactConnectivitySetIterator& operator= (CompactConnectivitySetIterator&& other) = delete;

    ~CompactConnectivitySetIterator() = default;


    explicit CompactConnectivitySetIterator(const std::vector<Bitset>& bitset) :
      _bitset(bitset),
      _current_id(-1),
      _current_bitset_id(0),
      _current_bitset(_bitset[0]) {
      next();
    }

    CompactConnectivitySetIterator(const std::vector<Bitset>& bitset,
                                   const PartitionID current_bitset_id,
                                   const uint8_t current_bitset) :
      _bitset(bitset),
      _current_id(-1),
      _current_bitset_id(current_bitset_id),
      _current_bitset(current_bitset) { }

    // ! Returns the id of the element the iterator currently points to.
    PartitionID operator* () const {
      ASSERT(_current_id < utils::count[_current_bitset]);
      return 8 * _current_bitset_id + utils::select[_current_bitset][_current_id + 1];
    }

    // ! Prefix increment. The iterator advances to the next valid element.
    CompactConnectivitySetIterator& operator++ () {
      next();
      return *this;
    }

    // ! Postfix increment. The iterator advances to the next valid element.
    CompactConnectivitySetIterator operator++ (int) {
      CompactConnectivitySetIterator copy = *this;
      operator++ ();
      return copy;
    }

    bool operator!= (const CompactConnectivitySetIterator& rhs) {
      return _current_bitset_id != rhs._current_bitset_id ||
             _current_bitset != rhs._current_bitset;
    }

    bool operator== (const CompactConnectivitySetIterator& rhs) {
      return _current_bitset_id == rhs._current_bitset_id &&
             _current_bitset == rhs._current_bitset;
    }

 private:
    void next() {
      ++_current_id;
      while (_current_id + 1 > utils::count[_current_bitset]) {
        _current_id = 0;
        ++_current_bitset_id;
        if (_current_bitset_id < static_cast<PartitionID>(_bitset.size())) {
          _current_bitset = _bitset[_current_bitset_id];
        } else {
          _current_bitset = 0;
          _current_bitset_id = _bitset.size();
          break;
        }
      }
    }

    const std::vector<Bitset>& _bitset;
    PartitionID _current_id;
    PartitionID _current_bitset_id;
    Bitset _current_bitset;
  };

  explicit CompactConnectivitySet(const PartitionID k) :
    _connectivity(0),
    _bitset() {
    size_t num_entries = k / 8 + (k % 8 > 0 ? 1 : 0);
    _bitset.assign(num_entries, Bitset(0));
  }

  CompactConnectivitySet(const CompactConnectivitySet&) = default;
  CompactConnectivitySet& operator= (const CompactConnectivitySet&) = delete;

  CompactConnectivitySet(CompactConnectivitySet&& other) = default;
  CompactConnectivitySet& operator= (CompactConnectivitySet&&) = default;

  ~CompactConnectivitySet() = default;

  bool contains(const PartitionID id) const {
    ASSERT(id >= 0 && id < ((PartitionID)(8 * _bitset.size())));
    const size_t entry = id / 8;
    const size_t offset = (id % 8) + 1;
    return _bitset[entry] & utils::bitmask[offset];
  }

  void add(const PartitionID id) {
    ASSERT(id >= 0 && id < ((PartitionID)(8 * _bitset.size())));
    ++_connectivity;
    XOR(id);
  }

  void remove(const PartitionID id) {
    ASSERT(id >= 0 && id < ((PartitionID)(8 * _bitset.size())));
    --_connectivity;
    XOR(id);
  }

  const CompactConnectivitySetIterator begin()  const {
    return CompactConnectivitySetIterator(_bitset);
  }

  const CompactConnectivitySetIterator end() const {
    return CompactConnectivitySetIterator(_bitset, _bitset.size(), 0);
  }

  void clear() {
    _connectivity = 0;
    for (size_t i = 0; i < _bitset.size(); ++i) {
      _bitset[i] &= utils::bitmask[0];
    }
  }

  PartitionID size() const {
    return _connectivity;
  }

 private:
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void XOR(const PartitionID id) {
    ASSERT(id >= 0 && id < ((PartitionID)(8 * _bitset.size())));
    const size_t entry = id / 8;
    const size_t offset = (id % 8) + 1;
    _bitset[entry] ^= utils::bitmask[offset];
  }

  PartitionID _connectivity;
  std::vector<Bitset> _bitset;
};
}  // namespace ds
}  // namespace kahypar
