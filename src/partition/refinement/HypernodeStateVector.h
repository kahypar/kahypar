/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_HYPERNODESTATEVECTOR_H_
#define SRC_PARTITION_REFINEMENT_HYPERNODESTATEVECTOR_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

#include "lib/macros.h"

using std::size_t;

namespace partition {
template <typename UnderlyingType = std::uint32_t>
class HypernodeStateVector {
 public:
  explicit HypernodeStateVector(const size_t size) :
    _v(std::make_unique<UnderlyingType[]>(size)),
    _threshold_active(1),
    _threshold_marked(2),
    _size(size) {
    memset(_v.get(), 0, size * sizeof(UnderlyingType));
  }

  HypernodeStateVector() :
    _v(nullptr),
    _threshold_active(1),
    _threshold_marked(2),
    _size(0) { }

  HypernodeStateVector(const HypernodeStateVector&) = delete;
  HypernodeStateVector& operator= (const HypernodeStateVector&) = delete;

  HypernodeStateVector(HypernodeStateVector&&) = default;
  HypernodeStateVector& operator= (HypernodeStateVector&&) = default;

  void swap(HypernodeStateVector& other) noexcept {
    using std::swap;
    swap(_v, other._v);
    swap(_threshold_active, other._threshold_active);
    swap(_threshold_marked, other._threshold_marked);
  }

  bool active(const size_t i) const {
    return _v[i] == _threshold_active;
  }

  bool marked(const size_t i) const {
    return _v[i] == _threshold_marked;
  }

  void mark(const size_t i) {
    ASSERT(_v[i] == _threshold_active, V(i));
    _v[i] = _threshold_marked;
  }

  void activate(const size_t i) {
    ASSERT(_v[i] < _threshold_active, V(i));
    _v[i] = _threshold_active;
  }

  void deactivate(const size_t i) {
    ASSERT(_v[i] == _threshold_active, V(i));
    --_v[i];
  }

  void reset() {
    if (_threshold_marked == std::numeric_limits<UnderlyingType>::max()) {
      for (size_t i = 0; i != _size; ++i) {
        _v[i] = 0;
      }
      _threshold_active = -2;
      _threshold_marked = -1;
    }
    _threshold_active += 2;
    _threshold_marked += 2;
  }

  void setSize(const size_t size) {
    ASSERT(_v == nullptr, "Error");
    _v = std::make_unique<UnderlyingType[]>(size);
    _size = size;
    memset(_v.get(), 0, size * sizeof(UnderlyingType));
  }

  std::unique_ptr<UnderlyingType[]> _v;
  UnderlyingType _threshold_active;
  UnderlyingType _threshold_marked;
  size_t _size;
};

template <typename UnderlyingType>
void swap(HypernodeStateVector<UnderlyingType>& a,
          HypernodeStateVector<UnderlyingType>& b) noexcept {
  a.swap(b);
}
}  // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_HYPERNODESTATEVECTOR_H_
