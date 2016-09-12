/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <limits>
#include <memory>
#include <vector>

#include "meta/mandatory.h"

namespace partition {
template <typename T = Mandatory>
class GainCache {
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
  };

 public:
  explicit GainCache(const size_t n) :
    _size(n),
    _cache(std::make_unique<CacheElement[]>(n)),
    _used_delta_entries() {
    _used_delta_entries.reserve(n);
  }

  GainCache(const GainCache&) = delete;
  GainCache& operator= (const GainCache&) = delete;

  GainCache(GainCache&&) = default;
  GainCache& operator= (GainCache&&) = default;

  T delta(const size_t n) const {
    return _cache[n].delta;
  }

  T value(const size_t n) const {
    return _cache[n].value;
  }

  size_t size() const {
    return _size;
  }

  void setDelta(const size_t n, const T value) {
    if (_cache[n].delta == 0) {
      _used_delta_entries.push_back(n);
    }
    _cache[n].delta = value;
  }

  void setNotCached(const size_t n) {
    _cache[n].value = kNotCached;
  }

  bool isCached(const size_t n) {
    return _cache[n].value != kNotCached;
  }

  void setValue(const size_t n, const T value) {
    _cache[n].value = value;
  }

  void updateValue(const size_t n, const T value) {
    _cache[n].value += value;
  }

  void uncheckedSetDelta(const size_t n, const T value) {
    ASSERT(_cache[n].delta != 0,
           "Index " << n << " is still unused and not tracked for reset!");
    _cache[n].delta = value;
  }

  void updateCacheAndDelta(const size_t n, const T delta) {
    if (_cache[n].delta == 0) {
      _used_delta_entries.push_back(n);
    }
    _cache[n].value += delta;
    _cache[n].delta -= delta;
  }

  template <typename use_pqs, class PQ, class Hypergraph>
  void rollbackDelta(PQ& rb_pq, Hypergraph& hg) {
    for (auto rit = _used_delta_entries.crbegin(); rit != _used_delta_entries.crend(); ++rit) {
      if (_cache[*rit].value != kNotCached) {
        _cache[*rit].value += _cache[*rit].delta;
      }
      if (use_pqs()) {
        rb_pq[1 - hg.partID(*rit)].updateKeyBy(*rit, _cache[*rit].delta);
      }
      _cache[*rit].delta = 0;
    }
    _used_delta_entries.clear();
  }

  void resetDelta() {
    for (const size_t used_entry : _used_delta_entries) {
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
}  // namespace partition
