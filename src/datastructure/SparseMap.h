/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

/*
 * Sparse map based on sparse set representation of
 * Briggs, Preston, and Linda Torczon. "An efficient representation for sparse sets."
 * ACM Letters on Programming Languages and Systems (LOPLAS) 2.1-4 (1993): 59-69.
 */

#pragma once

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "meta/Mandatory.h"

namespace datastructure {
template <typename Key = Mandatory,
          typename Value = Mandatory>
class SparseMap {
 private:
  struct MapElement {
    Key key;
    Value value;

    MapElement(Key k, Value val) :
      key(k),
      value(val) { }

    MapElement(MapElement&&) = default;
    MapElement& operator= (MapElement&&) = default;
  };

 public:
  explicit SparseMap(Key universe_size) :
    _dense(),
    _sparse(std::make_unique<size_t[]>(universe_size)),
    _size(0) { }

  SparseMap(const SparseMap&) = delete;
  SparseMap& operator= (const SparseMap&) = delete;

  SparseMap(SparseMap&&) = default;
  SparseMap& operator= (SparseMap&&) = default;

  void swap(SparseMap& other) noexcept {
    using std::swap;
    swap(_dense, other._dense);
    swap(_sparse, other._sparse);
    swap(_size, other._size);
  }

  bool contains(const Key key) const {
    const size_t index = _sparse[key];
    return index < _size && _dense[index].key == key;
  }

  Value& operator[] (const Key key) {
    const size_t index = _sparse[key];
    if (index >= _size || _dense[index].key != key) {
      _sparse[key] = _size++;
      _dense.emplace_back(key, Value());
      return _dense.back().value;
    }
    return _dense[index].value;
  }

  const Value & get(const Key key) const {
    ASSERT(contains(key), V(key));
    return _dense[_sparse[key]].value;
  }

  void add(const Key key, const Value value) {
    const size_t index = _sparse[key];
    if (index >= _size || _dense[index].key != key) {
      _sparse[key] = _size++;
      _dense.emplace_back(key, value);
    }
  }

  void remove(const Key key) {
    const size_t index = _sparse[key];
    if (index <= _size - 1 && _dense[index].key == key) {
      std::swap(_dense[index], _dense.back());
      _sparse[_dense[index].key] = index;
      _dense.pop_back();
      --_size;
    }
  }

  size_t size() const {
    return _size;
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
    _size = 0;
  }

 private:
  std::vector<MapElement> _dense;
  std::unique_ptr<size_t[]> _sparse;
  size_t _size;
};

template <typename Key, typename Value>
void swap(SparseMap<Key, Value>& a,
          SparseMap<Key, Value>& b) noexcept {
  a.swap(b);
}
}  // namespace datastructure
