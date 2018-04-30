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
#include <limits>
#include <utility>
#include <vector>

#include "kahypar/datastructure/hash_table.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/preprocessing/adaptive_lsh.h"
#include "kahypar/partition/preprocessing/policies/min_hash_policy.h"

namespace kahypar {
class MinHashSparsifier {
 private:
  struct Edge {
    Edge() :
      _begin(),
      _end() { }

    Edge(const IncidenceIterator begin, const IncidenceIterator end) :
      _begin(begin),
      _end(end) { }

    IncidenceIterator begin() const {
      return _begin;
    }

    IncidenceIterator end() const {
      return _end;
    }

    bool operator== (const Edge& edge) const {
      const size_t this_size = _end - _begin;
      const size_t other_size = edge._end - edge._begin;

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

    bool operator!= (const Edge& edge) const {
      return !(*this == edge);
    }

    bool operator< (const Edge& edge) const {
      const size_t this_size = _end - _begin;
      const size_t other_size = edge._end - edge._begin;

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

    IncidenceIterator _begin;
    IncidenceIterator _end;
  };

 public:
  MinHashSparsifier() :
    _clusters() { }

  Hypergraph buildSparsifiedHypergraph(const Hypergraph& hypergraph, const Context& context) {
    AdaptiveLSHWithConnectedComponents<LSHCombinedHashPolicy> sparsifier(hypergraph, context);

    _clusters = sparsifier.build();

    ASSERT([&]() {
        // Fixed vertices are not allowed to be sparsified
        for (const HypernodeID hn : hypergraph.fixedVertices()) {
          ASSERT(_clusters[hn] == hn);
          ASSERT(std::count(_clusters.begin(), _clusters.end(), hn) == 1);
        }
        return true;
      } ());

    HyperedgeID num_edges = hypergraph.currentNumEdges();

    // 0 : reenumerate
    const HypernodeID num_vertices = reenumerateClusters();

    // 1 : build vector of indicies and pins for hyperedges
    std::vector<size_t> indices_of_edges;
    indices_of_edges.reserve(static_cast<size_t>(num_edges) + 1);

    std::vector<HypernodeID> pins_of_edges;
    pins_of_edges.reserve(hypergraph.currentNumPins());


    struct HashEdge {
      size_t operator() (const Edge& edge) const {
        size_t hash = 0;
        math::MurmurHash<uint32_t> hash_func;
        for (const auto& edge_id : edge) {
          hash ^= hash_func(edge_id);
        }
        return hash;
      }
    };

    ds::InsertOnlyHashMap<Edge, std::pair<HyperedgeID, HyperedgeWeight>,
                          HashEdge, false> parallel_edges(3 * num_edges);

    size_t offset = 0;
    size_t removed_edges = 0;
    size_t non_disabled_edge_id = 0;
    for (const auto& edge_id : hypergraph.edges()) {
      auto pins_range = hypergraph.pins(edge_id);
      ds::InsertOnlyHashSet<HypernodeID> new_pins(pins_range.second - pins_range.first);

      for (const auto& vertex_id : pins_range) {
        new_pins.insert(_clusters[vertex_id]);
      }

      // if we have only one vertex in a hyperedge then we remove the hyperedge
      if (new_pins.size() <= 1) {
        ++removed_edges;
        ++non_disabled_edge_id;
        continue;
      }

      for (const auto& new_pin : new_pins) {
        pins_of_edges.push_back(new_pin);
      }

      // check for parallel edges
      const Edge edge(pins_of_edges.begin() + offset,
                      pins_of_edges.begin() + offset + new_pins.size());

      if (!parallel_edges.contains(edge)) {
        // no parallel edges
        indices_of_edges.push_back(offset);
        offset += new_pins.size();
        parallel_edges.insert(std::make_pair(edge, std::make_pair(non_disabled_edge_id - removed_edges,
                                                                  hypergraph.edgeWeight(edge_id))));
      } else {
        parallel_edges[edge].second += hypergraph.edgeWeight(edge_id);

        ++removed_edges;
        pins_of_edges.resize(pins_of_edges.size() - new_pins.size());
      }
      ++non_disabled_edge_id;
    }
    indices_of_edges.push_back(offset);

    // 2 : calculate weights of vertices in contracted graph
    std::vector<HypernodeWeight> vertex_weights(num_vertices);

    for (const auto& vertex : hypergraph.nodes()) {
      vertex_weights[_clusters[vertex]] += hypergraph.nodeWeight(vertex);
    }

    // 3 : calculate weights of hyper edgs in contracted graph
    num_edges -= removed_edges;
    std::vector<HyperedgeWeight> edge_weights(num_edges);
    for (const auto& value : parallel_edges) {
      edge_weights[value.second.first] = value.second.second;
    }

    Hypergraph sparse_hypergraph(num_vertices, num_edges, indices_of_edges, pins_of_edges,
                                 context.partition.k, &edge_weights, &vertex_weights);

    // update fixed vertex IDs
    for (const HypernodeID hn : hypergraph.fixedVertices()) {
      sparse_hypergraph.setFixedVertex(_clusters[hn], hypergraph.fixedVertexPartID(hn));
    }
    ASSERT(sparse_hypergraph.numFixedVertices() == hypergraph.numFixedVertices());

    return sparse_hypergraph;
  }

  void applyPartition(const Hypergraph& coarsaned_graph, Hypergraph& original_graph) {
    for (const auto& vertex_id : original_graph.nodes()) {
      original_graph.setNodePart(vertex_id, coarsaned_graph.partID(_clusters[vertex_id]));
    }
  }

  const std::vector<HypernodeID> & hnToSparsifiedHnMapping() const {
    return _clusters;
  }

 private:
  HypernodeID reenumerateClusters() {
    if (_clusters.empty()) {
      return 0;
    }

    std::vector<HypernodeID> new_clusters(_clusters.size());

    for (const auto& clst : _clusters) {
      new_clusters[clst] = 1;
    }

    for (size_t i = 1; i < new_clusters.size(); ++i) {
      new_clusters[i] += new_clusters[i - 1];
    }

    for (HypernodeID& cluster : _clusters) {
      cluster = new_clusters[cluster] - 1;
    }

    return new_clusters.back();
  }

  std::vector<HypernodeID> _clusters;
};
}  // namespace kahypar
