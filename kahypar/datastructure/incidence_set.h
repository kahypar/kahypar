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
    _memory(nullptr),
    _end(nullptr),
    _size(0),
    _max_size(math::nextPowerOfTwoCeiled(max_size + 1)),
    _max_sparse_size(InitialSizeFactor * _max_size) {
    static_assert(std::is_pod<T>::value, "T is not a POD");
    _memory = static_cast<T*>(malloc(sizeOfDense() + sizeOfSparse()));
    _end = _memory;

    for (T i = 0; i < _max_size; ++i) {
      new(dense() + i)T(std::numeric_limits<T>::max());
    }
    // sentinel for peek() operation of incidence set
    new(dense() + _max_size)T(std::numeric_limits<T>::max());

    for (T i = 0; i < _max_sparse_size; ++i) {
      new (sparse() + i)Element(empty, empty);
    }
  }

  ~IncidenceSet() {
    free(_memory);
  }

  IncidenceSet(const IncidenceSet& other) = delete;
  IncidenceSet& operator= (const IncidenceSet&) = delete;

  IncidenceSet& operator= (IncidenceSet&&) = delete;

  IncidenceSet(IncidenceSet&& other) :
    _memory(other._memory),
    _end(other._end),
    _size(other._size),
    _max_size(other._max_size),
    _max_sparse_size(other._max_sparse_size) {
    other._end = nullptr;
    other._max_size = 0;
    other._size = 0;
    other._memory = nullptr;
  }

  void add(const T element) {
    ASSERT(!contains(element), V(element));
    if (_size == _max_size) {
      resize();
    }

    insert(element, _size);
    dense()[_size] = element;
    ++_size;
    ++_end;
  }

  void insertIfNotContained(const T element) {
    if (!contains(element)) {
      add(element);
    }
  }

  void undoRemoval(const T element) {
    insert(element, _size);
    dense()[_size] = element;
    ++_size;
    ++_end;
  }

  void remove(const T v) {
    using std::swap;
    ASSERT(contains(v), V(v));

    // swap v with element at end
    Element& sparse_v = sparse()[find(v)];
    swap(dense()[sparse_v.second], dense()[_size - 1]);
    update(dense()[sparse_v.second], sparse_v.second);

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
    Element& sparse_v = sparse()[find(v)];
    swap(dense()[sparse_v.second], dense()[_size - 1]);
    update(dense()[sparse_v.second], sparse_v.second);

    // delete v
    sparse_v.first = deleted;

    // add u at v's place in dense
    insert(u, _size - 1);
    dense()[_size - 1] = u;
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
    Element& sparse_u = sparse()[find(u)];
    sparse_u.first = deleted;

    // replace u with v in dense
    insert(v, sparse_u.second);
    dense()[sparse_u.second] = v;
  }

  void swap(IncidenceSet& other) {
    using std::swap;
    swap(_memory, other._memory);
    swap(_end, other._end);
    swap(_size, other._size);
    swap(_max_size, other._max_size);
  }

  bool contains(const T key) const {
    const Position start_position = math::crc32(key) % _max_sparse_size;
    const Position before = mod(static_cast<uint32_t>(start_position) - 1, _max_sparse_size);
    for (Position position = start_position; position < _max_sparse_size; position = (position + 1) % _max_sparse_size) {
      if (sparse()[position].first == empty) {
        return false;
      } else if (sparse()[position].first == key) {
        return true;
      } else if (position == before) {
        return false;
      }
    }
  }

  T size() const {
    return _size;
  }

  const T* begin() const {
    return dense();
  }

  const T* end() const {
    return _end;
  }

  T capacity() const {
    return _max_size;
  }

  void printAll() const {
    for (auto it = begin(); it != end(); ++it) {
      LOGVAR(*it);
    }
  }

 private:
  void insert(const T key, const T value) {
    ASSERT(!contains(key), V(key));
    sparse()[nextFreeSlot(key)] = { key, value };
  }

  void update(const T key, const T value) {
    ASSERT(contains(key), V(key));
    ASSERT(sparse()[find(key)].first == key, V(key));
    sparse()[find(key)].second = value;
  }


  Position find(const T key) const {
    const Position start_position = math::crc32(key) % _max_sparse_size;
    for (Position position = start_position; position < _max_sparse_size; position = (position + 1) % _max_sparse_size) {
      if (sparse()[position].first == key) {
        return position;
      }
    }
    return std::numeric_limits<Position>::max();
  }

  Position nextFreeSlot(const T key) const {
    const Position start_position = math::crc32(key) % _max_sparse_size;
    for (Position position = start_position; position < _max_sparse_size; position = (position + 1) % _max_sparse_size) {
      if (sparse()[position].first >= deleted) {
        ASSERT(sparse()[position].first == empty || sparse()[position].first == deleted, V(position));
        return position;
      }
    }
    ASSERT(true == false, "This should never happen.");
  }


  constexpr size_t sizeOfDense() const {
    return (_max_size + 1  /*sentinel for peek */) * sizeof(T);
  }

  constexpr size_t sizeOfSparse() const {
    return _max_sparse_size * sizeof(std::pair<T, T>);
  }


  void resize() {
    IncidenceSet new_set(_max_size + 1);
    for (const auto e : * this) {
      new_set.add(e);
    }
    swap(new_set);
  }

  T* dense() {
    return _memory;
  }

  const T* dense() const {
    return _memory;
  }

  Element* sparse() {
    return reinterpret_cast<Element*>(dense() + _max_size + 1);
  }

  const Element* sparse() const {
    return reinterpret_cast<const Element*>(dense() + _max_size + 1);
  }

  int32_t mod(int32_t a, int32_t b) const {
    return (a % b + b) % b;
  }

  T* _memory;
  T* _end;
  T _size;
  T _max_size;
  T _max_sparse_size;
};
}  // namespace ds
}  // namespace kahypar
