/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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

#include <vector>

#include "kahypar/macros.h"
#include "kahypar/definitions.h"

// based on http://upcoder.com/9/fast-resettable-flag-vector/

namespace kahypar {
namespace ds {
  
class UnionFind {
 private:
  static constexpr HypernodeID MAX_NUM = std::numeric_limits<HypernodeID>::max();

 public:
  explicit UnionFind(const size_t size) :
    _size(size),
    _uf(size, MAX_NUM - 1) { }

  UnionFind(const UnionFind&) = delete;
  UnionFind& operator= (const UnionFind&) = delete;

  UnionFind(UnionFind&&) = default;
  UnionFind& operator= (UnionFind&&) = default;

  ~UnionFind() = default;

  HypernodeID find(const HypernodeID u) {
    if ( isRoot(u) ) {
      // Case, if u is the current root of the set
      return u;
    } 
    // Path compression
    _uf[u] = find(_uf[u]);
    return _uf[u];
  }

  HypernodeID link(const HypernodeID u, const HypernodeID v) {
    ASSERT(u < _size && v < _size, "Elements are not contained in union find");
    const HypernodeID rep_u = find(u);
    const HypernodeID rep_v = find(v);
    if ( rep_u != rep_v ) {
      // Case, if both nodes are not in the same set
      // => Union by size, bigger tree is made parent of smaller
      int size_u = size(rep_u);
      int size_v = size(rep_v);
      if ( size_u >= size_v ) {
        _uf[rep_v] = rep_u;
        _uf[rep_u] = encodeSize(size_u + size_v);
        ASSERT(_uf[rep_u] >= _size, "Size of set to large");
        return rep_u;
      } else {
        _uf[rep_u] = rep_v;
        _uf[rep_v] = encodeSize(size_u + size_v);
        ASSERT(_uf[rep_v] >= _size, "Size of set to large");
        return rep_v;
      }
    }
    return rep_u;
  }

  void reset() {
    for ( size_t i = 0; i < _size; ++i ) {
      _uf[i] = MAX_NUM - 1;
    }
  }

 private:
  inline bool isRoot(const HypernodeID u) {
    return _uf[u] >= _size;
  }

  inline HypernodeID size(const HypernodeID u) {
    ASSERT(isRoot(u), "Representative " << u << " is not the root of its set");
    return MAX_NUM - _uf[u];
  }

  inline HypernodeID encodeSize(const HypernodeID size) {
    return MAX_NUM - size;
  }

  size_t _size;
  std::vector<HypernodeID>  _uf;
};

}  // namespace ds
}  // namespace kahypar
