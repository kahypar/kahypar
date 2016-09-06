/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <limits>
#include <memory>
#include <utility>

#include "lib/macros.h"

namespace datastructure {
template <typename T>
class InsertOnlyConnectivitySet {
 public:
  explicit InsertOnlyConnectivitySet(const T k) :
    _size(0),
    _threshold(1),
    _sparse(nullptr),
    _dense(nullptr) {
    T* raw = static_cast<T*>(malloc(((2 * k)) * sizeof(T)));
    for (T i = 0; i < k; ++i) {
      new(raw + i)T(0);
    }
    for (T i = k; i < 2 * k; ++i) {
      new(raw + i)T(std::numeric_limits<T>::max());
    }
    _sparse = raw;
    _dense = raw + k;
  }

  ~InsertOnlyConnectivitySet() {
    free(_sparse);
  }

  InsertOnlyConnectivitySet(const InsertOnlyConnectivitySet&) = delete;
  InsertOnlyConnectivitySet& operator= (const InsertOnlyConnectivitySet&) = delete;

  InsertOnlyConnectivitySet(InsertOnlyConnectivitySet&& other) :
    _size(other._size),
    _threshold(other._threshold),
    _sparse(other._sparse),
    _dense(other._dense) {
    other._size = 0;
    other._threshold = 1;
    other._sparse = nullptr;
    other._dense = nullptr;
  }

  InsertOnlyConnectivitySet& operator= (InsertOnlyConnectivitySet&&) = delete;

  bool contains(const T value) const {
    return _sparse[value] == _threshold;
  }

  void add(const T value) {
    if (!contains(value)) {
      _sparse[value] = _threshold;
      _dense[_size++] = value;
    }
  }

  T size() const {
    return _size;
  }

  auto begin() const {
    return _dense;
  }

  auto end() const {
    return _dense + _size;
  }

  void clear() {
    _size = 0;
    if (_threshold == std::numeric_limits<T>::max()) {
      for (T i = 0; i < _dense - _sparse; ++i) {
        _sparse[i] = 0;
      }
      _threshold = 0;
    }
    ++_threshold;
  }

 private:
  T _size;
  T _threshold;
  T* _sparse;
  T* _dense;
};
}  // namespace datastructure
