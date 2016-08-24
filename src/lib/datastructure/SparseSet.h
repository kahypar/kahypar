/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

/*
 * Sparse set representation based on
 * Briggs, Preston, and Linda Torczon. "An efficient representation for sparse sets."
 * ACM Letters on Programming Languages and Systems (LOPLAS) 2.1-4 (1993): 59-69.
 */

#ifndef SRC_LIB_DATASTRUCTURE_SPARSESET_H_
#define SRC_LIB_DATASTRUCTURE_SPARSESET_H_

#include <limits>
#include <memory>
#include <utility>

#include "lib/core/Mandatory.h"
#include "lib/macros.h"

namespace datastructure {
template <typename T = Mandatory>
class SparseSet {
 public:
  explicit SparseSet(const T k) :
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

  ~SparseSet() {
    free(_sparse);
  }

  SparseSet(const SparseSet&) = delete;
  SparseSet& operator= (const SparseSet&) = delete;

  SparseSet(SparseSet&& other) :
    _size(other._size),
    _sparse(other._sparse),
    _dense(other._dense) {
    other._size = 0;
    other._sparse = nullptr;
    other._dense = nullptr;
  }

  SparseSet& operator= (SparseSet&&) = delete;

  bool contains(const T value) const {
    const T index = _sparse[value];
    return index < _size && _dense[index] == value;
  }

  void add(const T value) {
    const T index = _sparse[value];
    if (index >= _size || _dense[index] != value) {
      _sparse[value] = _size;
      _dense[_size++] = value;
    }
  }

  void remove(const T value) {
    const T index = _sparse[value];
    if (index < _size && _dense[index] == value) {
      const T e = _dense[--_size];
      _dense[index] = e;
      _sparse[e] = index;
    }
  }

  T size() const {
    return _size;
  }

  const T* begin() const {
    return _dense;
  }

  const T* end() const {
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
#endif  // SRC_LIB_DATASTRUCTURE_SPARSESET_H_
