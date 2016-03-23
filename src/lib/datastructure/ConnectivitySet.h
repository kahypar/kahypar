/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_CONNECTIVITYSET_H_
#define SRC_LIB_DATASTRUCTURE_CONNECTIVITYSET_H_

#include <limits>
#include <memory>
#include <utility>

#include "lib/macros.h"

namespace datastructure {
template <typename T,
          bool allow_duplicate_ops = false>
class ConnectivitySet {
 public:
  explicit ConnectivitySet(const T k) :
    _size(0),
    _sparse(nullptr),
    _dense(nullptr) {
    T* raw = static_cast<T*>(malloc(((2 * k)) * sizeof(T)));
    for (T i = 0; i < 2 * k; ++i) {
      new(raw + i)T(std::numeric_limits<T>::max());
    }
    _sparse = raw;
    _dense = raw + k;
  }

  ~ConnectivitySet() {
    free(_sparse);
  }

  ConnectivitySet(const ConnectivitySet&) = delete;
  ConnectivitySet& operator= (const ConnectivitySet&) = delete;

  ConnectivitySet(ConnectivitySet&& other) :
    _size(other._size),
    _sparse(other._sparse),
    _dense(other._dense) {
    other._size = 0;
    other._sparse = nullptr;
    other._dense = nullptr;
  }

  ConnectivitySet& operator= (ConnectivitySet&&) = delete;

  bool contains(const T value) const {
    const T index = _sparse[value];
    return index < _size && _dense[index] == value;
  }

  void add(const T value) {
    const T index = _sparse[value];
    if (!allow_duplicate_ops || (index >= _size || _dense[index] != value)) {
      ASSERT(index >= _size || _dense[index] != value, V(value));
      _sparse[value] = _size;
      _dense[_size++] = value;
    }
  }

  void remove(const T value) {
    const T index = _sparse[value];
    if (!allow_duplicate_ops || (index < _size && _dense[index] == value)) {
      ASSERT(index < _size && _dense[index] == value, V(value));
      const T e = _dense[--_size];
      _dense[index] = e;
      _sparse[e] = index;
    }
  }

  T size() const {
    return _size;
  }

  T* begin() const {
    return _dense;
  }

  T* end() const {
    return _dense + _size;
  }

  void clear() {
    _size = 0;
  }

 private:
  T _size;
  T* _sparse;
  T* _dense;
};
}  // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_CONNECTIVITYSET_H_
