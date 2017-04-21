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

#pragma once

#include <algorithm>
#include <limits>
#include <utility>

#include "kahypar/macros.h"
#include "kahypar/utils/math.h"

namespace kahypar {
namespace ds {
template <typename T, size_t InitialSizeFactor = 2,
          T empty = std::numeric_limits<T>::max(),
          T deleted = std::numeric_limits<T>::max() - 1>
class IncidenceSet {
 private:
  using Element = std::pair<T, T>;
  using Position = T;

 public:
  explicit IncidenceSet(const T max_size) :
    _dense(nullptr),
    _end(nullptr),
    _sparse(nullptr),
    _size(0),
    _max_sparse_size(0) {
    const size_t internal_size = math::nextPowerOfTwoCeiled(max_size + 1);
    _max_sparse_size = InitialSizeFactor * internal_size;

    static_assert(std::is_pod<T>::value, "T is not a POD");
    _dense = static_cast<T*>(malloc(sizeOfDense(internal_size) + sizeOfSparse()));
    _end = _dense;

    for (T i = 0; i < internal_size; ++i) {
      new(_dense + i)T(std::numeric_limits<T>::max());
    }
    // sentinel for peek() operation of incidence set
    new(_dense + internal_size)T(std::numeric_limits<T>::max());

    _sparse = reinterpret_cast<Element*>(_dense + internal_size + 1);
    for (T i = 0; i < _max_sparse_size; ++i) {
      new (_sparse + i)Element(empty, empty);
    }
  }

  IncidenceSet() :
    IncidenceSet(5) { }

  ~IncidenceSet() {
    free(_dense);
  }

  IncidenceSet(const IncidenceSet& other) = delete;
  IncidenceSet& operator= (const IncidenceSet&) = delete;

  IncidenceSet& operator= (IncidenceSet&&) = delete;

  IncidenceSet(IncidenceSet&& other) :
    _dense(other._dense),
    _end(other._end),
    _sparse(other._sparse),
    _size(other._size),
    _max_sparse_size(other._max_sparse_size) {
    other._end = nullptr;
    other._sparse = nullptr;
    other._dense = nullptr;
  }

  void add(const T element) {
    ASSERT(!contains(element), V(element));
    if (_end == reinterpret_cast<T*>(_sparse)) {
      resize();
    }

    insert(element, _size);
    *_end = element;
    ++_end;
    ++_size;
  }

  void insertIfNotContained(const T element) {
    if (!contains(element)) {
      add(element);
    }
  }

  void undoRemoval(const T element) {
    insert(element, _size);
    *_end = element;
    ++_end;
    ++_size;
  }

  void remove(const T v) {
    using std::swap;
    ASSERT(contains(v), V(v));

    // swap v with element at end
    Element& sparse_v = _sparse[find(v)];
    swap(_dense[sparse_v.second], *(_end - 1));
    update(_dense[sparse_v.second], sparse_v.second);

    // delete v
    sparse_v.first = deleted;
    --_end;
    --_size;
  }

  // reuse position of v to store u
  void reuse(const T u, const T v) {
    using std::swap;
    ASSERT(contains(v), V(v));

    // swap v with element at end
    Element& sparse_v = _sparse[find(v)];
    swap(_dense[sparse_v.second], *(_end - 1));
    update(_dense[sparse_v.second], sparse_v.second);

    // delete v
    sparse_v.first = deleted;

    // add u at v's place in dense
    insert(u, _size - 1);
    *(_end - 1) = u;
  }

  T peek() const {
    // This works, because we ensure that 'one past the current size'
    // is an element of dense(). In case dense is full, there is
    // a sentinel in place to ensure correct behavior.
    return *_end;
  }

  void undoReuse(const T u, const T v) {
    ASSERT(contains(u), V(u));

    // remove u
    Element& sparse_u = _sparse[find(u)];
    sparse_u.first = deleted;

    // replace u with v in dense
    insert(v, sparse_u.second);
    _dense[sparse_u.second] = v;
  }

  void swap(IncidenceSet& other) {
    using std::swap;
    swap(_dense, other._dense);
    swap(_end, other._end);
    swap(_sparse, other._sparse);
    swap(_size, other._size);
    swap(_max_sparse_size, other._max_sparse_size);
  }

  bool contains(const T key) {
    const Position start_position = startPosition(key);
    const T occupying_key = _sparse[start_position].first;
    if (occupying_key == key) {
      return true;
    } else {
      // We use the element key at start_position as sentinel for the
      // circular linear probing search. The sentinel is necessary,
      // because we cannot ensure that there always is at least one
      // empty slot. All 'unused' slots can potentially be marked as deleted.
      _sparse[start_position].first = key;
      Position position = nextPosition(start_position);
      for ( ; ; ) {
        if (_sparse[position].first == key) {
          _sparse[start_position].first = occupying_key;
          if (position != start_position) {
            return true;
          } else {
            return false;
          }
        }
        if (_sparse[position].first == empty) {
          _sparse[start_position].first = occupying_key;
          return false;
        }
        position = nextPosition(position);
      }
    }
  }

  T size() const {
    return _size;
  }

  const T* begin() const {
    return _dense;
  }

  const T* end() const {
    return _end;
  }

  T capacity() const {
    return reinterpret_cast<T*>(_sparse) - _dense - 1;
  }

  void printAll() const {
    for (auto it = begin(); it != end(); ++it) {
      LOG << V(*it);
    }
  }

 private:
  void insert(const T key, const T value) {
    ASSERT(!contains(key), V(key));
    _sparse[nextFreeSlot(key)] = { key, value };
  }

  void update(const T key, const T value) {
    ASSERT(contains(key), V(key));
    ASSERT(_sparse[find(key)].first == key, V(key));
    _sparse[find(key)].second = value;
  }


  Position find(const T key) {
    ASSERT(contains(key), V(key));
    Position position = startPosition(key);
    for ( ; ; ) {
      if (_sparse[position].first == key) {
        return position;
      }
      position = nextPosition(position);
    }
  }

  Position nextFreeSlot(const T key) const {
    Position position = startPosition(key);
    for ( ; ; ) {
      if (_sparse[position].first >= deleted) {
        ASSERT(_sparse[position].first == empty || _sparse[position].first == deleted,
               V(position));
        return position;
      }
      position = nextPosition(position);
    }
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE Position nextPosition(const Position position) const {
    return (position + 1) & (_max_sparse_size - 1);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE Position startPosition(const T key) const {
    ASSERT((_max_sparse_size & (_max_sparse_size - 1)) == 0,
           V(_max_sparse_size) << "is no power of 2");
    return math::identity(key) & (_max_sparse_size - 1);
  }

  size_t sizeOfDense(const size_t max_size) const {
    return (max_size + 1  /*sentinel for peek */) * sizeof(T);
  }

  constexpr size_t sizeOfSparse() const {
    return _max_sparse_size * sizeof(Element);
  }


  void resize() {
    IncidenceSet new_set((_max_sparse_size / 2) + 1);
    for (const auto& e : *this) {
      new_set.add(e);
    }
    swap(new_set);
  }

  T* _dense;
  T* _end;
  Element* _sparse;
  T _size;
  T _max_sparse_size;
};
}  // namespace ds
}  // namespace kahypar
