/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <vector>

#include "kahypar/meta/mandatory.h"

namespace kahypar {
namespace ds {
template <typename T = Mandatory>
class FastResetArray {
 public:
  FastResetArray(const size_t size, const T initial_value) :
    _initial_value(initial_value),
    _used_entries(),
    _entries(std::make_unique<T[]>(size)) {
    for (size_t i = 0; i < size; ++i) {
      _entries[i] = initial_value;
    }
    _used_entries.reserve(size);
  }

  FastResetArray(const FastResetArray&) = delete;
  FastResetArray& operator= (const FastResetArray&) = delete;

  FastResetArray(FastResetArray&&) = default;
  FastResetArray& operator= (FastResetArray&&) = default;

  ~FastResetArray() = default;

  const T & get(const size_t index) const {
    return _entries[index];
  }

  void set(const size_t index, const T value) {
    if (_entries[index] == _initial_value) {
      _used_entries.push_back(index);
    }
    _entries[index] = value;
  }

  void uncheckedSet(const size_t index, const T value) {
    ASSERT(_entries[index] != _initial_value);
    _entries[index] = value;
  }

  void update(const size_t index, const T delta) {
    if (_entries[index] == _initial_value) {
      _used_entries.push_back(index);
    }
    _entries[index] += delta;
  }

  void resetUsedEntries() {
    for (auto rit = _used_entries.crbegin(); rit != _used_entries.crend(); ++rit) {
      _entries[*rit] = _initial_value;
    }
    _used_entries.clear();
  }

  template <class Container>
  void resetUsedEntries(Container& container) {
    for (auto rit = _used_entries.crbegin(); rit != _used_entries.crend(); ++rit) {
      if (container[*rit] != std::numeric_limits<typename Container::value_type>::max()) {
        container[*rit] += _entries[*rit];
      }
      _entries[*rit] = _initial_value;
    }
    _used_entries.clear();
  }

  template <bool use_pqs, class Container, class PQ, class Hypergraph>
  void resetUsedEntries(Container& container, PQ& rb_pq, Hypergraph& hg) {
    for (auto rit = _used_entries.crbegin(); rit != _used_entries.crend(); ++rit) {
      if (container[*rit] != std::numeric_limits<typename Container::value_type>::max()) {
        container[*rit] += _entries[*rit];
      }
      if (use_pqs) {
        rb_pq[1 - hg.partID(*rit)].updateKeyBy(*rit, _entries[*rit]);
      }
      _entries[*rit] = _initial_value;
    }
    _used_entries.clear();
  }

  template <class Hypergraph, class GainCache, class RebalancePQs>
  void resetUsedEntries(const Hypergraph& hg, const GainCache& cache, RebalancePQs& pqs) {
    for (auto rit = _used_entries.crbegin(); rit != _used_entries.crend(); ++rit) {
      if (_entries[*rit] != _initial_value) {
        pqs[1 - hg.partID(*rit)].push(*rit, cache[*rit]);
      }
      _entries[*rit] = _initial_value;
    }
    _used_entries.clear();
  }

 private:
  const T _initial_value;
  std::vector<size_t> _used_entries;
  std::unique_ptr<T[]> _entries;
};
}  // namespace ds
}  // namespace kahypar
