/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_INCIDENCESET_H_
#define SRC_LIB_DATASTRUCTURE_INCIDENCESET_H_

#include <x86intrin.h>

#include <algorithm>
#include <limits>
#include <utility>

#include "lib/macros.h"
#include "lib/utils/Math.h"

namespace datastructure {
template <typename KeyType, typename ValueType, size_t SizeFactor,
          KeyType empty = std::numeric_limits<KeyType>::max(),
          KeyType deleted = std::numeric_limits<KeyType>::max() - 1>
class HashMap {
 public:
  using Element = std::pair<KeyType, ValueType>;

  explicit HashMap(size_t max_size) :
    _max_size(max_size * SizeFactor) {
    static_assert(std::is_pod<KeyType>::value, "KeyType is not a POD");
    static_assert(std::is_pod<ValueType>::value, "ValueType is not a POD");
    for (size_t i = 0; i < _max_size; ++i) {
      new (data() + i)Element(empty, empty);
    }
  }

  HashMap(const HashMap&) = delete;
  HashMap& operator= (const HashMap&) = delete;

  HashMap(HashMap&& other) = default;
  HashMap& operator= (HashMap&&) = default;

  bool contains(const KeyType key) const {
    const Position position = find(key);
    if (position == -1 || data()[position].first == empty) {
      return false;
    }
    return true;
  }

  void insert(const KeyType key, const ValueType value) {
    ASSERT(!contains(key), V(key));
    data()[nextFreeSlot(key)] = { key, value };
  }

  ValueType remove(const KeyType key) {
    ASSERT(contains(key), V(key));
    const Position position = find(key);
    data()[position].first = deleted;
    return data()[position].second;
  }

  void update(const KeyType key, const ValueType value) {
    ASSERT(contains(key), V(key));
    ASSERT(data()[find(key)].first == key, V(key));
    data()[find(key)].second = value;
  }

  const Element & get(const KeyType& key) const {
    return data()[find(key)];
  }

 private:
  using Position = size_t;

  Position find(const KeyType key) const {
    const Position start_position = utils::crc32(key) % _max_size;
    const Position before = start_position != 0 ? start_position - 1 : _max_size - 1;
    for (Position position = start_position; position < _max_size; position = (position + 1) % _max_size) {
      if (data()[position].first == empty || data()[position].first == key) {
        return position;
      } else if (position == before) {
        return -1;
      }
    }
    ASSERT(true == false, "This should never happen.");
  }

  Position nextFreeSlot(const KeyType key) {
    const Position start_position = utils::crc32(key) % _max_size;
    for (Position position = start_position; position < _max_size; position = (position + 1) % _max_size) {
      if (data()[position].first == empty || data()[position].first == deleted) {
        return position;
      }
    }
    ASSERT(true == false, "This should never happen.");
  }

  Element* data() {
    return reinterpret_cast<Element*>(&_max_size + 1);
  }

  const Element* data() const {
    return reinterpret_cast<const Element*>(&_max_size + 1);
  }

  Position _max_size;
};


template <typename T, size_t InitialSizeFactor = 2>
class IncidenceSet {
 private:
  using Sparse = HashMap<T, size_t, InitialSizeFactor>;

 public:
  explicit IncidenceSet(const T max_size) :
    _memory(nullptr),
    _size(0),
    _max_size(utils::nextPowerOfTwoCeiled(max_size + 1)) {
    static_assert(std::is_pod<T>::value, "T is not a POD");
    _memory = static_cast<T*>(malloc(sizeOfDense() + sizeOfSparse()));

    for (size_t i = 0; i < _max_size; ++i) {
      new(dense() + i)T(std::numeric_limits<T>::max());
    }
    // sentinel for peek() operation of incidence set
    new(dense() + _max_size)T(std::numeric_limits<T>::max());

    new(sparse())Sparse(_max_size);
  }

  ~IncidenceSet() {
    sparse()->~Sparse();
    free(_memory);
  }

  IncidenceSet(const IncidenceSet& other) = delete;
  IncidenceSet& operator= (const IncidenceSet&) = delete;

  IncidenceSet& operator= (IncidenceSet&&) = delete;

  IncidenceSet(IncidenceSet&& other) :
    _memory(other._memory),
    _size(other._size),
    _max_size(other._max_size) {
    other._size = 0;
    other._max_size = 0;
    other._memory = nullptr;
  }

  void add(const T element) {
    ASSERT(!contains(element), V(element));
    if (_size == _max_size) {
      resize();
    }

    sparse()->insert(element, _size);
    dense()[_size++] = element;
  }

  void insertIfNotContained(const T element) {
    if (!contains(element)) {
      add(element);
    }
  }

  void remove(const T element) {
    swapToEnd(element);
    removeAtEnd(element);
  }

  void undoRemoval(const T element) {
    sparse()->insert(element, _size);
    dense()[_size++] = element;
  }

  void swapToEnd(const T v) {
    using std::swap;
    const T index_v = sparse()->get(v).second;
    swap(dense()[index_v], dense()[_size - 1]);
    sparse()->update(dense()[index_v], index_v);
    sparse()->update(v, _size - 1);
    ASSERT(sparse()->get(v).second == _size - 1, V(v));
  }

  void removeAtEnd(const T v) {
    ASSERT(dense()[_size - 1] == v, V(v));
    sparse()->remove(v);
    // ASSERT(v_index == _size - 1, V(v));
    --_size;
  }

  // reuse position of v to store u
  void reuse(const T u, const T v) {
    ASSERT(sparse()->get(v).second == _size - 1, V(v));
    const T index = sparse()->remove(v);
    ASSERT(index == _size - 1, V(index));
    sparse()->insert(u, index);
    dense()[index] = u;
  }

  T peek() {
    // This works, because we ensure that 'one past the current size'
    // is an element of dense(). In case dense is full, there is
    // a sentinel in place to ensure correct behavior.
    return dense()[_size];
  }

  void undoReuse(const T u, const T v) {
    const T index = sparse()->remove(u);
    sparse()->insert(v, index);
    dense()[index] = v;
  }

  void swap(IncidenceSet& other) noexcept {
    using std::swap;
    swap(_memory, other._memory);
    swap(_size, other._size);
    swap(_max_size, other._max_size);
  }

  bool contains(const T element) const {
    return sparse()->contains(element);
  }

  T size() const {
    return _size;
  }

  const T* begin() const {
    return dense();
  }

  const T* end() const {
    return dense() + _size;
  }

  T capacity() const {
    return _max_size;
  }

  void printAll() const {
    for (auto it = begin(); it != begin() + _size; ++it) {
      LOGVAR(*it);
    }
  }

 private:
  size_t sizeOfDense() const {
    return (_max_size + 1  /*sentinel for peek */) * sizeof(T);
  }

  size_t sizeOfSparse() const {
    return sizeof(Sparse) + InitialSizeFactor * _max_size * sizeof(std::pair<T, size_t>);
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

  Sparse* sparse() {
    return reinterpret_cast<Sparse*>(dense() + _max_size + 1);
  }

  const Sparse* sparse() const {
    return reinterpret_cast<const Sparse*>(dense() + _max_size + 1);
  }

  T* _memory;
  T _size;
  T _max_size;
};
}  // namespace datastructure

#endif  // SRC_LIB_DATASTRUCTURE_INCIDENCESET_H_
