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
 * Sparse set representation based on
 * Briggs, Preston, and Linda Torczon. "An efficient representation for sparse sets."
 * ACM Letters on Programming Languages and Systems (LOPLAS) 2.1-4 (1993): 59-69.
 */

#pragma once

#include <limits>
#include <memory>
#include <utility>

#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"

namespace kahypar {
namespace ds {
template <typename ValueType = Mandatory,
          typename Derived = Mandatory>
class SparseSetBase {
 public:
  SparseSetBase(const SparseSetBase&) = delete;
  SparseSetBase& operator= (const SparseSetBase&) = delete;

  SparseSetBase& operator= (SparseSetBase&& other) {
    _sparse = std::move(other._sparse);
    _size = other._size;
    _dense = other._dense;

    other._size = 0;
    other._dense = nullptr;
    return *this;
  }

  ValueType size() const {
    return _size;
  }

  bool contains(const ValueType value) const {
    return static_cast<const Derived*>(this)->containsImpl(value);
  }

  void add(const ValueType value) {
    static_cast<Derived*>(this)->addImpl(value);
  }

  const ValueType* begin() const {
    return _dense;
  }

  const ValueType* end() const {
    return _dense + _size;
  }

  void clear() {
    static_cast<Derived*>(this)->clearImpl();
  }

 protected:
  explicit SparseSetBase(const ValueType k) :
    _size(0),
    _sparse(std::make_unique<ValueType[]>(2 * static_cast<size_t>(k))),
    _dense(nullptr) {
    for (ValueType i = 0; i < 2 * k; ++i) {
      _sparse[i] = std::numeric_limits<ValueType>::max();
    }
    _dense = _sparse.get() + k;
  }

  ~SparseSetBase() = default;

  SparseSetBase(SparseSetBase&& other) :
    _size(other._size),
    _sparse(std::move(other._sparse)),
    _dense(other._dense) {
    other._size = 0;
    other._sparse = nullptr;
    other._dense = nullptr;
  }

  ValueType _size;
  std::unique_ptr<ValueType[]> _sparse;
  ValueType* _dense;
};

template <typename ValueType = Mandatory>
class SparseSet final : public SparseSetBase<ValueType, SparseSet<ValueType> >{
  using Base = SparseSetBase<ValueType, SparseSet<ValueType> >;
  friend Base;

 public:
  explicit SparseSet(const ValueType k) :
    Base(k) { }

  SparseSet(const SparseSet&) = delete;

  SparseSet(SparseSet&& other) :
    Base(std::move(other)) { }


  SparseSet& operator= (SparseSet&& other) {
    if (this != &other) {
      Base::operator= (std::move(other));
    }
    return *this;
  }

  SparseSet& operator= (SparseSet&) = delete;

  ~SparseSet() = default;

  void remove(const ValueType value) {
    const ValueType index = _sparse[value];
    if (index < _size && _dense[index] == value) {
      const ValueType e = _dense[--_size];
      _dense[index] = e;
      _sparse[e] = index;
    }
  }

 private:
  bool containsImpl(const ValueType value) const {
    const ValueType index = _sparse[value];
    return index < _size && _dense[index] == value;
  }

  void addImpl(const ValueType value) {
    const ValueType index = _sparse[value];
    if (index >= _size || _dense[index] != value) {
      _sparse[value] = _size;
      _dense[_size++] = value;
    }
  }

  void clearImpl() {
    _size = 0;
  }

  using Base::_sparse;
  using Base::_dense;
  using Base::_size;
};

template <typename ValueType = Mandatory>
class InsertOnlySparseSet final : public SparseSetBase<ValueType,
                                                       InsertOnlySparseSet<ValueType> >{
  using Base = SparseSetBase<ValueType, InsertOnlySparseSet<ValueType> >;
  friend Base;

 public:
  explicit InsertOnlySparseSet(const ValueType k) :
    Base(k),
    _threshold(0) { }

  InsertOnlySparseSet(const InsertOnlySparseSet&) = delete;

  InsertOnlySparseSet(InsertOnlySparseSet&& other) :
    Base(std::move(other)),
    _threshold(other._threshold) {
    other._threshold = 0;
  }

  InsertOnlySparseSet& operator= (InsertOnlySparseSet&& other) {
    if (this != &other) {
      Base::operator= (std::move(other));
      _threshold = other._threshold;

      other._threshold = 0;
    }
    return *this;
  }

  InsertOnlySparseSet& operator= (const InsertOnlySparseSet&) = delete;

  ~InsertOnlySparseSet() = default;

 private:
  FRIEND_TEST(AnInsertOnlySparseSet, HandlesThresholdOverflow);

  bool containsImpl(const ValueType value) const {
    return _sparse[value] == _threshold;
  }

  void addImpl(const ValueType value) {
    if (!containsImpl(value)) {
      _sparse[value] = _threshold;
      _dense[_size++] = value;
    }
  }

  void clearImpl() {
    _size = 0;
    ++_threshold;
    if (_threshold == std::numeric_limits<ValueType>::max()) {
      for (ValueType i = 0; i < _dense - _sparse.get(); ++i) {
        _sparse[i] = std::numeric_limits<ValueType>::max();
      }
      _threshold = 0;
    }
  }

  ValueType _threshold;
  using Base::_size;
  using Base::_sparse;
  using Base::_dense;
};
}  // namespace ds
}  // namespace kahypar
