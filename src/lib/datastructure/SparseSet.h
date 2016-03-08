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

#include <memory>
#include <vector>

#include "lib/core/Mandatory.h"

namespace datastructure {
template <typename T = Mandatory>
class SparseSet {
 public:
  explicit SparseSet(T universe_size) :
    _dense(std::make_unique<T[]>(universe_size)),
    _sparse(std::make_unique<size_t[]>(universe_size)),
    _size(0) { }

  SparseSet(const SparseSet&) = delete;
  SparseSet& operator= (const SparseSet&) = delete;

  SparseSet(SparseSet&&) = default;
  SparseSet& operator= (SparseSet&&) = default;

  bool contains(const T value) const {
    const size_t index = _sparse[value];
    return index < _size && _dense[index] == value;
  }

  void add(const T value) {
    const size_t index = _sparse[value];
    if (index >= _size || _dense[index] != value) {
      _sparse[value] = _size;
      _dense[_size++] = value;
    }
  }

  void remove(const T value) {
    const size_t index = _sparse[value];
    if (index <= _size - 1 && _dense[index] == value) {
      const T e = _dense[_size - 1];
      _dense[index] = e;
      _sparse[e] = index;
      --_size;
    }
  }

  size_t size() const {
    return _size;
  }

  auto begin() const {
    return _dense.get();
  }

  auto end() const {
    return _dense.get() + _size;
  }

  void clear() {
    _size = 0;
  }

 private:
  std::unique_ptr<T[]> _dense;
  std::unique_ptr<size_t[]> _sparse;
  size_t _size;
};
}  // namespace datastructure
#endif  // SRC_LIB_DATASTRUCTURE_SPARSESET_H_
