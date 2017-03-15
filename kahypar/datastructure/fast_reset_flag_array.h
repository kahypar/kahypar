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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

#include "kahypar/macros.h"

// based on http://upcoder.com/9/fast-resettable-flag-vector/

namespace kahypar {
namespace ds {
template <typename UnderlyingType = std::uint16_t>
class FastResetFlagArray {
 public:
  explicit FastResetFlagArray(const size_t size) :
    _v(std::make_unique<UnderlyingType[]>(size)),
    _threshold(1),
    _size(size) {
    memset(_v.get(), 0, size * sizeof(UnderlyingType));
  }

  FastResetFlagArray() :
    _v(nullptr),
    _threshold(1),
    _size(0) { }

  FastResetFlagArray(const FastResetFlagArray&) = delete;
  FastResetFlagArray& operator= (const FastResetFlagArray&) = delete;

  FastResetFlagArray(FastResetFlagArray&&) = default;
  FastResetFlagArray& operator= (FastResetFlagArray&&) = default;

  ~FastResetFlagArray() = default;

  void swap(FastResetFlagArray& other) {
    using std::swap;
    swap(_v, other._v);
    swap(_threshold, other._threshold);
  }

  bool operator[] (const size_t i) const {
    return isSet(i);
  }

  void set(const size_t i, const bool value) {
    _v[i] = value ? _threshold : 0;
  }

  void reset() {
    if (_threshold == std::numeric_limits<UnderlyingType>::max()) {
      for (size_t i = 0; i != _size; ++i) {
        _v[i] = 0;
      }
      _threshold = 0;
    }
    ++_threshold;
  }

  void setSize(const size_t size, const bool initialiser = false) {
    ASSERT(_v == nullptr, "Error");
    _v = std::make_unique<UnderlyingType[]>(size);
    _size = size;
    memset(_v.get(), (initialiser ? 1 : 0), size * sizeof(UnderlyingType));
  }

 private:
  bool isSet(size_t i) const {
    return _v[i] == _threshold;
  }

  std::unique_ptr<UnderlyingType[]> _v;
  UnderlyingType _threshold;
  size_t _size;
};

template <typename UnderlyingType>
void swap(FastResetFlagArray<UnderlyingType>& a,
          FastResetFlagArray<UnderlyingType>& b) {
  a.swap(b);
}
}  // namespace ds
}  // namespace kahypar
