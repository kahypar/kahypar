/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#ifndef SRC_LIB_DATASTRUCTURE_FASTRESETBITVECTOR_H_
#define SRC_LIB_DATASTRUCTURE_FASTRESETBITVECTOR_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

// based on http://upcoder.com/9/fast-resettable-flag-vector/

using std::size_t;
using std::uint16_t;

namespace datastructure {
template <typename UnderlyingType = std::uint16_t>
class FastResetBitVector {
 public:
  FastResetBitVector(const size_t size, const bool initialiser) :
    _v(size, initialiser ? 1 : 0),
    _threshold(1) { }

  FastResetBitVector() :
    _v(),
    _threshold(1) { }

  FastResetBitVector(const FastResetBitVector&) = delete;
  FastResetBitVector& operator= (const FastResetBitVector&) = delete;

  FastResetBitVector(FastResetBitVector&&) = default;
  FastResetBitVector& operator= (FastResetBitVector&&) = default;

  void swap(FastResetBitVector& other) noexcept {
    using std::swap;
    swap(_v, other._v);
    swap(_threshold, other._threshold);
  }

  bool operator[] (const size_t i) const {
    return isSet(i);
  }

  void setBit(const size_t i, const bool value) {
    _v[i] = value ? _threshold : 0;
  }

  void resetAllBitsToFalse() {
    if (_threshold == std::numeric_limits<UnderlyingType>::max()) {
      for (size_t i = 0; i != _v.size(); ++i) {
        _v[i] = 0;
      }
      _threshold = 0;
    }
    ++_threshold;
  }

  void resize(const size_t size, const bool initialiser = false) {
    _v.resize(size, initialiser);
  }

 private:
  bool isSet(size_t i) const {
    return _v[i] == _threshold;
  }

  std::vector<UnderlyingType> _v;
  UnderlyingType _threshold;
};

template <typename UnderlyingType>
void swap(FastResetBitVector<UnderlyingType>& a,
          FastResetBitVector<UnderlyingType>& b) noexcept {
  a.swap(b);
}
}  // namespace datastructure
#endif  // SRC_LIB_DATASTRUCTURE_FASTRESETBITVECTOR_H_
