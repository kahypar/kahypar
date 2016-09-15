/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>
#include <cstring>

#include "macros.h"

// based on http://upcoder.com/9/fast-resettable-flag-vector/

namespace datastructure {
template <typename UnderlyingType = std::uint16_t>
class FastResetFlagVector {
 public:
  explicit FastResetFlagVector(const size_t size) :
    _v(std::make_unique<UnderlyingType[]>(size)),
    _threshold(1),
    _size(size) {
    memset(_v.get(), 0, size * sizeof(UnderlyingType));
  }

  FastResetFlagVector() :
    _v(nullptr),
    _threshold(1),
    _size(0) { }

  FastResetFlagVector(const FastResetFlagVector&) = delete;
  FastResetFlagVector& operator= (const FastResetFlagVector&) = delete;

  FastResetFlagVector(FastResetFlagVector&&) = default;
  FastResetFlagVector& operator= (FastResetFlagVector&&) = default;

  void swap(FastResetFlagVector& other) {
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
void swap(FastResetFlagVector<UnderlyingType>& a,
          FastResetFlagVector<UnderlyingType>& b) {
  a.swap(b);
}
}  // namespace datastructure
