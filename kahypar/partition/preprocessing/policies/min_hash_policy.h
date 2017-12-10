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
#include <vector>

#include "kahypar/utils/hash_vector.h"

namespace kahypar {
template <typename _HashFunc>
class MinHashPolicy {
 public:
  using HashFunc = _HashFunc;
  using VertexId = Hypergraph::HypernodeID;
  using HashValue = typename HashFunc::HashValue;
  using MyHashSet = HashStorage<HashValue>;
  using VertexSet = std::vector<VertexId>;

  explicit MinHashPolicy(const uint32_t hash_num = 0, const uint32_t seed = 0) :
    _dim(hash_num),
    _seed(seed),
    _hash_func_vector(hash_num, seed) { }

  void reset(const uint32_t seed) {
    _seed = seed;
    _hash_func_vector.reset(_seed);
  }

  void reset(const uint32_t seed, const uint32_t hash_num) {
    _dim = hash_num;
    _hash_func_vector.resize(hash_num);

    reset(seed);
  }

  // calculates minHashes for vertices in [begin, end)
  void operator() (const Hypergraph& graph, const VertexId begin, const VertexId end,
                   MyHashSet& hash_set) const {
    ALWAYS_ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (auto vertex_id = begin; vertex_id != end; ++vertex_id) {
      for (uint32_t hash_num = 0; hash_num < _hash_func_vector.getHashNum(); ++hash_num) {
        hash_set[hash_num][vertex_id] = minHash(_hash_func_vector[hash_num],
                                                graph.incidentEdges(vertex_id));
      }
    }
  }

  void calculateLastHash(const Hypergraph& graph, const VertexSet& vertices,
                         MyHashSet& hash_set) const {
    ALWAYS_ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (const auto& vertex_id : vertices) {
      const size_t last_hash = _hash_func_vector.getHashNum() - 1;
      hash_set[last_hash][vertex_id] = minHash(_hash_func_vector[last_hash],
                                               graph.incidentEdges(vertex_id));
    }
  }

  template <typename Iterator>
  void calculateLastHash(const Hypergraph& graph, const Iterator begin, const Iterator end,
                         MyHashSet& hash_set) const {
    ALWAYS_ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (auto it = begin; it != end; ++it) {
      const size_t last_hash = _hash_func_vector.getHashNum() - 1;
      hash_set[last_hash][*it] = minHash(_hash_func_vector[last_hash], graph.incidentEdges(*it));
    }
  }

  HashValue combinedHash(const Hypergraph& graph, const VertexId vertex_id) const {
    ALWAYS_ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    HashValue val = 0;
    for (uint32_t hash_num = 0; hash_num < _hash_func_vector.getHashNum(); ++hash_num) {
      val ^= minHash(_hash_func_vector[hash_num], graph.incidentEdges(vertex_id));
    }
    return val;
  }

  uint32_t getDim() const {
    return _dim;
  }

  size_t getHashNum() const {
    return _hash_func_vector.getHashNum();
  }

  void setHashNumAndDim(const uint32_t hash_num) {
    _dim = hash_num;
    _hash_func_vector.resize(hash_num);
  }

  void addHashFunction(const uint32_t seed) {
    _hash_func_vector.addHashFunction(seed);
    ++_dim;
  }

  void reserveHashFunctions(const uint32_t size) {
    _hash_func_vector.reserve(size);
  }

  void clear() {
    _hash_func_vector.clear();
  }

  void setSeed(const uint32_t seed) {
    _seed = seed;
  }

 protected:
  // The number of dimensions for buckets where we put hash values.
  // In this policy we put a hash value of ith hash function to ith dimension
  uint32_t _dim;

  uint32_t _seed;

  HashFuncVector<HashFunc> _hash_func_vector;

  // calculates minHash from incident edges of a particular vertex
  template <typename ValuesRange>
  HashValue minHash(HashFunc hash, const ValuesRange& values_range) const {
    HashValue res = std::numeric_limits<HashValue>::max();

    for (const auto& value : values_range) {
      res = std::min(res, hash(value));
    }
    return res;
  }
};


template <typename _THashFunc>
class CombinedHashPolicy {
 public:
  using BaseHashPolicy = _THashFunc;
  using VertexId = Hypergraph::HypernodeID;
  using HashValue = typename BaseHashPolicy::HashValue;
  using MyHashSet = HashStorage<HashValue>;

  CombinedHashPolicy(const uint32_t hash_num, const uint32_t combined_hash_num, const uint32_t seed) :
    _seed(seed),
    _dim(hash_num),
    _hash_num(hash_num),
    _combined_hash_num(combined_hash_num) { }

  // calculates minHashes for vertices in [begin, end)
  void operator() (const Hypergraph& graph, const VertexId begin, const VertexId end,
                   MyHashSet& hash_set) const {
    ALWAYS_ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    HashFuncVector<BaseHashPolicy> hash_func_vectors(_hash_num);

    hash_func_vectors.reset(_seed, _combined_hash_num);

    for (auto vertex_id = begin; vertex_id != end; ++vertex_id) {
      for (uint32_t hash_num = 0; hash_num < _hash_num; ++hash_num) {
        hash_set[hash_num][vertex_id] = hash_func_vectors[hash_num].combinedHash(graph, vertex_id);
      }
    }
  }

  uint32_t getDim() const {
    return _dim;
  }

  uint32_t getHashNum() const {
    return _hash_num;
  }

  uint32_t getCombinedHashNum() const {
    return _combined_hash_num;
  }

  void setHashNumAndDim(const uint32_t hash_num) {
    _dim = hash_num;
    _hash_num = hash_num;
  }

  void setCombinedHashNum(const uint32_t combined_hash_num) {
    _combined_hash_num = combined_hash_num;
  }

 private:
  uint32_t _seed;

  // The number of dimensions for buckets where we put hash values.
  // In this policy we put a hash value of ith hash function to ith dimension
  uint32_t _dim;

  // The number of hash functions
  uint32_t _hash_num;

  // The number of combined hash functions
  uint32_t _combined_hash_num;
};

using MinMurmurHashPolicy = MinHashPolicy<math::MurmurHash<uint32_t> >;
using LSHCombinedHashPolicy = CombinedHashPolicy<MinMurmurHashPolicy>;
}  // namespace kahypar
