/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Yaroslav Akhremtsev <yaroslav.akhremtsev@kit.edu>
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
#include <vector>

namespace std   {
template <typename T, typename V>
struct numeric_limits<pair<T, V> >{
  static constexpr pair<T, V> max() noexcept {
    return std::make_pair(numeric_limits<T>::max(), numeric_limits<V>::max());
  }
};
}  // namespace std

namespace kahypar {
namespace ds {
template <typename T>
struct SimpleHash {
  size_t operator() (const T& x) const {
    return x;
  }
};


template <>
struct SimpleHash<std::pair<int, int> >{
  uint64_t operator() (const std::pair<int, int>& x) const {
    return (static_cast<uint64_t>(x.first) << 32) + x.second;
  }
};


template <typename HashTable>
class HashTableIterator {
 private:
  using HashMap = HashTable;
  using Element = typename HashMap::Element;
  using Position = typename HashMap::Position;

 public:
  HashTableIterator(const HashMap& hm, const Position offset) :
    _hm(hm),
    _offset(offset),
    _specials(0),
    _num_specials(_hm.getNumSpecials()) { }

  const Element& operator* () {
    const Element* res = _hm.getSpecialElement(_specials);
    if (res) {
      return *res;
    } else {
      return _hm._ht[_hm._poses[_offset - _specials]];
    }
  }

  HashTableIterator& operator++ () {
    ++_offset;
    if (_specials < _num_specials) {
      ++_specials;
    }
    return *this;
  }

  bool operator== (const HashTableIterator& it) {
    return _offset == it._offset;
  }

  bool operator!= (const HashTableIterator& it) {
    return !(*this == it);
  }

 private:
  const HashMap& _hm;
  Position _offset;
  Position _specials;
  Position _num_specials;
};

template <typename Key, typename Value, typename Hash = SimpleHash<Key>,
          bool Cache = true, size_t SizeFactor = 2>
class HashMap {
 public:
  using Element = std::pair<Key, Value>;
  using key_type = Key;
  using mapped_type = Value;

 private:
  using Position = uint32_t;

 public:
  using Iterator = HashTableIterator<HashMap>;

  friend Iterator;

  explicit HashMap(const uint64_t max_size = 0) :
    _empty_element_key(false),
    _deleted_element_key(false),
    _empty_element(std::numeric_limits<Key>::max(), Value()),
    _deleted_element(_empty_element.first - 1, Value()),
    _ht_size(max_size * SizeFactor),
    _max_size(max_size),
    _ht(_ht_size + _max_size * 1.1, std::make_pair(_empty_element.first, Value())),
    _poses(),
    _pos_in_position(_ht_size + _max_size * 1.1),
    _hash(),
    _last_key(_empty_element.first),
    _last_position(0) {
    _poses.reserve(max_size);
  }

  HashMap(const HashMap&) = default;
  HashMap(HashMap&&) = default;

  HashMap& operator= (const HashMap& other) = delete;
  HashMap& operator= (HashMap&& other) = delete;

  ~HashMap() = default;

  void reserve(const uint32_t max_size) {
    _ht_size = max_size * SizeFactor;
    _max_size = max_size;
    _ht.resize(_ht_size + _max_size * 1.1, std::make_pair(_empty_element.first, Value()));
    _pos_in_position.resize(_ht_size + _max_size * 1.1);
  }

  size_t size() const {
    return _poses.size() + _empty_element_key + _deleted_element_key;
  }

  Iterator begin() const {
    return Iterator(*this, 0);
  }

  Iterator end() const {
    return Iterator(*this, size());
  }

  bool empty() const {
    return size() == 0;
  }

  void erase(const Key& key) {
    if (key == _empty_element.first) {
      _empty_element_key = false;
      return;
    }

    if (key == _deleted_element.first) {
      _deleted_element_key = false;
      return;
    }

    const Position pos = findPosition(key);
    if (_ht[pos].first != _empty_element.first) {
      _ht[pos].first = _deleted_element.first;
      const uint32_t pos_of_deleted = _pos_in_position[pos];
      _pos_in_position[_poses.back()] = pos_of_deleted;
      std::swap(_poses[pos_of_deleted], _poses.back());
      _poses.resize(_poses.size() - 1);
    }
  }

  Value& operator[] (const Key& key) {
    if (key == _empty_element.first) {
      if (!_empty_element_key) {
        _empty_element_key = true;
        _empty_element.second = Value();
      }
      return _empty_element.second;
    }

    if (key == _deleted_element.first) {
      if (!_deleted_element_key) {
        _deleted_element_key = true;
        _deleted_element.second = Value();
      }
      return _deleted_element.second;
    }

    const Position pos = findPosition(key);

    if (_ht[pos].first == _empty_element.first) {
      _ht[pos].first = key;
      _ht[pos].second = Value();
      _poses.push_back(pos);
      _pos_in_position[pos] = _poses.size() - 1;
    }

    return _ht[pos].second;
  }

  bool contains(const Key& key) {
    if (key == _empty_element.first) {
      return _empty_element_key;
    }

    if (key == _deleted_element.first) {
      return _deleted_element_key;
    }

    return _ht[findPosition(key)].first != _empty_element.first;
  }

  void insert(const Element& elem) {
    insertImpl(elem.first, elem.second);
  }

  void insert(const Key& key, const Value& value) {
    insertImpl(key, value);
  }

  void clear() {
    for (const auto& pos : _poses) {
      _ht[pos].first = _empty_element.first;
    }
    _poses.clear();

    _last_key = _empty_element.first;
    _last_position = 0;

    _empty_element_key = false;
    _deleted_element_key = false;
  }

  void swap(HashMap& hash_map) {
    std::swap(_ht_size, hash_map._ht_size);
    std::swap(_max_size, hash_map._max_size);
    _ht.swap(hash_map._ht);
    _poses.swap(hash_map._poses);
    std::swap(_last_key, hash_map._last_key);
    std::swap(_last_position, hash_map._last_position);
    _pos_in_position.swap(hash_map._pos_in_position);
    _empty_element.swap(hash_map._empty_element);
    _deleted_element.swap(hash_map._deleted_element);
    std::swap(_empty_element_key, hash_map._empty_element_key);
    std::swap(_deleted_element_key, hash_map._deleted_element_key);
  }

 private:
  Position getNumSpecials() const {
    return _empty_element_key + _deleted_element_key;
  }

  const Element* getSpecialElement(const Position offset) const {
    if (_empty_element_key && offset == 0) {
      return &_empty_element;
    }
    if (_deleted_element_key && offset == 1) {
      return &_deleted_element;
    }
    return nullptr;
  }

  void insertImpl(const Key& key, const Value& value) {
    if (key == _empty_element.first) {
      if (!_empty_element_key) {
        _empty_element_key = true;
        _empty_element.second = value;
      }
      return;
    }

    if (key == _deleted_element.first) {
      if (!_deleted_element_key) {
        _deleted_element_key = true;
        _deleted_element.second = value;
      }
      return;
    }

    const Position pos = findPosition(key);
    if (_ht[pos].first == _empty_element.first) {
      _ht[pos].first = key;
      _ht[pos].second = value;
      _poses.push_back(pos);
      _pos_in_position[pos] = _poses.size() - 1;
    }
  }

  Position findPosition(const Key& key) {
    if (Cache && key == _last_key) {
      return _last_position;
    }

    const Position startPosition = _hash(key) % _ht_size;
    for (Position pos = startPosition; pos < _ht.size(); ++pos) {
      if (_ht[pos].first == _empty_element.first || _ht[pos].first == key) {
        if (Cache) {
          _last_key = key;
          _last_position = pos;
        }
        return pos;
      }
    }

    std::cerr << "hash table overflowed" << std::endl;
    std::exit(-1);
  }

  const Position _num_specials = 2;
  bool _empty_element_key;
  bool _deleted_element_key;
  Element _empty_element;
  Element _deleted_element;
  uint64_t _ht_size;
  uint64_t _max_size;
  std::vector<Element> _ht;
  std::vector<Position> _poses;
  std::vector<uint32_t> _pos_in_position;
  Hash _hash;
  Key _last_key;
  Position _last_position;
};

template <typename Key, typename Value, typename Hash = SimpleHash<Key>,
          bool Cache = true, size_t SizeFactor = 2>
class InsertOnlyHashMap {
 public:
  using Element = std::pair<Key, Value>;
  using key_type = Key;
  using mapped_type = Value;

 private:
  using Position = uint32_t;

 public:
  using Iterator = HashTableIterator<InsertOnlyHashMap>;

  friend Iterator;

  explicit InsertOnlyHashMap(const uint64_t max_size = 0) :
    _empty_element_key(false),
    _empty_element(std::numeric_limits<Key>::max(), Value()),
    _ht_size(max_size * SizeFactor),
    _max_size(max_size),
    _ht(_ht_size + _max_size * 1.1, std::make_pair(_empty_element.first, Value())),
    _poses(),
    _hash(),
    _last_key(_empty_element.first),
    _last_position(0) {
    _poses.reserve(max_size);
  }

  InsertOnlyHashMap(const InsertOnlyHashMap&) = default;
  InsertOnlyHashMap(InsertOnlyHashMap&&) = default;

  InsertOnlyHashMap& operator= (const InsertOnlyHashMap& other) = default;
  InsertOnlyHashMap& operator= (InsertOnlyHashMap&& other) = default;

  ~InsertOnlyHashMap() = default;

  void reserve(const uint32_t max_size) {
    _ht_size = max_size * SizeFactor;
    _max_size = max_size;
    _ht.resize(_ht_size + _max_size * 1.1, std::make_pair(_empty_element, Value()));
  }

  uint64_t size() const {
    return _poses.size() + _empty_element_key;
  }

  Iterator begin() const {
    return Iterator(*this, 0);
  }

  Iterator end() const {
    return Iterator(*this, size());
  }

  bool empty() const {
    return size() == 0;
  }

  Value& operator[] (const Key& key) {
    if (key == _empty_element.first) {
      if (!_empty_element_key) {
        _empty_element_key = true;
        _empty_element.second = Value();
      }
      return _empty_element.second;
    }

    const Position pos = findPosition(key);
    if (_ht[pos].first == _empty_element.first) {
      _ht[pos].first = key;
      _ht[pos].second = Value();
      _poses.push_back(pos);
    }

    return _ht[pos].second;
  }

  bool contains(const Key& key) {
    if (key == _empty_element.first) {
      return _empty_element_key;
    }

    return _ht[findPosition(key)].first != _empty_element.first;
  }

  void insert(const Element& elem) {
    insertImpl(elem.first, elem.second);
  }

  void insert(const Key& key, const Value& value) {
    insertImpl(key, value);
  }

  void clear() {
    for (const auto& pos : _poses) {
      _ht[pos].first = _empty_element;
    }

    _poses.clear();

    _last_key = _empty_element;
    _last_position = 0;

    _empty_element_key = false;
  }

  void swap(InsertOnlyHashMap& hash_map) {
    std::swap(_ht_size, hash_map._ht_size);
    std::swap(_max_size, hash_map._max_size);
    _ht.swap(hash_map._ht);
    _poses.swap(hash_map._poses);
    std::swap(_last_key, hash_map._last_key);
    std::swap(_last_position, hash_map._last_position);
    _empty_element.swap(hash_map._empty_element);
    std::swap(_empty_element_key, hash_map._empty_element_key);
  }

 private:
  Position getNumSpecials() const {
    return _empty_element_key;
  }

  const Element* getSpecialElement(const Position offset) const {
    if (_empty_element_key && offset == 0) {
      return &_empty_element;
    }
    return nullptr;
  }

  void insertImpl(const Key& key, const Value& value) {
    if (key == _empty_element.first) {
      if (!_empty_element_key) {
        _empty_element_key = true;
        _empty_element.second = value;
      }
      return;
    }

    const Position pos = findPosition(key);
    if (_ht[pos].first == _empty_element.first) {
      _ht[pos].first = key;
      _ht[pos].second = value;
      _poses.push_back(pos);
    }
  }

  Position findPosition(const Key& key) {
    if (Cache && key == _last_key) {
      return _last_position;
    }

    const Position startPosition = _hash(key) % _ht_size;
    for (Position pos = startPosition; pos < _ht.size(); ++pos) {
      if (_ht[pos].first == _empty_element.first || _ht[pos].first == key) {
        if (Cache) {
          _last_key = key;
          _last_position = pos;
        }
        return pos;
      }
    }

    std::cerr << "hash table overflowed" << std::endl;
    std::exit(-1);
  }

  const Position _num_specials = 1;
  bool _empty_element_key;
  Element _empty_element;
  uint64_t _ht_size;
  uint64_t _max_size;
  std::vector<Element> _ht;
  std::vector<Position> _poses;
  Hash _hash;
  Key _last_key;
  Position _last_position;
};

template <typename Key, typename Hash = SimpleHash<Key>, bool Cache = true, size_t SizeFactor = 2>
class HashSet {
 public:
  using Element = Key;
  using key_type = Key;

 private:
  using Position = uint32_t;

 public:
  using Iterator = HashTableIterator<HashSet>;

  friend Iterator;

  explicit HashSet(const uint64_t max_size = 0) :
    _empty_element_key(false),
    _deleted_element_key(false),
    _empty_element(std::numeric_limits<Key>::max()),
    _deleted_element(_empty_element - 1),
    _ht_size(max_size * SizeFactor),
    _max_size(max_size),
    _ht(_ht_size + _max_size * 1.1, _empty_element),
    _pos_in_position(_ht_size + _max_size * 1.1),
    _last_key(_empty_element),
    _last_position(0) {
    _poses.reserve(max_size);
  }

  HashSet(const HashSet&) = default;
  HashSet(HashSet&&) = default;

  HashSet& operator= (const HashSet& other) = delete;
  HashSet& operator= (HashSet&& other) = default;

  ~HashSet() = default;

  void reserve(const uint32_t max_size) {
    _ht_size = max_size * SizeFactor;
    _max_size = max_size;
    _ht.resize(_ht_size + _max_size * 1.1, _empty_element);
    _pos_in_position.resize(_ht_size + _max_size * 1.1);
  }

  uint64_t size() const {
    return _poses.size() + _empty_element_key + _deleted_element_key;
  }

  Iterator begin() const {
    return Iterator(*this, 0);
  }

  Iterator end() const {
    return Iterator(*this, size());
  }

  bool empty() const {
    return size() == 0;
  }

  void erase(const Key& key) {
    if (key == _empty_element) {
      _empty_element_key = false;
      return;
    }

    if (key == _deleted_element) {
      _deleted_element_key = false;
      return;
    }

    const Position pos = findPosition(key);
    if (_ht[pos] != _empty_element) {
      _ht[pos] = _deleted_element;
      const uint32_t pos_of_deleted = _pos_in_position[pos];
      _pos_in_position[_poses.back()] = pos_of_deleted;
      std::swap(_poses[pos_of_deleted], _poses.back());
      _poses.resize(_poses.size() - 1);
    }
  }

  bool contains(const Key& key) {
    if (key == _empty_element) {
      return _empty_element_key;
    }

    if (key == _deleted_element) {
      return _deleted_element_key;
    }

    return _ht[findPosition(key)] != _empty_element;
  }

  void insert(const Key& key) {
    insertImpl(key);
  }

  void clear() {
    for (const auto& pos : _poses) {
      _ht[pos] = _empty_element;
    }

    _poses.clear();

    _last_key = _empty_element;
    _last_position = 0;

    _empty_element_key = false;
    _deleted_element_key = false;
  }

  void swap(HashSet& hash_set) {
    std::swap(_ht_size, hash_set._ht_size);
    std::swap(_max_size, hash_set._max_size);
    _ht.swap(hash_set._ht);
    _poses.swap(hash_set._poses);
    std::swap(_last_key, hash_set._last_key);
    std::swap(_last_position, hash_set._last_position);
    _pos_in_position.swap(hash_set._pos_in_position);
    std::swap(_empty_element_key, hash_set._empty_element_key);
    std::swap(_deleted_element_key, hash_set._deleted_element_key);
  }

 private:
  Position getNumSpecials() const {
    return _empty_element_key + _deleted_element_key;
  }

  const Element* getSpecialElement(const Position offset) const {
    if (_empty_element_key && offset == 0) {
      return &_empty_element;
    }
    if (_deleted_element_key && offset == 1) {
      return &_deleted_element;
    }
    return nullptr;
  }

  void insertImpl(const Key& key) {
    if (key == _empty_element) {
      if (!_empty_element_key) {
        _empty_element_key = true;
      }
      return;
    }

    if (key == _deleted_element) {
      if (!_deleted_element_key) {
        _deleted_element_key = true;
      }
      return;
    }

    const Position pos = findPosition(key);
    if (_ht[pos] == _empty_element) {
      _ht[pos] = key;
      _poses.push_back(pos);
      _pos_in_position[pos] = _poses.size() - 1;
    }
  }

  Position findPosition(const Key& key) {
    if (Cache && key == _last_key) {
      return _last_position;
    }

    const Position startPosition = _hash(key) % _ht_size;
    for (Position pos = startPosition; pos < _ht.size(); ++pos) {
      if (_ht[pos] == _empty_element || _ht[pos] == key) {
        if (Cache) {
          _last_key = key;
          _last_position = pos;
        }
        return pos;
      }
    }

    std::cerr << "hash table overflowed" << std::endl;
    std::exit(-1);
  }

  const Position _num_specials = 2;
  bool _empty_element_key;
  bool _deleted_element_key;
  const Key _empty_element;
  const Key _deleted_element;
  uint64_t _ht_size;
  uint64_t _max_size;
  std::vector<Element> _ht;
  std::vector<Position> _poses;
  std::vector<uint32_t> _pos_in_position;
  Hash _hash;
  Key _last_key;
  Position _last_position;
};

template <typename Key, typename Hash = SimpleHash<Key>, bool Cache = true, size_t SizeFactor = 2>
class InsertOnlyHashSet {
 public:
  using Element = Key;

 private:
  using Position = uint32_t;

 public:
  using Iterator = HashTableIterator<InsertOnlyHashSet>;

  friend Iterator;

  explicit InsertOnlyHashSet(const uint64_t max_size = 0) :
    _empty_element_key(false),
    _empty_element(std::numeric_limits<Key>::max()),
    _ht_size(max_size * SizeFactor),
    _max_size(max_size),
    _ht(_ht_size + _max_size * 1.1, _empty_element),
    _poses(),
    _hash(),
    _last_key(_empty_element),
    _last_position(0) {
    _poses.reserve(max_size);
  }

  InsertOnlyHashSet(const InsertOnlyHashSet&) = default;
  InsertOnlyHashSet(InsertOnlyHashSet&&) = default;

  InsertOnlyHashSet& operator= (const InsertOnlyHashSet& other) = default;
  InsertOnlyHashSet& operator= (InsertOnlyHashSet&& other) = default;

  ~InsertOnlyHashSet() = default;

  void reserve(const uint32_t max_size) {
    _ht_size = max_size * SizeFactor;
    _max_size = max_size;
    _ht.resize(_ht_size + _max_size * 1.1, _empty_element);
  }

  uint64_t size() const {
    return _poses.size() + _empty_element_key;
  }

  Iterator begin() const {
    return Iterator(*this, 0);
  }

  Iterator end() const {
    return Iterator(*this, size());
  }

  bool empty() const {
    return size() == 0;
  }

  bool contains(const Key& key) {
    if (key == _empty_element) {
      return _empty_element_key;
    }
    return _ht[findPosition(key)] != _empty_element;
  }

  void insert(const Key& key) {
    insertImpl(key);
  }

  void clear() {
    for (const auto& pos : _poses) {
      _ht[pos] = _empty_element;
    }

    _poses.clear();

    _last_key = _empty_element;
    _last_position = 0;

    _empty_element_key = false;
  }

  void swap(InsertOnlyHashSet& hash_set) {
    std::swap(_ht_size, hash_set._ht_size);
    std::swap(_max_size, hash_set._max_size);
    _ht.swap(hash_set._ht);
    _poses.swap(hash_set._poses);
    std::swap(_last_key, hash_set._last_key);
    std::swap(_last_position, hash_set._last_position);
    std::swap(_empty_element_key, hash_set._empty_element_key);
  }

 private:
  Position getNumSpecials() const {
    return _empty_element_key;
  }

  const Element* getSpecialElement(const Position offset) const {
    if (_empty_element_key && offset == 0) {
      return &_empty_element;
    } else {
      return nullptr;
    }
  }

  void insertImpl(const Key& key) {
    if (key == _empty_element && !_empty_element_key) {
      _empty_element_key = true;
      return;
    }

    const Position pos = findPosition(key);
    if (_ht[pos] == _empty_element) {
      _ht[pos] = key;
      _poses.push_back(pos);
    }
  }

  Position findPosition(const Key& key) {
    if (Cache && key == _last_key) {
      return _last_position;
    }

    const Position startPosition = _hash(key) % _ht_size;
    for (Position pos = startPosition; pos < _ht.size(); ++pos) {
      if (_ht[pos] == _empty_element || _ht[pos] == key) {
        if (Cache) {
          _last_key = key;
          _last_position = pos;
        }
        return pos;
      }
    }

    std::cerr << "hash table overflowed" << std::endl;
    std::exit(-1);
  }

  const Position _num_specials = 2;
  bool _empty_element_key;
  const Key _empty_element;
  uint64_t _ht_size;
  uint64_t _max_size;
  std::vector<Element> _ht;
  std::vector<Position> _poses;
  Hash _hash;
  Key _last_key;
  Position _last_position;
};
}  // namespace ds
}  // namespace kahypar
