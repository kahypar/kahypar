/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Yaroslav Akhremtsev <yaroslav.akhremtsev@kit.edu>
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
#include <fstream>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "kahypar/datastructure/hash_table.h"

namespace kahypar {
template <typename _HashFunc>
class HashFuncVector {
 public:
  using HashFunc = _HashFunc;

  explicit HashFuncVector(const uint8_t hash_num, const uint32_t seed = 0) :
    _hash_functions(hash_num) {
    reset(seed);
  }

  inline constexpr size_t getHashNum() const {
    return _hash_functions.size();
  }

  const HashFunc& operator[] (const size_t index) const {
    return _hash_functions[index];
  }

  HashFunc& operator[] (const size_t index) {
    return _hash_functions[index];
  }

  void resize(const size_t hash_num) {
    ALWAYS_ASSERT(hash_num >= 0, "HashNum should be greater that 0");
    _hash_functions.resize(hash_num);
  }

  void addHashFunction(const uint32_t seed) {
    _hash_functions.push_back(HashFunc());
    _hash_functions.back().reset(seed);
  }

  void reserve(const uint32_t hash_num) {
    _hash_functions.reserve(hash_num);
  }

  void clear() {
    _hash_functions.clear();
  }

  template <typename ... TArgs>
  void reset(const uint32_t seed, TArgs&& ... args) {
    std::default_random_engine eng(seed);
    std::uniform_int_distribution<uint32_t> rnd;

    for (auto& hash_function : _hash_functions) {
      hash_function.reset(rnd(eng), std::forward<TArgs>(args) ...);
    }
  }

 private:
  std::vector<HashFunc> _hash_functions;
};

template <typename _HashValue>
class HashStorage {
 public:
  using HashValue = _HashValue;

  explicit HashStorage(const size_t hash_num, const size_t obj_number) :
    _obj_number(obj_number),
    _current_num_hash_vectors(hash_num),
    _max_capacity(hash_num),
    _hash_vectors() { }

  const HashValue* operator[] (const uint8_t hash_num) const {
    ASSERT(hash_num < _current_num_hash_vectors);
    return &_hash_vectors[hash_num * _obj_number];
  }

  HashValue* operator[] (const uint8_t hash_num) {
    ASSERT(hash_num < _current_num_hash_vectors);
    return &_hash_vectors[hash_num * _obj_number];
  }

  size_t getHashNum() const {
    return _current_num_hash_vectors;
  }

  void reserve(const size_t hash_num) {
    _hash_vectors.reserve(hash_num * _obj_number);
    for (size_t i = 0; i < hash_num; ++i) {
      for (size_t j = 0; j < _obj_number; ++j) {
        _hash_vectors.emplace_back(HashValue());
      }
    }
    _max_capacity = hash_num;
  }

  void addHashVector() {
    if (_current_num_hash_vectors == _max_capacity) {
      for (size_t j = 0; j < _obj_number; ++j) {
        _hash_vectors.emplace_back(HashValue());
      }
      ++_max_capacity;
    }
    ++_current_num_hash_vectors;
  }

  void clear() {
    _current_num_hash_vectors = 0;
  }

 private:
  const size_t _obj_number;
  size_t _current_num_hash_vectors;
  size_t _max_capacity;

  // We use std::unique_ptr<HashValue[]> instead of std::vector<HashValue>
  // because we want move constructor to be used during reallocation of std::vector _hash_vectors
  std::vector<HashValue> _hash_vectors;
};

template <typename MultiContainer>
class Buckets {
 public:
  using HashValue = typename MultiContainer::key_type;
  using TObject = typename MultiContainer::mapped_type::key_type;

  static_assert(std::is_integral<HashValue>::value, "HashFunc value should be of integral type");
  static_assert(std::is_integral<TObject>::value, "Object should be of integral type");

  explicit Buckets(const size_t capacity) :
    _buckets() {
    _buckets.reserve(capacity);
  }

  void put(const HashValue hash_value, const TObject object) {
    _buckets[hash_value].insert(object);
  }

  auto getObjects(const HashValue hash_value) {
    return std::make_pair(_buckets[hash_value].begin(), _buckets[hash_value].end());
  }

  size_t getBucketSize(const HashValue hash_value) const {
    return _buckets[hash_value].size();
  }

  void removeObject(const HashValue hash_value, const TObject object) {
    _buckets[hash_value].erase(object);
  }

  void clear() {
    _buckets.clear();
  }

  std::pair<size_t, size_t> getMaxSize() const {
    size_t max = 0;
    size_t hash = 0;
    for (const auto& bucket : _buckets) {
      max = std::max(max, bucket.second.size());
      if (max == bucket.second.size()) {
        hash = bucket.first;
      }
    }
    return std::make_pair(hash, max);
  }

 private:
  MultiContainer _buckets;
};

template <typename HashFunc, typename TObject>
using HashBuckets = Buckets<ds::HashMap<HashFunc, std::set<TObject> > >;
}  // namespace kahypar
