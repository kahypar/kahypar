/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <x86intrin.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "lib/macros.h"
#include "lib/utils/Math.h"

namespace datastructure {
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
    _max_size(0),
    _mask(0) {
    static_assert(std::is_pod<T>::value, "T is not a POD");

    allocate(max_size);
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
    _mask(other._mask) {
    other._end = nullptr;
    other._max_size = 0;
    other._size = 0;
    other._memory = nullptr;
    other._mask = 0;
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

  void swap(IncidenceSet& other) noexcept {
    using std::swap;
    swap(_memory, other._memory);
    swap(_end, other._end);
    swap(_size, other._size);
    swap(_max_size, other._max_size);
    swap(_mask, other._mask);
  }

  bool contains(const T key) const {
    const Position start_position = desiredPosition(key);
    const Position before = start_position == 0 ? _mask : start_position - 1;
    for (Position position = start_position; position <= _mask;
         position = (position + 1) & _mask) {
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
    const Position start_position = desiredPosition(key);
    for (Position position = start_position; position <= _mask;
         position = (position + 1) & _mask) {
      if (sparse()[position].first == key) {
        return position;
      }
    }
    return std::numeric_limits<Position>::max();
  }

  Position nextFreeSlot(const T key) const {
    const Position start_position = desiredPosition(key);
    for (Position position = start_position; position <= _mask;
         position = (position + 1) & _mask) {
      if (sparse()[position].first >= deleted) {
        ASSERT(sparse()[position].first == empty ||
               sparse()[position].first == deleted, V(position));
        return position;
      }
    }
    ASSERT(true == false, "This should never happen.");
  }

  Position desiredPosition(const T key) const {
    return utils::crc32(key) & _mask;
  }

  size_t sizeOfDense() const {
    return (_max_size + 1  /*sentinel for peek */) * sizeof(T);
  }

  size_t sizeOfSparse() const {
    return (_mask + 1) * sizeof(std::pair<T, T>);
  }


  void resize() {
    T* old_mem = _memory;
    const T* old_end = _end;

    allocate(_max_size);

    for (const T* elem_ptr = old_mem; elem_ptr < old_end; ++elem_ptr) {
      add(*elem_ptr);
    }

    free(old_mem);
  }

  void allocate(const size_t max_size) {
    _size = 0;
    _max_size = utils::nextPowerOfTwoCeiled(max_size + 1);
    _mask = (InitialSizeFactor * _max_size) - 1;

    _memory = static_cast<T*>(malloc(sizeOfDense() + sizeOfSparse()));
    _end = _memory;

    for (T i = 0; i < _max_size; ++i) {
      new(dense() + i)T(std::numeric_limits<T>::max());
    }
    // sentinel for peek() operation of incidence set
    new(dense() + _max_size)T(std::numeric_limits<T>::max());

    for (T i = 0; i <= _mask; ++i) {
      new (sparse() + i)Element(empty, empty);
    }
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

  std::int32_t mod(std::int32_t a, std::int32_t b) const {
    return (a % b + b) % b;
  }

  T* _memory;
  T* _end;
  T _size;
  T _max_size;
  T _mask;
};
}  // namespace datastructure
