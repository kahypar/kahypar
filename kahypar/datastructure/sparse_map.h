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
#include "kahypar/utils/randomize.h"

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
    _dense(nullptr),
    _initial_value(initial_value) {
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
    _dense(std::move(other._dense)),
    _initial_value(std::move(other._initial_value)) {
    other._size = 0;
    other._sparse = nullptr;
    other._dense = nullptr;
  }

  size_t _size;
  std::unique_ptr<size_t[]> _sparse;
  MapElement* _dense;
  Value _initial_value;
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

template <typename Key = Mandatory,
          typename Value = Mandatory>
class ProbabilisticSparseMap final : public SparseMapBase<Key, Value, ProbabilisticSparseMap<Key, Value> >{
  using Base = SparseMapBase<Key, Value, ProbabilisticSparseMap<Key, Value> >;
  friend Base;
  using MapElement = typename Base::MapElement;

  static constexpr size_t NUM_PRECOMPUTED_COIN_FLIPS = 100;

 public:
  explicit ProbabilisticSparseMap(const Key max_size,
                                const Value initial_value = 0) :
    Base(max_size, initial_value),
    _initial_size(max_size),
    _flip_coin_index(0),
    _precomputed_flip_coin(NUM_PRECOMPUTED_COIN_FLIPS),
    _locked(max_size) { 
    for ( size_t i = 0; i < NUM_PRECOMPUTED_COIN_FLIPS; ++i ) {
      _precomputed_flip_coin[i] = Randomize::instance().flipCoin();
    }
  }

  ProbabilisticSparseMap(const ProbabilisticSparseMap&) = delete;
  ProbabilisticSparseMap& operator= (const ProbabilisticSparseMap& other) = delete;

  ProbabilisticSparseMap(ProbabilisticSparseMap&& other) :
    Base(std::move(other)) { }

  ProbabilisticSparseMap& operator= (ProbabilisticSparseMap&& other) {
    _sparse = std::move(other._sparse);
    _size = 0;
    _dense = std::move(other._dense);
    other._size = 0;
    other._sparse = nullptr;
    other._dense = nullptr;
    return *this;
  }

  ~ProbabilisticSparseMap() = default;

  void update(const Key key, const Value delta) {
    const size_t index = _sparse[key % _initial_size];
    if ( index >= _size ) {
      // Case, where key is not contained in map and entry is
      // not occupied by any other key
      _dense[_size] = { key, delta };
      _sparse[key % _initial_size] = _size++;
      _locked[key % _initial_size] = false;
    } else if ( _dense[index].key == key ) {
      // Case, where key is already contained in map
      _dense[index].value += delta;
    } else if ( !_locked[key % _initial_size] && coinFlip() ) {
      // Case, where another key occupies the entry for current key
      // and coin flip decides to remove that key
      _dense[index] = { key, delta };
      _locked[key % _initial_size] = true;
    }
  }

  void remove(const Key key) {
    const size_t index = _sparse[key % _initial_size];
    if (index < _size && _dense[index].key == key) {
      std::swap(_dense[index], _dense[_size - 1]);
      _sparse[_dense[index].key % _initial_size] = index;
      --_size;
    }
  }

  void remove_last_entry() {
    if ( _size > 0 ) {
      const Key key = _dense[_size - 1].key;
      _sparse[key % _initial_size] = std::numeric_limits<Key>::max();
      _dense[--_size] = { std::numeric_limits<Key>::max(), _initial_value };
    }
  }

 private:

  bool containsImpl(const Key key) const {
    const size_t index = _sparse[key % _initial_size];
    return index < _size && _dense[index].key == key;
  }

  void addImpl(const Key key, const Value value) {
    const size_t index = _sparse[key % _initial_size];
    if ( index >= _size ) {
      // Case, where key is not contained in map and entry is
      // not occupied by any other key
      _dense[_size] = { key, value };
      _sparse[key % _initial_size] = _size++;
      _locked[key % _initial_size] = false;
    } else if ( _dense[index].key == key ) {
      // Case, where key is already contained in map
      _dense[index].value = value;
    } else if ( !_locked[key % _initial_size] && coinFlip() ) {
      // Case, where another key occupies the entry for current key
      // and coin flip decides to remove that key
      _dense[index] = { key, value };
      _locked[key % _initial_size] = true;
    }
  }

  void clearImpl() {
    _size = 0;
  }

  inline bool coinFlip() {
    _flip_coin_index = ( ( _flip_coin_index + 1 ) % NUM_PRECOMPUTED_COIN_FLIPS );
    return _precomputed_flip_coin[_flip_coin_index];
  }

  using Base::_sparse;
  using Base::_dense;
  using Base::_size;
  using Base::_initial_value;

  size_t _initial_size;
  size_t _flip_coin_index;
  std::vector<bool> _precomputed_flip_coin;
  std::vector<bool> _locked;
};
}  // namespace ds
}  // namespace kahypar
