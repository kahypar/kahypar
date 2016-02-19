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
    _dense(),
    _sparse(std::make_unique<T[]>(universe_size)) { }

  SparseSet(const SparseSet&) = delete;
  SparseSet& operator= (const SparseSet&) = delete;

  SparseSet(SparseSet&&) = default;
  SparseSet& operator= (SparseSet&&) = default;

  bool contains(const T value) const {
    const T index = _sparse[value];
    return index < _dense.size() && _dense[index] == value;
  }

  void add(const T value) {
    const T index = _sparse[value];
    const size_t n = _dense.size();
    if (index >= n || _dense[index] != value) {
      _sparse[value] = _dense.size();
      _dense.push_back(value);
    }
  }

  void remove(const T value) {
    const T index = _sparse[value];
    if (index <= _dense.size() - 1 && _dense[index] == value) {
      const T e = _dense.back();
      _dense[index] = e;
      _sparse[e] = index;
      _dense.pop_back();
    }
  }

  size_t size() const {
    return _dense.size();
  }

  const auto begin() const {
    return _dense.cbegin();
  }

  const auto end() const {
    return _dense.cend();
  }

  void clear() {
    _dense.clear();
  }

 private:
  std::vector<T> _dense;
  std::unique_ptr<T[]> _sparse;
};
}  // namespace datastructure
#endif  // SRC_LIB_DATASTRUCTURE_SPARSESET_H_
