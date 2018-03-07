/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
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
#include <array>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_array.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/flow_network.h"
#include "kahypar/datastructure/graph.h"
#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"

namespace kahypar {
using ds::Graph;
using ds::Edge;

template <class Network = Mandatory>
class MostBalancedMinimumCut {
 public:
  MostBalancedMinimumCut(Hypergraph& hypergraph,
                         const Context& context,
                         Network& flowNetwork) :
    _hg(hypergraph),
    _context(context),
    _flow_network(flowNetwork),
    _visited(_hg.initialNumNodes() + 2 * _hg.initialNumEdges()),
    _graph_to_flow_network(flowNetwork.initialSize(), INVALID_NODE),
    _flow_network_to_graph(flowNetwork.initialSize(), INVALID_NODE),
    _scc_node_weight(flowNetwork.initialSize(), 0),
    _Q(),
    _S(),
    _dfs_low(flowNetwork.initialSize(), 0) { }

  MostBalancedMinimumCut(const MostBalancedMinimumCut&) = delete;
  MostBalancedMinimumCut(MostBalancedMinimumCut&&) = delete;
  MostBalancedMinimumCut& operator= (const MostBalancedMinimumCut&) = delete;
  MostBalancedMinimumCut& operator= (MostBalancedMinimumCut&&) = delete;

  void mostBalancedMinimumCut(const PartitionID block_0, const PartitionID block_1) {
    reset();

    // Mark all reachable nodes from source and sink set as invalid
    markAllReachableNodesAsVisited<true>(block_0, block_1);
    markAllReachableNodesAsVisited<false>(block_0, block_1);

    // Build residual graph
    Graph residual_graph(std::move(buildResidualGraph()));

    // Find strongly connected components
    findStronglyConnectedComponents(residual_graph);

    // Contract strongly connected components
    auto contraction = residual_graph.contractClusters();
    Graph dag(std::move(contraction.first));
    std::vector<NodeID> contraction_mapping = std::move(contraction.second);

    // Build mapping from contracted graph to flow network
    std::vector<std::vector<NodeID> > scc_to_flow_network(dag.numNodes(), std::vector<NodeID>());
    for (const NodeID& u : residual_graph.nodes()) {
      const NodeID flow_u = _graph_to_flow_network.get(u);
      if (_flow_network.isHypernode(flow_u)) {
        scc_to_flow_network[contraction_mapping[u]].push_back(flow_u);
        _scc_node_weight.update(contraction_mapping[u], _hg.nodeWeight(flow_u));
      }
    }

    // Calculate in degrees of nodes in DAG graph for topological ordering
    std::vector<size_t> in_degree(dag.numNodes(), 0);
    for (const NodeID& u : dag.nodes()) {
      for (const Edge& e : dag.incidentEdges(u)) {
        const NodeID v = e.target_node;
        if (u != v) {
          in_degree[v]++;
        }
      }
    }

    // Find most balanced minimum cut
    std::vector<NodeID> topological_order(dag.numNodes(), 0);
    std::vector<PartitionID> best_partition_id(dag.numNodes(), block_0);
    double best_imbalance = metrics::imbalance(_hg, _context);

    DBG << "Start Most Balanced Minimum Cut (Bipartition = {" << block_0 << "," << block_1 << "}";
    DBG << "Initial imbalance: " << V(metrics::imbalance(_hg, _context));

    for (size_t i = 0; i < 20; ++i) {
      // Compute random topological order
      topologicalSort(dag, in_degree, topological_order);

      // Sweep through topological order and find best imbalance
      std::vector<PartitionID> tmp_partition_id(dag.numNodes(), block_0);
      double tmp_best_imbalance = metrics::imbalance(_hg, _context);

      std::vector<HypernodeWeight> part_weight(_context.partition.k, 0);
      for (PartitionID part = 0; part < _context.partition.k; ++part) {
        part_weight[part] = _hg.partWeight(part);
      }
      for (size_t idx = 0; idx < topological_order.size(); ++idx) {
        const NodeID u = topological_order[idx];
        tmp_partition_id[u] = block_1;
        part_weight[block_0] -= _scc_node_weight.get(u);
        part_weight[block_1] += _scc_node_weight.get(u);
        double cur_imbalance = imbalance<true>(part_weight);
        if (_context.partition.k > 2) {
          cur_imbalance = imbalance<false>(part_weight);
        }

        if (cur_imbalance > tmp_best_imbalance) {
          tmp_partition_id[u] = block_0;
          break;
        }
        tmp_best_imbalance = cur_imbalance;
      }

      if (tmp_best_imbalance < best_imbalance) {
        best_imbalance = tmp_best_imbalance;
        best_partition_id = tmp_partition_id;
      }
    }

    DBG << "Best imbalance: " << best_imbalance;

    ASSERT([&]() {
        const HyperedgeWeight metric_before = metrics::objective(_hg, _context.partition.objective);
        const double imbalance_before = metrics::imbalance(_hg, _context);
        std::vector<NodeID> topological_order(dag.numNodes(), 0);
        std::vector<NodeID> part_before(dag.numNodes(), block_0);
        topologicalSort(dag, in_degree, topological_order);
        for (const NodeID& u : topological_order) {
          for (const NodeID& v : scc_to_flow_network[u]) {
            const PartitionID from = _hg.partID(v);
            const PartitionID to = best_partition_id[u];
            if (from != to) {
              _hg.changeNodePart(v, from, to);
              part_before[u] = from;
            }
          }
          // Check cut after assignment of an SCC
          // Should be the same as the starting cut
          const HyperedgeWeight metric_after = metrics::objective(_hg, _context.partition.objective);
          if (metric_after != metric_before) {
            LOG << "Assignment of SCC leads to inconsistent hyperedge cut!";
            LOG << V(metric_before) << V(metric_after);
            return false;
          }
        }

        // Rollback hypernode assignment
        for (const NodeID& u : dag.nodes()) {
          for (const NodeID& v : scc_to_flow_network[u]) {
            const PartitionID from = _hg.partID(v);
            const PartitionID to = part_before[u];
            if (from != to) {
              _hg.changeNodePart(v, from, to);
            }
          }
        }

        const HyperedgeWeight metric = metrics::objective(_hg, _context.partition.objective);
        if (metric != metric_before ||
            metrics::imbalance(_hg, _context) != imbalance_before) {
          LOG << "Restoring original partition failed!";
          LOG << V(metric_before) << V(metric);
          LOG << V(imbalance_before) << V(metrics::imbalance(_hg, _context));
          return false;
        }

        return true;
      } (), "Most balanced minimum cut failed!");

    // Assign most balanced minimum cut
    for (const NodeID& u : dag.nodes()) {
      for (const NodeID& v : scc_to_flow_network[u]) {
        const PartitionID from = _hg.partID(v);
        const PartitionID to = best_partition_id[u];
        if (from != to) {
          _hg.changeNodePart(v, from, to);
        }
      }
    }

    ASSERT(best_imbalance == metrics::imbalance(_hg, _context),
           "Best imbalance didn't match with current imbalance"
           << V(best_imbalance) << V(metrics::imbalance(_hg, _context)));
  }

 private:
  static constexpr bool debug = false;

  void reset() {
    _visited.reset();
    _graph_to_flow_network.resetUsedEntries();
    _flow_network_to_graph.resetUsedEntries();
    _scc_node_weight.resetUsedEntries();
    _dfs_low.resetUsedEntries();
  }


  /**
   * Executes a BFS starting from the source (sourceSet = true)
   * or sink set (sourceSet = false). Touched nodes by BFS
   * are marked as visited and are not considered during
   * residual graph building step.
   *
   * @t_param sourceSet Indicates, if BFS start from source or sink set
   */
  template <bool sourceSet>
  void markAllReachableNodesAsVisited(const PartitionID block_0, const PartitionID block_1) {
    auto start_set_iterator = sourceSet ? _flow_network.sources() : _flow_network.sinks();
    for (const NodeID& node : start_set_iterator) {
      _Q.push(node);
      _visited.set(node, true);
    }

    while (!_Q.empty()) {
      const NodeID u = _Q.front();
      _Q.pop();

      if (_flow_network.interpreteHypernode(u)) {
        if (!sourceSet) {
          const PartitionID from = _hg.partID(u);
          if (from == block_0) {
            _hg.changeNodePart(u, block_0, block_1);
          }
        }
      } else if (_flow_network.interpreteHyperedge(u, sourceSet)) {
        const HyperedgeID he = _flow_network.mapToHyperedgeID(u);
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (_flow_network.containsHypernode(pin)) {
            if (!sourceSet) {
              PartitionID from = _hg.partID(pin);
              if (from == block_0) {
                _hg.changeNodePart(pin, block_0, block_1);
              }
            }
            if (_flow_network.isRemovedHypernode(pin)) {
              _visited.set(pin, true);
            }
          }
        }
      }

      for (FlowEdge& e : _flow_network.incidentEdges(u)) {
        const FlowEdge& reverse_edge = _flow_network.reverseEdge(e);
        const NodeID v = e.target;
        if (!_visited[v]) {
          if ((sourceSet && _flow_network.residualCapacity(e)) ||
              (!sourceSet && _flow_network.residualCapacity(reverse_edge))) {
            _Q.push(v);
            _visited.set(v, true);
          }
        }
      }
    }
  }

  Graph buildResidualGraph() {
    size_t cur_graph_node = 0;
    for (const NodeID& node : _flow_network.nodes()) {
      if (!_visited[node]) {
        _graph_to_flow_network.set(cur_graph_node, node);
        _flow_network_to_graph.set(node, cur_graph_node++);
      }
    }

    for (const HypernodeID& hn : _flow_network.removedHypernodes()) {
      if (!_visited[hn]) {
        _graph_to_flow_network.set(cur_graph_node, hn);
        _flow_network_to_graph.set(hn, cur_graph_node++);
      }
    }

    std::vector<std::vector<Edge> > adj_list(cur_graph_node, std::vector<Edge>());

    for (const NodeID& node : _flow_network.nodes()) {
      if (!_visited[node]) {
        const NodeID source = _flow_network_to_graph.get(node);
        for (FlowEdge& flow_edge : _flow_network.incidentEdges(node)) {
          const NodeID target = flow_edge.target;
          if (_flow_network.residualCapacity(flow_edge) && !_visited[target]) {
            Edge e;
            e.target_node = _flow_network_to_graph.get(target);
            e.weight = 1.0;
            adj_list[source].push_back(e);
          }
        }
      }
    }

    for (const HypernodeID& hn : _flow_network.removedHypernodes()) {
      if (!_visited[hn]) {
        const NodeID hn_node = _flow_network_to_graph.get(hn);
        for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
          const NodeID in_he = _flow_network_to_graph.get(_flow_network.mapToIncommingHyperedgeID(he));
          const NodeID out_he = _flow_network_to_graph.get(_flow_network.mapToOutgoingHyperedgeID(he));
          if (in_he != INVALID_NODE) {
            Edge e;
            e.target_node = in_he;
            e.weight = 1.0;
            adj_list[hn_node].push_back(e);
          }
          if (out_he != INVALID_NODE) {
            Edge e;
            e.target_node = hn_node;
            e.weight = 1.0;
            adj_list[out_he].push_back(e);
          }
        }
      }
    }

    std::vector<NodeID> adj_array(1, 0);
    std::vector<Edge> edges;
    for (NodeID u = 0; u < cur_graph_node; ++u) {
      for (const Edge& e : adj_list[u]) {
        edges.push_back(e);
      }
      adj_array.push_back(adj_array[u] + adj_list[u].size());
    }

    return Graph(adj_array, edges);
  }

  void findStronglyConnectedComponents(Graph& g) {
    _visited.reset();
    _S.clear();
    NodeID dfs_num = 0;
    ClusterID cid = 0;

    for (const NodeID& u : g.nodes()) {
      if (!_visited[u]) {
        tarjanSCC(u, g, dfs_num, cid);
      }
    }
  }

  void tarjanSCC(const NodeID u, Graph& g, NodeID& dfs_num, ClusterID& cid) {
    _dfs_low.set(u, dfs_num++);
    _visited.set(u, true);
    _S.push_back(u);
    bool is_component_parent = true;

    NodeID u_low = _dfs_low.get(u);
    for (const Edge& e : g.incidentEdges(u)) {
      const NodeID v = e.target_node;
      if (!_visited[v]) {
        tarjanSCC(v, g, dfs_num, cid);
      }
      const NodeID v_low = _dfs_low.get(v);
      if (u_low > v_low) {
        _dfs_low.set(u, v_low);
        u_low = v_low;
        is_component_parent = false;
      }
    }

    if (is_component_parent) {
      while (true) {
        const NodeID v = _S.back();
        _S.pop_back();
        g.setClusterID(v, cid);
        _dfs_low.set(v, INVALID_NODE);
        if (u == v) {
          break;
        }
      }
      cid++;
    }
  }

  void topologicalSort(const Graph& g,
                       std::vector<size_t> in_degree,
                       std::vector<NodeID>& topological_order) {
    std::vector<NodeID> start_nodes;
    for (const NodeID& u : g.nodes()) {
      if (in_degree[u] == 0) {
        start_nodes.push_back(u);
      }
    }
    std::random_shuffle(start_nodes.begin(), start_nodes.end());
    for (const NodeID& u : start_nodes) {
      _Q.push(u);
    }

    size_t idx = 0;
    while (!_Q.empty()) {
      const NodeID u = _Q.front();
      _Q.pop();
      topological_order[idx++] = u;
      for (const Edge& e : g.incidentEdges(u)) {
        const NodeID v = e.target_node;
        if (u != v) {
          in_degree[v]--;
          if (in_degree[v] == 0) {
            _Q.push(v);
          }
        }
      }
    }

    ASSERT(idx == g.numNodes(), "Topological sort failed!" << V(idx) << V(g.numNodes()));
  }


  template <bool bipartition = true>
  double imbalance(const std::vector<HypernodeWeight>& part_weight) {
    if (bipartition) {
      const HypernodeWeight weight_part0 = part_weight[0];
      const HypernodeWeight weight_part1 = part_weight[1];
      const double imbalance_part0 = (weight_part0 /
                                      static_cast<double>(_context.partition.perfect_balance_part_weights[0]));
      const double imbalance_part1 = (weight_part1 /
                                      static_cast<double>(_context.partition.perfect_balance_part_weights[1]));
      return std::max(imbalance_part0, imbalance_part1) - 1.0;
    } else {
      double max_balance = (part_weight[0] /
                            static_cast<double>(_context.partition.perfect_balance_part_weights[0]));
      for (PartitionID i = 1; i < _context.partition.k; ++i) {
        const double balance_i =
          (part_weight[i] /
           static_cast<double>(_context.partition.perfect_balance_part_weights[1]));
        max_balance = std::max(max_balance, balance_i);
      }
      return max_balance - 1.0;
    }
  }


  Hypergraph& _hg;
  const Context& _context;
  Network& _flow_network;

  FastResetFlagArray<> _visited;
  FastResetArray<NodeID> _graph_to_flow_network;
  FastResetArray<NodeID> _flow_network_to_graph;
  FastResetArray<HypernodeWeight> _scc_node_weight;

  std::queue<NodeID> _Q;  // BFS queue
  std::vector<NodeID> _S;  // DFS stack
  FastResetArray<NodeID> _dfs_low;
};
}  // namespace kahypar
