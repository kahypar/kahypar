/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

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
