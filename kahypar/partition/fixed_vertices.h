/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@gmx.net>
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
#include <cmath>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/context.h"

namespace kahypar {
namespace fixed_vertices {
using NodeID = int32_t;
using Flow = HyperedgeWeight;

using AdjacencyMatrix = std::vector<std::vector<HyperedgeWeight> >;
using Matching = std::vector<std::pair<PartitionID, PartitionID> >;
using VertexCover = std::vector<NodeID>;

static constexpr bool debug = false;
static constexpr bool debug_permutations = false;

// Verify, that the matching found is an valid matching.
static bool verify(const Matching& matching, const PartitionID k) {
  std::vector<bool> matched_left(k, false);
  std::vector<bool> matched_right(k, false);
  for (auto matched_edge : matching) {
    const PartitionID l = matched_edge.first;
    const PartitionID r = matched_edge.second;
    if (matched_left[l] || matched_right[r]) {
      return false;
    }
    matched_left[l] = true;
    matched_right[r] = true;
  }
  return true;
}


class BipartiteMaximumFlow {
 public:
  explicit BipartiteMaximumFlow(const AdjacencyMatrix& graph) :
    _num_nodes(2 * graph.size() + 2),
    _residualGraph(_num_nodes, std::vector<HypernodeWeight>(_num_nodes, 0)),
    _visited(_num_nodes),
    _parent(_num_nodes, -1),
    _source(_num_nodes - 2),
    _sink(_num_nodes - 1) {
    // Converts the bipartite graph representation into a general
    // adjacency matrix. Therefore nodes are relabeled.
    const PartitionID k = graph.size();
    for (PartitionID i = 0; i < k; ++i) {
      for (PartitionID j = 0; j < k; ++j) {
        if (graph[i][j]) {
          // All nodes from left side of the bipartite graph are labeled from 0..(k-1)
          // All nodes from right side of the bipartite graph are labeled from k..(2k-1)
          const NodeID u = i;
          const NodeID v = j + k;
          _residualGraph[u][v] = 1;
        }
      }
    }

    // Connect source to all left side nodes of the bipartite graph (Source Node: 2k)
    for (PartitionID i = 0; i < k; ++i) {
      _residualGraph[_source][i] = 1;
    }

    // Connect all right side nodes of the bipartite graph to sink (Sink Node: 2k + 1)
    for (PartitionID i = 0; i < k; ++i) {
      _residualGraph[i + k][_sink] = 1;
    }
  }

  BipartiteMaximumFlow(const BipartiteMaximumFlow&) = delete;
  BipartiteMaximumFlow(BipartiteMaximumFlow&&) = delete;
  BipartiteMaximumFlow& operator= (const BipartiteMaximumFlow&) = delete;
  BipartiteMaximumFlow& operator= (BipartiteMaximumFlow&&) = delete;
  ~BipartiteMaximumFlow() = default;

  /**
   * Edmond-Karps Maximum Flow Algorithm
   * Returns the value of the maximum flow.
   */
  Flow maximumFlow() {
    Flow max_flow = 0;
    while (bfs(_source)) {
      max_flow += augment(_sink);
    }
    ASSERT(!bfs(_source), "There still exists an augmenting path in the residual graph");
    return max_flow;
  }

 private:
  /**
   * Increases the flow along the augmenting path found
   * by the BFS traversal of the residual graph.
   * Returns the amount of flow which is send over
   * the augmenting path.
   * NOTE: Function should be called with sink node.
   */
  Flow augment(const NodeID u, const Flow flow = std::numeric_limits<Flow>::max()) {
    if (_parent[u] == -1) {
      return flow;
    }
    const Flow min_flow = augment(_parent[u], std::min(flow, _residualGraph[_parent[u]][u]));
    _residualGraph[_parent[u]][u] -= min_flow;
    _residualGraph[u][_parent[u]] += min_flow;
    return min_flow;
  }

 protected:
  /**
   * Breath-First-Search starting from a specific node
   * and searching for the sink node in the residual graph.
   * Returns true, if a path from node to _sink in the residual
   * graph exists.
   */
  bool bfs(const NodeID node, const bool reset = true) {
    if (reset) {
      _visited.reset();
    }
    std::queue<NodeID> q;
    q.push(node);
    _visited.set(node, true);
    _parent[node] = -1;

    while (!q.empty()) {
      const NodeID u = q.front();
      q.pop();
      if (u == _sink) {
        return true;
      }

      for (NodeID v = 0; v < _num_nodes; ++v) {
        if (!_visited[v] && _residualGraph[u][v]) {
          q.push(v);
          _visited.set(v, true);
          _parent[v] = u;
        }
      }
    }
    return false;
  }

  NodeID _num_nodes;
  AdjacencyMatrix _residualGraph;
  ds::FastResetFlagArray<> _visited;
  std::vector<NodeID> _parent;
  NodeID _source;
  NodeID _sink;
};

class MaximumBipartiteMatching : public BipartiteMaximumFlow {
 public:
  explicit MaximumBipartiteMatching(const AdjacencyMatrix& graph) :
    BipartiteMaximumFlow(graph) { }

  MaximumBipartiteMatching(const MaximumBipartiteMatching&) = delete;
  MaximumBipartiteMatching(MaximumBipartiteMatching&&) = delete;
  MaximumBipartiteMatching& operator= (const MaximumBipartiteMatching&) = delete;
  MaximumBipartiteMatching& operator= (MaximumBipartiteMatching&&) = delete;
  ~MaximumBipartiteMatching() = default;

  /**
   * Function computes a maximum matching in an unweighted
   * bipartite graph. There exists a duality between a
   * maximum flow and a maximum bipartite matching. The size
   * of a maximum bipartite matching is equal to the value of
   * the maximum flow f. Each edge (u,v), where f(u,v) = 1 is part
   * of the maximum bipartite matching.
   */
  Matching findMaximumBipartiteMatching() {
    Matching matching;
    const PartitionID k = _num_nodes / 2 - 1;

    const Flow max_flow = maximumFlow();
    unused(max_flow);

    // Iterate over L-vertices
    for (PartitionID i = 0; i < k; ++i) {
      // Iterate over R-vertices
      for (PartitionID j = 0; j < k; ++j) {
        // Map L- and R-vertices to their corresponding
        // nodes of the flow graph
        const NodeID u = i;
        const NodeID v = j + k;
        // If (i,j) is part of maximum bipartite matching,
        // then edge (v,u) must be in the residual graph
        // of a maximum flow.
        if (_residualGraph[v][u]) {
          matching.push_back(std::make_pair(i, j));
        }
      }
    }

    ASSERT(verify(matching, k), "Invalid matching");
    ASSERT(static_cast<size_t>(max_flow) == matching.size(),
           "Size of maximum matching not equal to value of maximum flow");

    return matching;
  }

 protected:
  using BipartiteMaximumFlow::_num_nodes;
  using BipartiteMaximumFlow::_residualGraph;
};

class MinimumBipartiteVertexCover final : public MaximumBipartiteMatching {
 public:
  explicit MinimumBipartiteVertexCover(const AdjacencyMatrix& graph) :
    MaximumBipartiteMatching(graph),
    _bipartite_graph(graph),
    _maximum_matching_already_calculated(false),
    _maximum_matching() { }

  MinimumBipartiteVertexCover(const MinimumBipartiteVertexCover&) = delete;
  MinimumBipartiteVertexCover(MinimumBipartiteVertexCover&&) = delete;
  MinimumBipartiteVertexCover& operator= (const MinimumBipartiteVertexCover&) = delete;
  MinimumBipartiteVertexCover& operator= (MinimumBipartiteVertexCover&&) = delete;
  ~MinimumBipartiteVertexCover() = default;

  /**
   * Function computes a minimum vertex cover in an unweighted
   * bipartite graph. There exists a duality between a
   * minimum vertex cover and a maximum bipartite matching.
   */
  VertexCover findMinimumBipartiteVertexCover() {
    VertexCover cover;
    const PartitionID k = _num_nodes / 2 - 1;

    computeMaximumBipartiteMatching();

    // Compute all unmatched left side vertices
    _visited.reset();
    for (auto matched_edge : _maximum_matching) {
      const NodeID l = matched_edge.first;
      _visited.set(l, true);
    }
    std::vector<NodeID> unmatched_vertices;
    for (NodeID u = 0; u < k; ++u) {
      if (!_visited[u]) {
        unmatched_vertices.push_back(u);
      }
    }

    // Remove source and sink from residual graph
    for (NodeID u = 0; u < k; ++u) {
      _residualGraph[_source][u] = 0;
      _residualGraph[u][_source] = 0;
      _residualGraph[u + k][_sink] = 0;
      _residualGraph[_sink][u + k] = 0;
    }

    // Construct set H of vertices reachable via an alternating
    // path from unmatched vertices of the left side.
    // NOTE: Set H is marked as visited after BFS
    _visited.reset();
    for (NodeID u : unmatched_vertices) {
      bfs(u, false);
    }
    // Let L be the left side and R be the right side
    // of the bipartite graph, then a minimum vertex
    // cover is defined as follows C := (L \ H) \cup (R \cap H)
    // L \ H:
    for (NodeID u = 0; u < k; ++u) {
      if (!_visited[u]) {
        cover.push_back(u);
      }
    }
    // R \cap H:
    for (NodeID u = k; u < 2 * k; ++u) {
      if (_visited[u]) {
        cover.push_back(u);
      }
    }

    ASSERT(cover.size() == _maximum_matching.size(),
           "Size of minimum vertex cover is not equal with size of the maximum matching");
    // Verify, that the vertex cover found is an valid vertex cover.
    ASSERT([&]() {
          // Remove all edges covered by the vertex cover
          for (NodeID u : cover) {
            if (u < k) {
              for (PartitionID i = 0; i < k; ++i) {
                _bipartite_graph[u][i] = 0;
              }
            } else {
              for (PartitionID i = 0; i < k; ++i) {
                _bipartite_graph[i][u - k] = 0;
              }
            }
          }
          // If there is still an edge left in the graph, than
          // our vertex cover is not an valid vertex cover
          for (PartitionID i = 0; i < k; ++i) {
            for (PartitionID j = 0; j < k; ++j) {
              if (_bipartite_graph[i][j]) {
                LOG << "Unmatched edge found (" << i << "," << j << ")";
                return false;
              }
            }
          }
          return true;
        } (), "Invalid vertex cover");
    return cover;
  }

  Matching computeMaximumBipartiteMatching() {
    if (!_maximum_matching_already_calculated) {
      _maximum_matching = findMaximumBipartiteMatching();
      _maximum_matching_already_calculated = true;
    }
    return _maximum_matching;
  }

 private:
  using BipartiteMaximumFlow::_num_nodes;
  using BipartiteMaximumFlow::_residualGraph;
  using BipartiteMaximumFlow::_visited;
  using BipartiteMaximumFlow::_source;
  using BipartiteMaximumFlow::_sink;

  AdjacencyMatrix _bipartite_graph;
  bool _maximum_matching_already_calculated;
  Matching _maximum_matching;
};

static inline AdjacencyMatrix setupWeightedBipartiteMatchingGraph(Hypergraph& input_hypergraph,
                                                                  const Context& original_context) {
  const PartitionID k = original_context.partition.k;
  AdjacencyMatrix graph(k, std::vector<HyperedgeWeight>(k, 0));

  std::vector<std::vector<HypernodeID> > fixed_vertices(k, std::vector<HypernodeID>());
  for (const HypernodeID& hn : input_hypergraph.fixedVertices()) {
    fixed_vertices[input_hypergraph.fixedVertexPartID(hn)].push_back(hn);
  }

  std::vector<PartitionID> fixed_connectivity(input_hypergraph.initialNumEdges(), 0);
  ds::FastResetFlagArray<> k_visited(k);
  for (const HyperedgeID& he : input_hypergraph.edges()) {
    k_visited.reset();
    for (const HypernodeID& pin : input_hypergraph.pins(he)) {
      if (input_hypergraph.isFixedVertex(pin)) {
        const PartitionID part = input_hypergraph.fixedVertexPartID(pin);
        if (!k_visited[part]) {
          fixed_connectivity[he]++;
          k_visited.set(part, true);
        }
      }
    }
  }

  ds::FastResetFlagArray<> visited(input_hypergraph.initialNumEdges());
  for (PartitionID i = 0; i < k; ++i) {
    visited.reset();
    for (const HypernodeID& hn : fixed_vertices[i]) {
      for (const HyperedgeID& he : input_hypergraph.incidentEdges(hn)) {
        if (!visited[he]) {
          if (original_context.partition.objective == Objective::km1) {
            // The km1 metric only increases if we would assign a fixed vertex
            // to a block not contained in the connectivity set of a hyperedge.
            // Therefore we increase the weight of all edges (i,j) in the
            // bipartite graph by the weight of the hyperedge, where j is a block
            // not contained in the connectivty set of the hyperedge.
            // Therefore a minimum weighted bipartitie matching optimizes
            // the km1 metric.
            // NOTE: Matchings are preferred to blocks, where a fixed vertex
            //       block is most connected to. Instead of increasing the
            //       weight by the hyperedge weight to blocks not contained
            //       in the connectivity set, we can increase the weight
            //       on all edges contained in the connectivity set and
            //       solve a maximum weighted bipartite matching problem
            //       to optimize the km1 metric (proposed by kPaToH)
            for (PartitionID j : input_hypergraph.connectivitySet(he)) {
              graph[i][j] += input_hypergraph.edgeWeight(he);
            }
          } else if (original_context.partition.objective == Objective::cut) {
            // The cut metric only increases if we would make a non-cut
            // hyperedge cut. Therefore we increase the weight on the edge
            // (i,j) by the weight of a non-cut hyperedge, where i is
            // the fixed vertex part and j the part contained in a non-cut
            // hyperedge. More general, if we would not assign the fixed
            // vertices of block i to block j, we would make that hyperedge
            // cut. Therefore, maximum weighted bipartite matching optimizes
            // the cut metric.
            if (input_hypergraph.connectivity(he) == 1 && fixed_connectivity[he] == 1) {
              for (PartitionID j : input_hypergraph.connectivitySet(he)) {
                graph[i][j] += input_hypergraph.edgeWeight(he);
              }
            }
          }
          visited.set(he, true);
        }
      }
    }
    // Discard assignment of fixed vertices to a block which violates
    // balanced contraint
    for (PartitionID j = 0; j < k; ++j) {
      if (input_hypergraph.fixedVertexPartWeight(i) + input_hypergraph.partWeight(j) >
          original_context.partition.max_part_weights[0]) {
        graph[i][j] = 0;
      }
    }
  }

  return graph;
}

static inline void printAdjacencyMatrix(const AdjacencyMatrix& graph, bool weighted = false) {
  if (debug) {
    const PartitionID k = graph.size();
    LOG << (weighted ? "WEIGHTED" : "UNWEIGHTED") << " MATCHING GRAPH:";
    for (PartitionID i = 0; i < k; ++i) {
      for (PartitionID j = 0; j < k; ++j) {
        LLOG << graph[i][j] << " ";
      }
      LOG << "";
    }
  }
}

static inline void printMatching(const Matching& matching) {
  if (debug) {
    DBG << "Calculated Matching (Size:" << matching.size() << "):";
    for (auto matched_edge : matching) {
      const PartitionID left_vertex = matched_edge.first;
      const PartitionID right_vertex = matched_edge.second;
      DBG << V(left_vertex) << V(right_vertex);
    }
  }
}
static inline void printVertexCover(const VertexCover& cover, const PartitionID k) {
  if (debug) {
    DBG << "Calculated Vertex Cover (Size:" << cover.size() << "):";
    for (const NodeID u : cover) {
      DBG << (u < k ? "Left-Side Vertex" : "Right-Side Vertex") << (u < k ? u : (u - k));
    }
  }
}


/**
 * This method implements the Hungarian Algorithm for solving
 * Maximum Weighted Bipartite Matching. It makes use of a duality
 * between the maximum weighted bipartite matching problem and the
 * problem to find a minimum weighted cover in a bipartite graph.
 *
 * The Minimum Weighted Cover Problem:
 * Given a bipartite graph G = (L u R,E,w) where L are the nodes
 * from the left side and R are the nodes from the right side
 * of the bipartite graph. A weighted cover is a choice of
 * labels u[1..|L|] and v[1..|R|] such that u[i] + v[j] >= w[i][j]
 * for all i,j. The cost of the cover is the sum of all labels.
 * The minimum weighted cover problem is to find a cover of
 * minimum cost.
 *
 * The basic idea is to construct a subgraph G' which contains
 * an edge (i,j) (with i \in L and j \in R) between each pair of
 * nodes where u[i] + v[j] = w[i][j]. If we find a perfect matching
 * in that subgraph, we are done and can report the maximum weighted
 * bipartite matching. Otherwise, we have to adapt the weights of
 * the cover u and v and repeat the procedure until we find a perfect
 * matching in the subgraph.
 *
 * Time complexity: O(k^3)
 *
 * Input:
 * The input of the function is an adjacency matrix which contains
 * the L-vertices as columns and the R-vertices as rows . A value
 * w := graph[i][j] indicates that there is an edge between L-vertex i
 * and R-vertex j with a weight of w.
 *
 * References:
 * Kuhn, Harold W.
 * "The Hungarian method for the assignment problem."
 * Naval Research Logistics (NRL) 2.1‐2 (1955): 83-97.
 *
 * The implementation is similiar as proposed in the following
 * online resource:
 * https://www.ti.inf.ethz.ch/ew/lehre/GT03/lectures/PDF/lecture6f.pdf
 */
static inline Matching findMaximumWeightedBipartiteMatching(const AdjacencyMatrix& graph) {
  const PartitionID k = graph.size();

  // Initialize Weighted Cover
  std::vector<HyperedgeWeight> u(k, 0);
  std::vector<HyperedgeWeight> v(k, 0);
  for (PartitionID i = 0; i < k; ++i) {
    u[i] = *std::max_element(graph[i].begin(), graph[i].end());
  }

  // Lambda function to construct the subgraph which contains
  // an edge between each pair of nodes (i,j) of the bipartite
  // graph where u[i] + v[j] == graph[i][j].
  auto construct_matching_graph = [&u, &v, &graph, k]() {
                                    AdjacencyMatrix matching_graph(k, std::vector<HyperedgeWeight>(k, 0));
                                    for (PartitionID i = 0; i < k; ++i) {
                                      for (PartitionID j = 0; j < k; ++j) {
                                        if (u[i] + v[j] == graph[i][j]) {
                                          matching_graph[i][j] = 1;
                                        }
                                      }
                                    }
                                    return matching_graph;
                                  };

  // Excess function which is important to adjust the
  // weights of the weighted cover.
  auto excess = [&u, &v, &graph](PartitionID i, PartitionID j) {
                  return u[i] + v[j] - graph[i][j];
                };

  Matching matching;
  size_t iteration = 1;
  while (true) {
    DBG << "Maximum Weighted Bipartite Matching - Iteration" << iteration;
    if (debug) {
      for (PartitionID i = 0; i < k; ++i) {
        DBG << V(i) << V(u[i]) << V(v[i]);
      }
    }

    ASSERT([&]() {
          for (PartitionID i = 0; i < k; ++i) {
            for (PartitionID j = 0; j < k; ++j) {
              if (u[i] + v[j] < graph[i][j]) {
                LOG << V(i) << V(j) << "=>" << V(u[i]) << "+" << V(v[j]) << ">=" << V(graph[i][j]);
                return false;
              }
            }
          }
          return true;
        } (), "Minimum weighted cover invariant violated");

    // Compute matching subgraph
    AdjacencyMatrix matching_graph = construct_matching_graph();
    printAdjacencyMatrix(matching_graph);

    // Compute maximum matching
    // NOTE: matching_graph is a unweighted bipartite graph
    MinimumBipartiteVertexCover vertex_cover(matching_graph);
    matching = vertex_cover.computeMaximumBipartiteMatching();
    printMatching(matching);

    if (matching.size() == static_cast<size_t>(k)) {
      // If we found a perfect matching we are done and
      // report the maximum bipartite weighted matching
      DBG << "Maximum bipartite weighted matching found";
      break;
    } else {
      // Otherwise we have to adapt the weights of the
      // weighted cover.
      DBG << "Maximum matching is not a perfect matching";
      // Therefore we compute a minimum vertex cover in
      // the subgraph.
      VertexCover cover = vertex_cover.findMinimumBipartiteVertexCover();
      printVertexCover(cover, k);

      // (Un)Matched vertices (not contained in the vertex cover)
      // from left and right side of bipartite graph
      std::vector<bool> L(k, false);
      std::vector<bool> R(k, false);
      for (NodeID u : cover) {
        if (u < k) {
          L[u] = true;
        } else {
          R[u - k] = true;
        }
      }

      // Compute minimum excess between all vertices not in the vertex cover
      HyperedgeWeight min_excess = std::numeric_limits<HyperedgeWeight>::max();
      for (PartitionID i = 0; i < k; ++i) {
        for (PartitionID j = 0; j < k; ++j) {
          if (!L[i] && !R[j]) {
            min_excess = std::min(excess(i, j), min_excess);
          }
        }
      }
      DBG << V(min_excess);

      // Update weighted cover vectors
      for (PartitionID i = 0; i < k; ++i) {
        if (!L[i]) {
          u[i] -= min_excess;
        }
        if (R[i]) {
          v[i] += min_excess;
        }
      }
    }
    DBG << "================================================================";
    ++iteration;
  }

  ASSERT(verify(matching, k), "Invalid matching");

  return matching;
}

static inline void applyPermutation(const std::vector<PartitionID>& permutation,
                                    const std::vector<PartitionID>& original_partition,
                                    Hypergraph& hypergraph) {
  for (const HypernodeID& hn : hypergraph.nodes()) {
    if (!hypergraph.isFixedVertex(hn)) {
      const PartitionID from = hypergraph.partID(hn);
      const PartitionID to = permutation[original_partition[hn]];
      if (from != to) {
        hypergraph.changeNodePart(hn, from, to);
      }
    }
  }
}

static inline void partition(Hypergraph& input_hypergraph,
                             const Context& original_context) {
  ASSERT([&]() {
        for (const HypernodeID& hn : input_hypergraph.nodes()) {
          if (input_hypergraph.isFixedVertex(hn) &&
              input_hypergraph.partID(hn) != Hypergraph::kInvalidPartition) {
            LOG << "Hypernode" << hn << "is a fixed vertex and already partitioned";
            return false;
          } else if (!input_hypergraph.isFixedVertex(hn) &&
                     input_hypergraph.partID(hn) == Hypergraph::kInvalidPartition) {
            LOG << "Hypernode" << hn << "is not a fixed vertex and unpartitioned";
            return false;
          }
        }
        return true;
      } (), "Precondition check for fixed vertex assignment failed");

  // Idea: Fixed vertices assigned to a fixed block should be assigned to a part
  // of the current partition in which the increase in the objective function is minimal.
  // However several fixed vertex sets might be assigned to the same part of the current
  // partition such that the increase is minimal, which is obviously not possible.
  // Therefore we try to find a permutation of the partition ids such that the assignment
  // of the fixed vertices is optimal among all possible permutations.
  // Reference:
  // Aykanat, Cevdet, B. Barla Cambazoglu, and Bora Uçar.
  // "Multi-level direct k-way hypergraph partitioning with multiple constraints and fixed vertices."
  // Journal of Parallel and Distributed Computing 68.5 (2008): 609-625.
  AdjacencyMatrix graph = setupWeightedBipartiteMatchingGraph(input_hypergraph, original_context);
  printAdjacencyMatrix(graph, true);

  Matching maximum_weighted_matching = findMaximumWeightedBipartiteMatching(graph);
  ASSERT(maximum_weighted_matching.size() == static_cast<size_t>(original_context.partition.k),
         "Matching is not a perfect matching");

  std::vector<PartitionID> partition_permutation(original_context.partition.k, 0);

  if (original_context.initial_partitioning.verbose_output) {
    LOG << "Computed Permutation:";
  }

  HypernodeWeight matching_weight = 0;
  for (auto matched_edge : maximum_weighted_matching) {
    const PartitionID from = matched_edge.second;
    const PartitionID to = matched_edge.first;
    partition_permutation[from] = to;
    if (debug || original_context.initial_partitioning.verbose_output) {
      LOG << "Block" << from << "assigned to fixed vertices with id"
          << to << "with weight" << graph[to][from];
      matching_weight += graph[to][from];
    }
  }
  if (debug || original_context.initial_partitioning.verbose_output) {
    LOG << "Weight of matching  :" << matching_weight;

    // In this DEBUG code, we compute the best and worst quality
    // achievable with a matching. Matchings are computed hyperedge-wise.
    HyperedgeWeight lower_bound = 0;
    HyperedgeWeight upper_bound = 0;
    std::vector<PartitionID> min_connectivity(input_hypergraph.initialNumEdges(), 0);
    std::vector<PartitionID> max_connectivity(input_hypergraph.initialNumEdges(), 0);
    ds::FastResetFlagArray<> k_visited(original_context.partition.k);
    for (const HyperedgeID& he : input_hypergraph.edges()) {
      k_visited.reset();
      ALWAYS_ASSERT(input_hypergraph.connectivity(he) >= 0,
                    V(he) << V(input_hypergraph.connectivity(he)));
      min_connectivity[he] = input_hypergraph.connectivity(he);
      for (const HypernodeID& pin : input_hypergraph.pins(he)) {
        if (input_hypergraph.isFixedVertex(pin)) {
          const PartitionID part = input_hypergraph.fixedVertexPartID(pin);
          if (!k_visited[part]) {
            max_connectivity[he]++;
            k_visited.set(part, true);
          }
        }
      }

      // If min_connectivity[he] == 0, this means that the hyperedge only contains fixed vertices.
      max_connectivity[he] += min_connectivity[he];
      min_connectivity[he] = std::max(min_connectivity[he], 1);

      if (original_context.partition.objective == Objective::cut) {
        lower_bound += (min_connectivity[he] > 1 ? input_hypergraph.edgeWeight(he) : 0);
        upper_bound += (max_connectivity[he] > 1 ? input_hypergraph.edgeWeight(he) : 0);
      } else if (original_context.partition.objective == Objective::km1) {
        lower_bound += (min_connectivity[he] - 1) * input_hypergraph.edgeWeight(he);
        upper_bound += (max_connectivity[he] - 1) * input_hypergraph.edgeWeight(he);
      }
    }
    LOG << "Lower Bound (" << original_context.partition.objective << ") :" << lower_bound;
    LOG << "Upper Bound (" << original_context.partition.objective << ") :" << upper_bound;
    LOG << "Final" << original_context.partition.objective << "=" << upper_bound << "-"
        << matching_weight << "=" << (upper_bound - matching_weight);
  }

  std::vector<PartitionID> original_partition;
  if (debug_permutations) {
    original_partition.resize(input_hypergraph.initialNumNodes(), 0);
    for (const HypernodeID& hn : input_hypergraph.nodes()) {
      if (!input_hypergraph.isFixedVertex(hn)) {
        original_partition[hn] = input_hypergraph.partID(hn);
      }
    }
  }

  for (const HypernodeID& hn : input_hypergraph.nodes()) {
    if (input_hypergraph.isFixedVertex(hn)) {
      // This explicit setNodePart is necessary since the fixed vertex part ID is stored
      // in a different vector than the actual part id of the partition.
      input_hypergraph.setNodePart(hn, input_hypergraph.fixedVertexPartID(hn));
    } else {
      const PartitionID from = input_hypergraph.partID(hn);
      const PartitionID to = partition_permutation[from];
      if (from != to) {
        input_hypergraph.changeNodePart(hn, from, to);
      }
    }
  }
  DBG << "OPT:" << original_context.partition.objective << "="
      << metrics::objective(input_hypergraph, original_context.partition.objective)
      << V(metrics::imbalance(input_hypergraph, original_context));

  if (debug_permutations) {
    std::vector<PartitionID> permutations(original_context.partition.k, 0);
    std::iota(permutations.begin(), permutations.end(), 0);
    do {
      for (PartitionID i = 0; i < original_context.partition.k; ++i) {
        LOG << V(i) << V(partition_permutation[i]);
      }

      applyPermutation(permutations, original_partition, input_hypergraph);

      DBG1 << original_context.partition.objective << "="
           << (original_context.partition.objective == Objective::cut ?
          metrics::hyperedgeCut(input_hypergraph) :
          metrics::km1(input_hypergraph))
           << V(metrics::imbalance(input_hypergraph, original_context));
    } while (std::next_permutation(permutations.begin(), permutations.end()));

    applyPermutation(partition_permutation, original_partition, input_hypergraph);
  }
}
}  // namespace fixed_vertices
}  // namespace kahypar
