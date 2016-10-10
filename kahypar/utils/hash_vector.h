#pragma once

#include "kahypar/datastructure/hash_table.h"

#include <random>
#include <unordered_set>

#include <fstream>

template <typename _HashFunc>
class HashFuncVector {
public:
  using HashFunc = _HashFunc;

  HashFuncVector(uint8_t hashNum, uint32_t seed = 0)
          :   _hashFunctions(hashNum)
  {
    ASSERT(_hashFunctions.size() >= 0, "HashNum should be greater that 0");

    reset(seed);
  }

  inline constexpr size_t getHashNum() const {
    return _hashFunctions.size();
  }

  const HashFunc& operator[] (size_t index) const {
    return _hashFunctions[index];
  }

  HashFunc& operator[] (size_t index) {
    return _hashFunctions[index];
  }

  void resize(size_t hashNum) {
    ASSERT(hashNum >= 0, "HashNum should be greater that 0");
    _hashFunctions.resize(hashNum);
  }

  void addHashFunction(uint32_t seed) {
    _hashFunctions.push_back(HashFunc());
    _hashFunctions.back().reset(seed);
  }

  void reserve(uint32_t hashNum) {
    _hashFunctions.reserve(hashNum);
  }

  template <typename... TArgs>
  void reset(uint32_t seed, TArgs&&... args) {
    std::default_random_engine eng(seed);
    std::uniform_int_distribution<uint32_t> rnd;

    for (auto& hashFunction : _hashFunctions) {
      hashFunction.reset(rnd(eng), std::forward<TArgs>(args)...);
    }
  }

private:
  std::vector<HashFunc> _hashFunctions;

};

template <typename _HashValue>
class HashSet {
public:
  using HashValue = _HashValue;

  explicit HashSet(size_t hashNum, size_t objNumber)
          :   _objNumber(objNumber)
          ,   _hashVectors(hashNum)
  {
    for (auto& hashVec : _hashVectors) {
      hashVec = std::make_unique<HashValue[]>(_objNumber);
    }
  }

  const HashValue* operator[] (uint8_t hashNum) const {
    return _hashVectors[hashNum].get();
  }

  HashValue* operator[] (uint8_t hashNum) {
    return _hashVectors[hashNum].get();
  }

  size_t getHashNum() const {
    return _hashVectors.size();
  }

  void reserve(size_t hashNum) {
    _hashVectors.reserve(hashNum);
  }

  void addHashVector() {
    _hashVectors.push_back(std::make_unique<HashValue[]>(_objNumber));
  }

private:
  const size_t _objNumber;

  // We use std::unique_ptr<HashValue[]> instead of std::vector<HashValue>
  // because we want move constructor to be used during reallocation of std::vector _hashVectors
  std::vector<std::unique_ptr<HashValue[]>> _hashVectors;

};

template <bool HasReserve>
struct Reserve {
  template <typename Container>
  static void reserve(Container& container, size_t capacity) {
    container.reserve(capacity);
  }
};

template <>
struct Reserve<false> {
  template <typename Container>
  static void reserve(Container& container, size_t capacity) {
  }
};

template <typename Container>
class HasReserve
{
  typedef char One;
  typedef long Two;

  template <typename C> static One test( decltype(&C::reserve) ) ;
  template <typename C> static Two test(...);

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

  Buckets(uint8_t dim, size_t capacity)
          :   _dim(dim)
  {
    _bucketsSets = std::make_unique<MultiContainer[]>(_dim);
    for (uint8_t dim = 0; dim < _dim; ++dim) {
      Reserve<HasReserve<MultiContainer>::value>::reserve(_bucketsSets[dim], capacity);
    }
  }

  void put(uint8_t dim, HashValue hashValue, TObject object) {
    _bucketsSets[dim][hashValue].insert(object);
  }

  auto getObjects(uint8_t dim, HashValue hashValue) {
    auto& bucket = _bucketsSets[dim][hashValue];

    return std::make_pair(bucket.begin(), bucket.end());
  };

  size_t getBucketSize(uint8_t dim, HashValue hashValue) const {
    return _bucketsSets[dim].at(hashValue).size();
  }

  bool isOneDimensional() const {
    return _dim == 1;
  }

  void removeObject(uint8_t dim, HashValue hashValue, TObject object) {
    _bucketsSets[dim][hashValue].erase(object);
  }

  std::pair<size_t, size_t> getMaxSize() const {
    size_t max = 0;
    size_t hash = 0;
    for (const auto& bucketsSet : _bucketsSets) {
      for (const auto& bucket : bucketsSet) {
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
  std::unique_ptr<MultiContainer[]> _bucketsSets;

};

template <typename HashFunc, typename TObject>
using FastHashBuckets = Buckets<hash_map<HashFunc, hash_set<TObject>>>;

template <typename HashFunc, typename TObject>
using HashBuckets = Buckets<hash_map<HashFunc, std::unordered_multiset<TObject>>>;
