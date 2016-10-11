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
#include <fstream>
#include <random>
#include <unordered_set>
#include <utility>
#include <vector>

#include "kahypar/datastructure/hash_table.h"

namespace kahypar {
template <typename _HashFunc>
class HashFuncVector {
 public:
  using HashFunc = _HashFunc;

  HashFuncVector(const uint8_t hash_num, const uint32_t seed = 0) :
    _hashFunctions(hash_num) {
    ALWAYS_ASSERT(_hashFunctions.size() >= 0, "HashNum should be greater that 0");

    reset(seed);
  }

  inline constexpr size_t getHashNum() const {
    return _hashFunctions.size();
  }

  const HashFunc& operator[] (const size_t index) const {
    return _hashFunctions[index];
  }

  HashFunc& operator[] (const size_t index) {
    return _hashFunctions[index];
  }

  void resize(const size_t hash_num) {
    ALWAYS_ASSERT(hash_num >= 0, "HashNum should be greater that 0");
    _hashFunctions.resize(hash_num);
  }

  void addHashFunction(const uint32_t seed) {
    _hashFunctions.push_back(HashFunc());
    _hashFunctions.back().reset(seed);
  }

  void reserve(const uint32_t hash_num) {
    _hashFunctions.reserve(hash_num);
  }

  template <typename ... TArgs>
  void reset(const uint32_t seed, TArgs&& ... args) {
    std::default_random_engine eng(seed);
    std::uniform_int_distribution<uint32_t> rnd;

    for (auto& hash_function : _hashFunctions) {
      hash_function.reset(rnd(eng), std::forward<TArgs>(args) ...);
    }
  }

 private:
  std::vector<HashFunc> _hashFunctions;
};

template <typename _HashValue>
class HashSet {
 public:
  using HashValue = _HashValue;

  explicit HashSet(const size_t hash_num, const size_t obj_number) :
    _obj_number(obj_number)
    ,
    _hash_vectors(hash_num) {
    for (auto& hash_vec : _hash_vectors) {
      hash_vec = std::make_unique<HashValue[]>(_obj_number);
    }
  }

  const HashValue* operator[] (const uint8_t hash_num) const {
    return _hash_vectors[hash_num].get();
  }

  HashValue* operator[] (const uint8_t hash_num) {
    return _hash_vectors[hash_num].get();
  }

  size_t getHashNum() const {
    return _hash_vectors.size();
  }

  void reserve(const size_t hash_num) {
    _hash_vectors.reserve(hash_num);
  }

  void addHashVector() {
    _hash_vectors.push_back(std::make_unique<HashValue[]>(_obj_number));
  }

 private:
  const size_t _obj_number;

  // We use std::unique_ptr<HashValue[]> instead of std::vector<HashValue>
  // because we want move constructor to be used during reallocation of std::vector _hash_vectors
  std::vector<std::unique_ptr<HashValue[]> > _hash_vectors;
};

template <bool HasReserve>
struct Reserve {
  template <typename Container>
  static void reserve(Container& container, const size_t capacity) {
    container.reserve(capacity);
  }
};

template <>
struct Reserve<false>{
  template <typename Container>
  static void reserve(Container& container, const size_t capacity) { }
};

template <typename Container>
class HasReserve {
  typedef char One;
  typedef long Two;

  template <typename C>
  static One test(decltype(&C::reserve));
  template <typename C>
  static Two test(...);

 public:
  enum { value = sizeof(test<Container>(0)) == sizeof(char) };
};

template <typename MultiContainer>
class Buckets {
 public:
  using HashValue = typename MultiContainer::key_type;
  using TObject = typename MultiContainer::mapped_type::key_type;

  static_assert(std::is_integral<HashValue>::value, "HashFunc value should be of integral type");
  static_assert(std::is_integral<TObject>::value, "Object should be of integral type");

  Buckets(const uint8_t dim, const size_t capacity) :
    _dim(dim) {
    _buckets_sets = std::make_unique<MultiContainer[]>(_dim);
    for (uint8_t dim = 0; dim < _dim; ++dim) {
      Reserve<HasReserve<MultiContainer>::value>::reserve(_buckets_sets[dim], capacity);
    }
  }

  void put(const uint8_t dim, HashValue hash_value, TObject object) {
    _buckets_sets[dim][hash_value].insert(object);
  }

  auto getObjects(const uint8_t dim, HashValue hash_value) {
    auto& bucket = _buckets_sets[dim][hash_value];

    return std::make_pair(bucket.begin(), bucket.end());
  };

  size_t getBucketSize(const uint8_t dim, HashValue hash_value) const {
    return _buckets_sets[dim].at(hash_value).size();
  }

  bool isOneDimensional() const {
    return _dim == 1;
  }

  void removeObject(const uint8_t dim, HashValue hash_value, TObject object) {
    _buckets_sets[dim][hash_value].erase(object);
  }

  std::pair<size_t, size_t> getMaxSize() const {
    size_t max = 0;
    size_t hash = 0;
    for (const auto& buckets_set : _buckets_sets) {
      for (const auto& bucket : buckets_set) {
        max = std::max(max, bucket.second.size());
        if (max == bucket.second.size()) {
          hash = bucket.first;
        }
      }
    }
    return std::make_pair(hash, max);
  }

 private:
  const uint8_t _dim;
  std::unique_ptr<MultiContainer[]> _buckets_sets;
};

template <typename HashFunc, typename TObject>
using FastHashBuckets = Buckets<ds::HashMap<HashFunc, ds::HashSet<TObject> > >;

template <typename HashFunc, typename TObject>
using HashBuckets = Buckets<ds::HashMap<HashFunc, std::unordered_multiset<TObject> > >;
}  // namespace kahypar
