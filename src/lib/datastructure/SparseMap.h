/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

/*
 * Sparse map based on sparse set representation of
 * Briggs, Preston, and Linda Torczon. "An efficient representation for sparse sets."
 * ACM Letters on Programming Languages and Systems (LOPLAS) 2.1-4 (1993): 59-69.
 */

#ifndef SRC_LIB_DATASTRUCTURE_SPARSEMAP_H_
#define SRC_LIB_DATASTRUCTURE_SPARSEMAP_H_

#include <memory>
#include <utility>
#include <vector>

#include "lib/core/Mandatory.h"

namespace datastructure {
template <typename Key = Mandatory,
          typename Value = Mandatory >
class SparseMap {
 private:
  struct MapElement {
    const Key key;
    Value value;

    MapElement(Key k, Value val) :
        key(k),
        value(val) {}
  };

 public:
  explicit SparseMap(Key universe_size) :
    _dense(),
    _sparse(std::make_unique<size_t[]>(universe_size)) { }

  SparseMap(const SparseMap&) = delete;
  SparseMap& operator= (const SparseMap&) = delete;

  SparseMap(SparseMap&&) = default;
  SparseMap& operator= (SparseMap&&) = default;

  bool contains(const Key key) const {
    const size_t index = _sparse[key];
    return index < _dense.size() && _dense[index].key == key;
  }

  Value& operator[] (const Key key) {
    const size_t index = _sparse[key];
    const size_t n = _dense.size();
    if (index >= n || _dense[index].key != key) {
      _sparse[key] = _dense.size();
      _dense.emplace_back(key, 0);
      return _dense.back().value;
    }
    return _dense[index].value;
  }

  void add(const Key key, const Value value) {
    const size_t index = _sparse[key];
    const size_t n = _dense.size();
    if (index >= n || _dense[index].key != key) {
      _sparse[key] = _dense.size();
      _dense.emplace_back(key,value);
    }
  }

  void remove(const Key key) {
    const size_t index = _sparse[key];
    if (index <= _dense.size() - 1 && _dense[index].key == key) {
      const MapElement e = _dense.back();
      _dense[index] = e;
      _sparse[e] = index;
      _dense.pop_back();
    }
  }

  size_t size() const {
    return _dense.size();
  }

  auto begin() const {
    return _dense.cbegin();
  }

  auto end() const {
    return _dense.cend();
  }

  auto crbegin() const {
    return _dense.crbegin();
  }

  auto crend() const {
    return _dense.crend();
  }

  void clear() {
    _dense.clear();
  }

 private:
  std::vector<MapElement> _dense;
  std::unique_ptr<size_t[]> _sparse;
};
}  // namespace datastructure
#endif  // SRC_LIB_DATASTRUCTURE_SPARSEMAP_H_
