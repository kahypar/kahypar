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
#include <memory>
#include <utility>

#include "kahypar/macros.h"
#include "kahypar/utils/math.h"

namespace kahypar {
namespace ds {
template <typename T,
          T empty = std::numeric_limits<T>::max(),
          T deleted = std::numeric_limits<T>::max() - 1>
class IncidenceSet {
 private:
  using Element = std::pair<T, T>;
  using Position = T;
  using Byte = char;

  static constexpr bool debug = false;

 public:
  explicit IncidenceSet(const T max_size) :
    _begin(nullptr),
    _end(nullptr),
    _size(0),
    _capacity(max_size),
    _hash_table(nullptr),
    _bitmask(0) {
    const size_t hash_table_size = math::nextPowerOfTwoCeiled(max_size) << 1;
    _bitmask = hash_table_size - 1;

    DBG << V(_capacity) << V(hash_table_size);

    static_assert(std::is_pod<T>::value, "T is not a POD");
    _begin = std::make_unique<T[]>(_capacity +  /*sentinel for peek */ 1 + 2 * hash_table_size);
    _end = _begin.get();

    // initialize iteration area [begin, begin + max_size)
    for (T i = 0; i < max_size; ++i) {
      new(_begin.get() + i)T(std::numeric_limits<T>::max());
    }
    // sentinel for peek() operation of incidence set
    new(_begin.get() + max_size)T(std::numeric_limits<T>::max());

    // hash table comes afterwards
    _hash_table = reinterpret_cast<Element*>(_begin.get() + max_size + 1);
    for (T i = 0; i < hash_table_size; ++i) {
      new (_hash_table + i)Element(empty, empty);
    }
  }

  IncidenceSet() :
    IncidenceSet(10) { }

  ~IncidenceSet() = default;

  IncidenceSet(const IncidenceSet& other) = delete;
  IncidenceSet& operator= (const IncidenceSet&) = delete;

  IncidenceSet& operator= (IncidenceSet&& other) = default;
  IncidenceSet(IncidenceSet&& other) = default;

  void add(const T element) {
    DBG << V(element) << V(_size) << V(_capacity);
    ASSERT(!contains(element), V(element));
    insert(element, _size);
    handleAdd(element);
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
    Element& sparse_v = _hash_table[find(v)];
    swap(_begin[sparse_v.second], *(_end - 1));
    update(_begin[sparse_v.second], sparse_v.second);

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
    Element& sparse_v = _hash_table[find(v)];
    swap(_begin[sparse_v.second], *(_end - 1));
    update(_begin[sparse_v.second], sparse_v.second);

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
    Element& sparse_u = _hash_table[find(u)];
    sparse_u.first = deleted;

    // replace u with v in dense
    insert(v, sparse_u.second);
    _begin[sparse_u.second] = v;
  }

  bool contains(const T key) {
    const Position start_position = startPosition(key);
    const T occupying_key = _hash_table[start_position].first;
    if (occupying_key == key) {
      return true;
    } else {
      // We use the element key at start_position as sentinel for the
      // circular linear probing search. The sentinel is necessary,
      // because we cannot ensure that there always is at least one
      // empty slot. All 'unused' slots can potentially be marked as deleted.
      _hash_table[start_position].first = key;
      for (Position position = nextSlot(start_position); ; position = nextSlot(position)) {
        if (_hash_table[position].first == key) {
          _hash_table[start_position].first = occupying_key;
          if (position != start_position) {
            return true;
          } else {
            return false;
          }
        }
        if (_hash_table[position].first == empty) {
          _hash_table[start_position].first = occupying_key;
          return false;
        }
      }
    }
    return false;
  }

  T size() const {
    return _size;
  }

  const T* begin() const {
    return _begin.get();
  }

  const T* end() const {
    return _end;
  }

  T capacity() const {
    return _capacity;
  }

  void printAll() const {
    for (auto it = begin(); it != end(); ++it) {
      LOG << V(*it);
    }
  }

 private:
  void handleAdd(const T element) {
    *_end = element;
    ++_end;
    if (++_size == _capacity) {
      resize();
    }
  }

  void insert(const T key, const T value) {
    ASSERT(!contains(key), V(key));
    _hash_table[nextFreeSlot(key)] = { key, value };
  }

  void update(const T key, const T value) {
    ASSERT(contains(key), V(key));
    ASSERT(_hash_table[find(key)].first == key, V(key));
    _hash_table[find(key)].second = value;
  }

  Position find(const T key) {
    ASSERT(contains(key), V(key));
    Position position = startPosition(key);
    for ( ; ; position = nextSlot(position)) {
      if (_hash_table[position].first == key) {
        return position;
      }
    }
    return -1;
  }

  Position nextFreeSlot(const T key) const {
    Position position = startPosition(key);
    for ( ; ; ) {
      if (_hash_table[position].first >= deleted) {
        ASSERT(_hash_table[position].first == empty || _hash_table[position].first == deleted,
               V(position));
        return position;
      }
      position = nextSlot(position);
    }
    return -1;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE Position nextSlot(const Position position) const {
    return (position + 1) & _bitmask;
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE Position startPosition(const T key) const {
    return math::identity(key) & _bitmask;
  }

  void resize() {
    DBG << "resizing to capacity: " << (capacity() << 1);
    IncidenceSet new_set(capacity() << 1);
    for (const auto& e : *this) {
      new_set.add(e);
    }
    *this = std::move(new_set);
  }

  std::unique_ptr<T[]> _begin;
  T* _end;
  T _size;
  T _capacity;
  Element* _hash_table;
  T _bitmask;
};
}  // namespace ds
}  // namespace kahypar
