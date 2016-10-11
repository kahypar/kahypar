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

#include "kahypar/datastructure/hash_table.h"
#include "kahypar/definitions.h"
#include "kahypar/utils/hash_vector.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace kahypar {
template <typename _HashFunc>
class MinHashPolicy {
 public:
  using HashFunc = _HashFunc;
  using VertexId = Hypergraph::HypernodeID;
  using HashValue = typename HashFunc::HashValue;
  using MyHashSet = HashSet<HashValue>;
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
  void operator() (const Hypergraph& graph, const VertexId begin, const VertexId end, MyHashSet& hash_set) const {
    ALWAYS_ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (auto vertex_id = begin; vertex_id != end; ++vertex_id) {
      for (uint32_t hash_num = 0; hash_num < _hash_func_vector.getHashNum(); ++hash_num) {
        hash_set[hash_num][vertex_id] = minHash(_hash_func_vector[hash_num], graph.incidentEdges(vertex_id));
      }
    }
  }

  void calculateLastHash(const Hypergraph& graph, const VertexSet& vertices, MyHashSet& hash_set) const {
    ALWAYS_ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (auto vertex_id : vertices) {
      size_t last_hash = _hash_func_vector.getHashNum() - 1;
      hash_set[last_hash][vertex_id] = minHash(_hash_func_vector[last_hash], graph.incidentEdges(vertex_id));
    }
  }

  template <typename Iterator>
  void calculateLastHash(const Hypergraph& graph, Iterator begin, Iterator end,
                         MyHashSet& hash_set) const {
    ALWAYS_ASSERT(getHashNum() > 0, "The number of hashes should be greater than zero");
    for (auto it = begin; it != end; ++it) {
      size_t last_hash = _hash_func_vector.getHashNum() - 1;
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

  uint32_t getHashNum() const {
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

    for (auto value : values_range) {
      res = std::min(res, hash(value));
    }
    return res;
  }
};

using MinMurmurHashPolicy = MinHashPolicy<math::MurmurHash<uint32_t> >;

template <typename _THashFunc>
class CombinedHashPolicy {
 public:
  using BaseHashPolicy = _THashFunc;
  using VertexId = Hypergraph::HypernodeID;
  using HashValue = typename BaseHashPolicy::HashValue;
  using MyHashSet = HashSet<HashValue>;

  CombinedHashPolicy(const uint32_t hash_num, const uint32_t combined_hash_num, const uint32_t seed) :
    _seed(seed),
    _dim(hash_num),
    _hash_num(hash_num),
    _combined_hash_num(combined_hash_num) { }

  // calculates minHashes for vertices in [begin, end)
  void operator() (const Hypergraph& graph, const VertexId begin, const VertexId end, MyHashSet& hash_set) const {
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

using LSHCombinedHashPolicy = CombinedHashPolicy<MinMurmurHashPolicy>;

class Coarsening {
 public:
  using VertexId = HypernodeID;
  using EdgeId = HyperedgeID;

  static Hypergraph build(const Hypergraph& hypergraph, std::vector<VertexId>& clusters,
                          const PartitionID partition_id) {
    EdgeId num_edges = hypergraph.currentNumEdges();

    // 0 : reenumerate
    VertexId num_vertices = reenumerateClusters(clusters);

    // 1 : build vector of indicies and pins for hyperedges
    std::vector<size_t> indices_of_edges;
    indices_of_edges.reserve(num_edges + 1);

    std::vector<VertexId> pins_of_edges;
    pins_of_edges.reserve(hypergraph.currentNumPins());

    ds::InsertOnlyHashMap<Edge, std::pair<EdgeId, HyperedgeWeight>, HashEdge, false> parallel_edges(3 * num_edges);

    size_t offset = 0;
    size_t removed_edges = 0;
    size_t non_disabled_edge_id = 0;
    for (auto edge_id : hypergraph.edges()) {
      auto pins_range = hypergraph.pins(edge_id);
      ds::InsertOnlyHashSet<VertexId> new_pins(pins_range.second - pins_range.first);

      for (auto vertex_id : pins_range) {
        new_pins.insert(clusters[vertex_id]);
      }

      // if we have only one vertex in a hyperedge then we remove the hyperedge
      if (new_pins.size() <= 1) {
        ++removed_edges;
        ++non_disabled_edge_id;
        continue;
      }

      for (auto new_pin : new_pins) {
        pins_of_edges.push_back(new_pin);
      }

      // check for parallel edges
      Edge edge(pins_of_edges.begin() + offset, pins_of_edges.begin() + offset + new_pins.size());

      if (!parallel_edges.contains(edge)) {
        // no parallel edges
        indices_of_edges.push_back(offset);
        offset += new_pins.size();
        parallel_edges.insert(std::make_pair(edge, std::make_pair(non_disabled_edge_id - removed_edges,
                                                                  hypergraph.edgeWeight(edge_id))));
      } else {
        ++parallel_edges[edge].second;

        ++removed_edges;
        pins_of_edges.resize(pins_of_edges.size() - new_pins.size());
      }
      ++non_disabled_edge_id;
    }
    indices_of_edges.push_back(offset);

    // 2 : calculate weights of vertices in contracted graph
    std::vector<HypernodeWeight> vertex_weights(num_vertices);

    for (auto cluster_id : clusters) {
      ++vertex_weights[cluster_id];
    }

    // 3 : calculate weights of hyper edgs in contracted graph
    num_edges -= removed_edges;
    std::vector<HyperedgeWeight> edge_weights(num_edges);
    for (const auto& value : parallel_edges) {
      edge_weights[value.second.first] = value.second.second;
    }

    return Hypergraph(num_vertices, num_edges, indices_of_edges, pins_of_edges, partition_id, &edge_weights, &vertex_weights);
  }

 private:
  using IncidenceIterator = Hypergraph::IncidenceIterator;

  static VertexId reenumerateClusters(std::vector<VertexId>& clusters) {
    if (clusters.empty())
      return 0;

    std::vector<VertexId> new_clusters(clusters.size());

    for (auto clst : clusters)
      new_clusters[clst] = 1;

    for (size_t i = 1; i < new_clusters.size(); ++i)
      new_clusters[i] += new_clusters[i - 1];

    for (size_t node = 0; node < clusters.size(); ++node)
      clusters[node] = new_clusters[clusters[node]] - 1;

    return new_clusters.back();
  }

  struct Edge {
    IncidenceIterator _begin, _end;

    Edge() { }

    Edge(IncidenceIterator begin, IncidenceIterator end) :
      _begin(begin),
      _end(end) { }

    IncidenceIterator begin() const {
      return _begin;
    }

    IncidenceIterator end() const {
      return _end;
    }

    bool operator== (const Edge& edge) const {
      size_t this_size = _end - _begin;
      size_t other_size = edge._end - edge._begin;

      if (this_size != other_size) {
        return false;
      }

      for (size_t offset = 0; offset < this_size; ++offset) {
        if (*(_begin + offset) != *(edge._begin + offset)) {
          return false;
        }
      }

      return true;
    }

    bool operator< (const Edge& edge) const {
      size_t this_size = _end - _begin;
      size_t other_size = edge._end - edge._begin;

      size_t size = std::min(this_size, other_size);

      for (size_t offset = 0; offset < size; ++offset) {
        if (*(_begin + offset) < *(edge._begin + offset)) {
          return true;
        } else {
          if (*(_begin + offset) > *(edge._begin + offset)) {
            return false;
          }
        }
      }
      return this_size < other_size;
    }
  };

  struct HashEdge {
    size_t operator() (const Edge& edge) const {
      size_t hash = 0;
      math::MurmurHash<uint32_t> hash_func;
      for (auto edge_id : edge) {
        hash ^= hash_func(edge_id);
      }
      return hash;
    }
  };
};

class Uncoarsening {
 public:
  using VertexId = HypernodeID;

  static void applyPartition(const Hypergraph& coarsaned_graph, std::vector<VertexId>& clusters,
                             Hypergraph& original_graph) {
    for (auto vertex_id : original_graph.nodes()) {
      original_graph.setNodePart(vertex_id, coarsaned_graph.partID(clusters[vertex_id]));
    }
  }
};
}
