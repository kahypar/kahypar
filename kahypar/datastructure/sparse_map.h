/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
******************************************************************************/
/*
 * Sparse map based on sparse set representation of
 * Briggs, Preston, and Linda Torczon. "An efficient representation for sparse sets."
 * ACM Letters on Programming Languages and Systems (LOPLAS) 2.1-4 (1993): 59-69.
 */

#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"

namespace kahypar {
namespace ds {
template <typename Key = Mandatory,
          typename Value = Mandatory,
          typename Derived = Mandatory>
class SparseMapBase {
 protected:
  struct MapElement {
    Key key;
    Value value;
  };

 public:
  SparseMapBase(const SparseMapBase&) = delete;
  SparseMapBase& operator= (const SparseMapBase&) = delete;

  SparseMapBase& operator= (SparseMapBase&&) = delete;

  size_t size() const {
    return _size;
  }

  bool contains(const Key key) const {
    return static_cast<const Derived*>(this)->containsImpl(key);
  }

  void add(const Key key, const Value value) {
    static_cast<Derived*>(this)->addImpl(key, value);
  }

  const MapElement* begin() const {
    return _dense;
  }

  const MapElement* end() const {
    return _dense + _size;
  }

  MapElement* begin() {
    return _dense;
  }

  MapElement* end() {
    return _dense + _size;
  }


  void clear() {
    static_cast<Derived*>(this)->clearImpl();
  }

  Value& operator[] (const Key key) {
    const size_t index = _sparse[key];
    if (!contains(key)) {
      _dense[_size] = MapElement { key, Value() };
      _sparse[key] = _size++;
      return _dense[_size - 1].value;
    }
    return _dense[index].value;
  }

  const Value & get(const Key key) const {
    ASSERT(contains(key), V(key));
    return _dense[_sparse[key]].value;
  }

 protected:
  explicit SparseMapBase(const size_t max_size,
                         const Value initial_value = 0) :
    _size(0),
    _sparse(std::make_unique<size_t[]>((max_size * sizeof(MapElement) +
                                        max_size * sizeof(size_t)) / sizeof(size_t))),
    _dense(nullptr) {
    _dense = reinterpret_cast<MapElement*>(_sparse.get() + max_size);
    for (size_t i = 0; i < max_size; ++i) {
      _sparse[i] = std::numeric_limits<size_t>::max();
      _dense[i] = MapElement { std::numeric_limits<Key>::max(), initial_value };
    }
  }

  ~SparseMapBase() = default;

  SparseMapBase(SparseMapBase&& other) :
    _size(other._size),
    _sparse(std::move(other._sparse)),
    _dense(std::move(other._dense)) {
    other._size = 0;
    other._sparse = nullptr;
    other._dense = nullptr;
  }

  size_t _size;
  std::unique_ptr<size_t[]> _sparse;
  MapElement* _dense;
};


template <typename Key = Mandatory,
          typename Value = Mandatory>
class SparseMap final : public SparseMapBase<Key, Value, SparseMap<Key, Value> >{
  using Base = SparseMapBase<Key, Value, SparseMap<Key, Value> >;
  friend Base;

 public:
  explicit SparseMap(const Key max_size,
                     const Value initial_value = 0) :
    Base(max_size, initial_value) { }

  SparseMap(const SparseMap&) = delete;
  SparseMap& operator= (const SparseMap& other) = delete;

  SparseMap(SparseMap&& other) :
    Base(std::move(other)) { }

  SparseMap& operator= (SparseMap&& other) {
    _sparse = std::move(other._sparse);
    _size = 0;
    _dense = std::move(other._dense);
    other._size = 0;
    other._sparse = nullptr;
    other._dense = nullptr;
    return *this;
  }

  ~SparseMap() = default;

  void remove(const Key key) {
    const size_t index = _sparse[key];
    if (index < _size && _dense[index].key == key) {
      std::swap(_dense[index], _dense[_size - 1]);
      _sparse[_dense[index].key] = index;
      --_size;
    }
  }

 private:
  bool containsImpl(const Key key) const {
    const size_t index = _sparse[key];
    return index < _size && _dense[index].key == key;
  }


  void addImpl(const Key key, const Value value) {
    const size_t index = _sparse[key];
    if (index >= _size || _dense[index].key != key) {
      _dense[_size] = { key, value };
      _sparse[key] = _size++;
    }
  }

  void clearImpl() {
    _size = 0;
  }

  using Base::_sparse;
  using Base::_dense;
  using Base::_size;
};
}  // namespace ds
}  // namespace kahypar
