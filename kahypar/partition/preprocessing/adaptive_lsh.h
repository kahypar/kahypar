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
#include <queue>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/hash_table.h"
#include "kahypar/definitions.h"
#include "kahypar/utils/hash_vector.h"

namespace kahypar {
template <typename _HashPolicy>
class AdaptiveLSHWithConnectedComponents {
 private:
  static constexpr bool debug = false;

  using HashPolicy = _HashPolicy;
  using BaseHashPolicy = typename HashPolicy::BaseHashPolicy;
  using HashValue = typename HashPolicy::HashValue;
  using MyHashSet = HashStorage<HashValue>;
  using Buckets = HashBuckets<HashValue, HypernodeID>;
  using Pair = std::pair<HashValue, HypernodeID>;

 public:
  explicit AdaptiveLSHWithConnectedComponents(const Hypergraph& hypergraph,
                                              const Context& context) :
    _hypergraph(hypergraph),
    _context(context),
    _bfs_neighbours(),
    _vertices(),
    _new_hashes(),
    _hash_set(0, _hypergraph.currentNumNodes()),
    _hashes(_hypergraph.currentNumNodes()),
    _buckets(),
    _new_buckets(),
    _base_hash_policy(0),
    _multiset_buckets(_hypergraph.currentNumNodes()),
    _visited(_hypergraph.currentNumNodes()) {
    _buckets.reserve(_hypergraph.currentNumNodes());
    _new_buckets.reserve(_hypergraph.currentNumNodes());
    _bfs_neighbours.reserve(_context.preprocessing.min_hash_sparsifier.max_hyperedge_size);
    _hash_set.reserve(_context.preprocessing.min_hash_sparsifier.combined_num_hash_functions);
    _base_hash_policy.reserveHashFunctions(
      _context.preprocessing.min_hash_sparsifier.combined_num_hash_functions);
  }

  std::vector<HypernodeID> build() {
    std::default_random_engine eng(_context.partition.seed);
    std::uniform_int_distribution<uint32_t> rnd;

    MyHashSet main_hash_set(0, _hypergraph.currentNumNodes());
    main_hash_set.reserve(20);

    std::vector<HypernodeID> clusters;
    clusters.reserve(_hypergraph.currentNumNodes());

    std::vector<uint32_t> cluster_size(_hypergraph.currentNumNodes(), 1);

    std::vector<uint8_t> active_clusters_bool_set(_hypergraph.currentNumNodes(), true);

    const HypernodeID num_free_vertices = _hypergraph.currentNumNodes() -
                                          _hypergraph.numFixedVertices();
    HypernodeID num_active_vertices = num_free_vertices;

    for (HypernodeID vertex_id = 0; vertex_id < _hypergraph.currentNumNodes(); ++vertex_id) {
      clusters.push_back(vertex_id);
    }

    std::vector<HypernodeID> inactive_clusters;
    inactive_clusters.reserve(_hypergraph.currentNumNodes());

    std::vector<HypernodeID> active_vertices_set;
    active_vertices_set.reserve(num_active_vertices);

    while (num_active_vertices > 0 && main_hash_set.getHashNum() <
           _context.preprocessing.min_hash_sparsifier.num_hash_functions) {
      main_hash_set.addHashVector();
      active_vertices_set.clear();

      for (HypernodeID vertex_id = 0; vertex_id < _hypergraph.currentNumNodes(); ++vertex_id) {
        if (!_hypergraph.isFixedVertex(vertex_id)) {
          const HypernodeID cluster = clusters[vertex_id];
          if (active_clusters_bool_set[cluster]) {
            active_vertices_set.push_back(vertex_id);
          }
        } else {
          active_clusters_bool_set[clusters[vertex_id]] = false;
        }
      }

      const uint32_t hash_num = main_hash_set.getHashNum() - 1;
      incrementalParametersEstimation(active_vertices_set, rnd(eng), main_hash_set, hash_num);

      _multiset_buckets.clear();
      calculateOneDimBucket(_hypergraph.currentNumNodes(),
                            active_clusters_bool_set, clusters, main_hash_set, hash_num);

      calculateClustersIncrementally(active_clusters_bool_set,
                                     clusters, cluster_size,
                                     main_hash_set, hash_num, inactive_clusters);

      std::vector<char> bit_map(clusters.size());
      for (const auto& clst : clusters) {
        bit_map[clst] = 1;
      }

      size_t num_cl = 0;
      for (const auto& bit : bit_map) {
        if (bit) {
          ++num_cl;
        }
      }
      DBG << "Num clusters:" << num_cl;

      std::sort(inactive_clusters.begin(), inactive_clusters.end());
      auto end_iter = std::unique(inactive_clusters.begin(), inactive_clusters.end());
      inactive_clusters.resize(end_iter - inactive_clusters.begin());

      for (const auto& cluster : inactive_clusters) {
        active_clusters_bool_set[cluster] = false;
        num_active_vertices -= cluster_size[cluster];
      }
      inactive_clusters.clear();

      if (num_cl <= num_free_vertices / 2) {
        if (!_context.partition.quiet_mode) {
          LOG << "Adaptively chosen number of hash functions:" << main_hash_set.getHashNum();
        }
        break;
      }
    }

    return clusters;
  }

 private:
  void incrementalParametersEstimation(std::vector<HypernodeID>& active_vertices,
                                       const uint32_t seed, MyHashSet& main_hash_set,
                                       const uint32_t main_hash_num) {
    _hash_set.clear();
    _base_hash_policy.clear();
    _base_hash_policy.setSeed(seed);

    std::default_random_engine eng(seed);
    std::uniform_int_distribution<uint32_t> rnd;

    // in the beginning all vertices are in the same bucket (same hash)
    std::fill(_hashes.begin(), _hashes.end(), 0);

    size_t remained_vertices = active_vertices.size();

    _buckets.clear();

    // empirically best value
    const uint32_t min_hash_num = 10;

    const uint32_t max_bucket_size = _context.preprocessing.min_hash_sparsifier.max_cluster_size;
    ASSERT(min_hash_num <= _context.preprocessing.min_hash_sparsifier.combined_num_hash_functions,
           "# min combined hash funcs should be <= # max combined hash funcs");

    for (size_t i = 0; i + 1 < min_hash_num; ++i) {
      _hash_set.addHashVector();
      _base_hash_policy.addHashFunction(rnd(eng));

      _base_hash_policy.calculateLastHash(_hypergraph, active_vertices, _hash_set);

      const uint32_t last_hash = _hash_set.getHashNum() - 1;
      for (const auto& ver : active_vertices) {
        _hashes[ver] ^= _hash_set[last_hash][ver];
      }
    }

    for (const auto& ver : active_vertices) {
      _buckets.emplace_back(_hashes[ver], ver);
    }
    std::sort(_buckets.begin(), _buckets.end());

    while (remained_vertices > 0) {
      _hash_set.addHashVector();
      _base_hash_policy.addHashFunction(rnd(eng));

      const uint32_t last_hash = _hash_set.getHashNum() - 1;

      // Decide for which vertices we continue to increase the number of hash functions
      _new_buckets.clear();

      auto begin = _buckets.begin();

      while (begin != _buckets.end()) {
        auto end = begin;
        while (end != _buckets.end() && begin->first == end->first) {
          ++end;
        }

        _vertices.clear();
        for (auto it = begin; it != end; ++it) {
          _vertices.push_back(it->second);
        }

        _base_hash_policy.calculateLastHash(_hypergraph, _vertices, _hash_set);

        if (_vertices.size() == 1) {
          --remained_vertices;
          const HashValue hash = _hash_set[last_hash][_vertices.front()];
          _hashes[_vertices.front()] ^= hash;
          begin = end;
          continue;
        }

        _new_hashes.clear();

        for (const auto& vertex : _vertices) {
          const HashValue hash = _hash_set[last_hash][vertex];
          _hashes[vertex] ^= hash;

          _new_hashes.emplace_back(_hashes[vertex], vertex);
        }

        std::sort(_new_hashes.begin(), _new_hashes.end());
        const auto e = std::unique(_new_hashes.begin(), _new_hashes.end());
        _new_hashes.resize(e - _new_hashes.begin());

        auto new_begin = _new_hashes.begin();
        while (new_begin != _new_hashes.end()) {
          auto new_end = new_begin;
          while (new_end != _new_hashes.end() && new_begin->first == new_end->first) {
            ++new_end;
          }
          const HypernodeID bucket_size = new_end - new_begin;
          if ((bucket_size <= max_bucket_size && _hash_set.getHashNum() >= min_hash_num) ||
              _hash_set.getHashNum() >=
              _context.preprocessing.min_hash_sparsifier.combined_num_hash_functions
              ) {
            remained_vertices -= bucket_size;
          } else {
            std::copy(new_begin, new_end, std::back_inserter(_new_buckets));
          }
          new_begin = new_end;
        }
        begin = end;
      }
      _buckets.swap(_new_buckets);
    }
    for (uint32_t vertex_id = 0; vertex_id < _hypergraph.currentNumNodes(); ++vertex_id) {
      main_hash_set[main_hash_num][vertex_id] = _hashes[vertex_id];
    }
  }

  // distributes vertices among bucket according to the new hash values
  void calculateOneDimBucket(const HypernodeID num_vertex,
                             const std::vector<uint8_t>& active_clusters_bool_set,
                             const std::vector<HypernodeID>& clusters, const MyHashSet& hash_set,
                             const uint32_t hash_num) {
    for (HypernodeID vertex_id = 0; vertex_id < num_vertex; ++vertex_id) {
      const HypernodeID cluster = clusters[vertex_id];
      if (active_clusters_bool_set[cluster]) {
        _multiset_buckets.put(hash_set[hash_num][vertex_id], vertex_id);
      }
    }
  }

  // incrementally calculates clusters according to the new bucket
  void calculateClustersIncrementally(std::vector<uint8_t>& active_clusters_bool_set,
                                      std::vector<HypernodeID>& clusters,
                                      std::vector<uint32_t>& cluster_size,
                                      const MyHashSet& hash_set, const uint32_t hash_num,
                                      std::vector<HypernodeID>& inactive_clusters) {
    // Calculate clusters according to connected components
    // of an implicit graph induced by buckets. We use BFS on this graph.
    runIncrementalBfs(active_clusters_bool_set, hash_set, hash_num,
                      clusters, cluster_size, inactive_clusters);
  }

  void runIncrementalBfs(std::vector<uint8_t>& active_clusters_bool_set,
                         const MyHashSet& hash_set, const uint32_t hash_num,
                         std::vector<HypernodeID>& clusters, std::vector<uint32_t>& cluster_size,
                         std::vector<HypernodeID>& inactive_clusters) {
    _visited.reset();
    for (HypernodeID vertex_id = 0; vertex_id < _hypergraph.currentNumNodes(); ++vertex_id) {
      const HypernodeID cluster = clusters[vertex_id];
      if (!_visited[vertex_id] && active_clusters_bool_set[cluster]) {
        _visited.set(vertex_id, true);
        runIncrementalBfs(vertex_id, active_clusters_bool_set, hash_set,
                          hash_num, clusters, cluster_size, inactive_clusters);
      }
    }
  }

  void runIncrementalBfs(const HypernodeID cur_vertex, std::vector<uint8_t>& active_clusters_bool_set,
                         const MyHashSet& hash_set,
                         const uint32_t hash_num, std::vector<HypernodeID>& clusters,
                         std::vector<uint32_t>& cluster_size,
                         std::vector<HypernodeID>& inactive_clusters) {
    const HypernodeID cur_cluster = clusters[cur_vertex];
    const HashValue hash = hash_set[hash_num][cur_vertex];

    _bfs_neighbours.clear();

    const auto iters = _multiset_buckets.getObjects(hash);
    for (auto neighbour_iter = iters.first; neighbour_iter != iters.second; ++neighbour_iter) {
      const auto neighbour = *neighbour_iter;
      const HypernodeID neighbour_cluster = clusters[neighbour];
      if (active_clusters_bool_set[neighbour_cluster]) {
        const HypernodeWeight weight = _hypergraph.nodeWeight(neighbour);
        if (cluster_size[cur_cluster] + weight >
            _context.preprocessing.min_hash_sparsifier.max_cluster_size) {
          ASSERT(cluster_size[cur_cluster] <=
                 _context.preprocessing.min_hash_sparsifier.max_cluster_size,
                 "Cluster is overflowed");
          break;
        }

        cluster_size[neighbour_cluster] -= weight;
        clusters[neighbour] = cur_cluster;
        cluster_size[cur_cluster] += weight;

        if (cluster_size[cur_cluster] >=
            _context.preprocessing.min_hash_sparsifier.min_cluster_size) {
          inactive_clusters.push_back(cur_cluster);
          active_clusters_bool_set[cur_cluster] = false;
        }

        _visited.set(neighbour, true);
        _bfs_neighbours.push_back(neighbour);
      }
    }

    for (const auto& neighbour : _bfs_neighbours) {
      _multiset_buckets.removeObject(hash_set[hash_num][neighbour], neighbour);
    }
  }

  const Hypergraph& _hypergraph;
  const Context& _context;
  std::vector<HypernodeID> _bfs_neighbours;
  std::vector<HypernodeID> _vertices;
  std::vector<Pair> _new_hashes;
  MyHashSet _hash_set;
  std::vector<HashValue> _hashes;
  std::vector<Pair> _buckets;
  std::vector<Pair> _new_buckets;
  BaseHashPolicy _base_hash_policy;
  Buckets _multiset_buckets;
  ds::FastResetFlagArray<> _visited;
};
}  // namespace kahypar
