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
#include <vector>

#include "kahypar/meta/mandatory.h"

namespace kahypar {
template <typename T = Mandatory>
class TwoWayFMGainCache {
 private:
  static constexpr T kNotCached = std::numeric_limits<T>::max();

  struct CacheElement {
    T value;
    T delta;

    CacheElement() :
      value(kNotCached),
      delta(0) { }

    CacheElement(const CacheElement&) = delete;
    CacheElement& operator= (const CacheElement&) = delete;

    CacheElement(CacheElement&&) = default;
    CacheElement& operator= (CacheElement&&) = default;

    ~CacheElement() = default;
  };

 public:
  explicit TwoWayFMGainCache(const size_t size) :
    _size(size),
    _cache(std::make_unique<CacheElement[]>(size)),
    _used_delta_entries() {
    _used_delta_entries.reserve(size);
  }

  TwoWayFMGainCache(const TwoWayFMGainCache&) = delete;
  TwoWayFMGainCache& operator= (const TwoWayFMGainCache&) = delete;

  TwoWayFMGainCache(TwoWayFMGainCache&&) = default;
  TwoWayFMGainCache& operator= (TwoWayFMGainCache&&) = default;

  ~TwoWayFMGainCache() = default;

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE T delta(const size_t index) const {
    ASSERT(index < _size);
    return _cache[index].delta;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE T value(const size_t index) const {
    ASSERT(index < _size);
    return _cache[index].value;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE size_t size() const {
    return _size;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void setDelta(const size_t index, const T value) {
    ASSERT(index < _size);
    if (_cache[index].delta == 0) {
      _used_delta_entries.push_back(index);
    }
    _cache[index].delta = value;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void setNotCached(const size_t index) {
    ASSERT(index < _size);
    _cache[index].value = kNotCached;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE bool isCached(const size_t index) {
    ASSERT(index < _size);
    return _cache[index].value != kNotCached;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void setValue(const size_t index, const T value) {
    ASSERT(index < _size);
    _cache[index].value = value;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void updateValue(const size_t index, const T value) {
    ASSERT(index < _size);
    _cache[index].value += value;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void uncheckedSetDelta(const size_t index, const T value) {
    ASSERT(index < _size);
    ASSERT(_cache[index].delta != 0,
           "Index" << index << "is still unused and not tracked for reset!");
    _cache[index].delta = value;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void updateCacheAndDelta(const size_t index, const T delta) {
    ASSERT(index < _size);
    if (_cache[index].delta == 0) {
      _used_delta_entries.push_back(index);
    }
    _cache[index].value += delta;
    _cache[index].delta -= delta;
  }

  void rollbackDelta() {
    for (auto rit = _used_delta_entries.crbegin(); rit != _used_delta_entries.crend(); ++rit) {
      if (_cache[*rit].value != kNotCached) {
        _cache[*rit].value += _cache[*rit].delta;
      }
      _cache[*rit].delta = 0;
    }
    _used_delta_entries.clear();
  }

  void resetDelta() {
    for (const size_t& used_entry : _used_delta_entries) {
      _cache[used_entry].delta = 0;
    }
    _used_delta_entries.clear();
  }

  void clear() {
    for (size_t i = 0; i < _size; ++i) {
      _cache[i] = CacheElement();
    }
  }

 private:
  const size_t _size;
  std::unique_ptr<CacheElement[]> _cache;
  std::vector<size_t> _used_delta_entries;
};
}  // namespace kahypar
