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
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "macros.h"
#include "meta/mandatory.h"

namespace datastructure {
template <typename Key = Mandatory,
          typename Value = Mandatory,
          typename Derived = Mandatory>
class SparseMapBase {
 protected:
  struct MapElement {
    Key key;
    Value value;

    MapElement(Key k, Value val) :
      key(k),
      value(val) { }

    MapElement(const MapElement&) = delete;
    MapElement& operator= (const MapElement&) = delete;
    MapElement(MapElement&&) = default;
    MapElement& operator= (MapElement&&) = default;
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

  void clear() {
    static_cast<Derived*>(this)->clearImpl();
  }

  Value& operator[] (const Key key) {
    const size_t index = _sparse[key];
    if (!contains(key)) {
      _dense[_size] = MapElement(key, Value());
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
  explicit SparseMapBase(const size_t max_size) :
    _size(0),
    _sparse(nullptr),
    _dense(nullptr) {
    static_assert(std::alignment_of<MapElement>::value ==
                  std::alignment_of<size_t>::value, "Alignment mismatch");

    char* raw = static_cast<char*>(malloc(max_size * sizeof(MapElement) +
                                          max_size * sizeof(size_t)));

    _sparse = reinterpret_cast<size_t*>(raw);
    for (size_t i = 0; i < max_size; ++i) {
      new(_sparse + i)Value(std::numeric_limits<Value>::max());
    }

    _dense = reinterpret_cast<MapElement*>(_sparse + max_size);
    for (size_t i = 0; i < max_size; ++i) {
      new(_dense + i)MapElement(std::numeric_limits<Key>::max(),
                                std::numeric_limits<Value>::max());
    }
  }

  ~SparseMapBase() {
    free(_sparse);
  }

  SparseMapBase(SparseMapBase&& other) :
    _size(other._size),
    _sparse(other._sparse),
    _dense(other._dense) {
    other._size = 0;
    other._sparse = nullptr;
    other._dense = nullptr;
  }

  size_t _size;
  size_t* _sparse;
  MapElement* _dense;
};


template <typename Key = Mandatory,
          typename Value = Mandatory>
class SparseMap final : public SparseMapBase<Key, Value, SparseMap<Key, Value> >{
  using Base = SparseMapBase<Key, Value, SparseMap<Key, Value> >;
  friend Base;

 public:
  explicit SparseMap(Key max_size) :
    Base(max_size) { }

  SparseMap(const SparseMap&) = delete;
  SparseMap& operator= (const SparseMap&) = delete;

  SparseMap(SparseMap&& other) :
    Base(std::move(other)) { }
  SparseMap& operator= (SparseMap&&) = delete;

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
class InsertOnlySparseMap final : public SparseMapBase<Key, Value, SparseMap<Key, Value> >{
  using Base = SparseMapBase<Key, Value, SparseMap<Key, Value> >;
  friend Base;

 public:
  explicit InsertOnlySparseMap(Key max_size) :
    Base(max_size),
    _threshold(0) { }

  InsertOnlySparseMap(const InsertOnlySparseMap&) = delete;
  InsertOnlySparseMap& operator= (const InsertOnlySparseMap&) = delete;

  InsertOnlySparseMap(InsertOnlySparseMap&& other) :
    Base(std::move(other)) { }
  InsertOnlySparseMap& operator= (InsertOnlySparseMap&&) = delete;

 private:
  bool containsImpl(const Key key) const {
    return _sparse[key] == _threshold;
  }

  void addImpl(const Key key, const Value value) {
    if (!contains(key)) {
      _dense[_size] = { key, value };
      _sparse[key] = _size++;
    }
  }

  void clearImpl() {
    _size = 0;
    if (_threshold == std::numeric_limits<Value>::max()) {
      for (size_t i = 0; i < _dense - _sparse; ++i) {
        _sparse[i] = std::numeric_limits<Value>::max();
      }
      _threshold = 0;
    }
  }

  size_t _threshold;
  using Base::_sparse;
  using Base::_dense;
  using Base::_size;
};
}  // namespace datastructure
