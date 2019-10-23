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
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#include "external_maximum_flow/bk/graph.h"
#include "external_maximum_flow/ibfs/ibfs.h"
#include "kahypar/datastructure/fast_reset_array.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/flow_network.h"
#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/meta/typelist.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/flow/most_balanced_minimum_cut.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
template <class Network = Mandatory>
class MaximumFlow {
 public:
  MaximumFlow(Hypergraph& hypergraph, const Context& context, Network& flow_network) :
    _hg(hypergraph),
    _context(context),
    _flow_network(flow_network),
    _parent(flow_network.initialSize(), nullptr),
    _visited(flow_network.initialSize()),
    _Q(),
    _mbmc(hypergraph, _context, flow_network),
    _original_part_id(_hg.initialNumNodes(), 0) { }

  virtual ~MaximumFlow() { }

  MaximumFlow(const MaximumFlow&) = delete;
  MaximumFlow& operator= (const MaximumFlow&) = delete;

  MaximumFlow(MaximumFlow&&) = delete;
  MaximumFlow& operator= (MaximumFlow&&) = delete;


  virtual Flow maximumFlow() = 0;

  HyperedgeWeight minimumSTCut(const PartitionID block_0, const PartitionID block_1) {
    if (_flow_network.isTrivialFlow()) {
      return Network::kInfty;
    }

    const PartitionID default_part =
      _context.local_search.flow.use_most_balanced_minimum_cut ? block_0 : block_1;
    for (const HypernodeID& hn : _flow_network.hypernodes()) {
      _original_part_id[hn] = _hg.partID(hn);
      moveHypernode(hn, default_part);
    }

    const HyperedgeWeight cut = maximumFlow();

    if (_context.local_search.flow.use_most_balanced_minimum_cut) {
      _mbmc.mostBalancedMinimumCut(block_0, block_1);
    } else {
      bfs<true>(block_0);
    }

    return cut;
  }

  void rollback(const bool store_part_id = false) {
    for (const HypernodeID& hn : _flow_network.hypernodes()) {
      const PartitionID from = _hg.partID(hn);
      moveHypernode(hn, _original_part_id[hn]);
      if (store_part_id) {
        _original_part_id[hn] = from;
      }
    }
  }

  PartitionID getOriginalPartition(const HypernodeID hn) const {
    return _original_part_id[hn];
  }

  template <bool assign_hypernodes = false>
  bool bfs(const PartitionID block = 0) {
    bool augmenting_path_exists = false;
    _parent.resetUsedEntries();
    _visited.reset();
    while (!_Q.empty()) {
      _Q.pop();
    }

    // Initialize queue with all source nodes
    for (const NodeID& s : _flow_network.sources()) {
      _visited.set(s, true);
      _parent.set(s, nullptr);
      _Q.push(s);
    }

    while (!_Q.empty()) {
      NodeID u = _Q.front();
      _Q.pop();

      if (assign_hypernodes) {
        if (_flow_network.interpreteHypernode(u)) {
          moveHypernode(u, block);
        } else if (_flow_network.interpreteHyperedge(u)) {
          const HyperedgeID he = _flow_network.mapToHyperedgeID(u);
          for (const HypernodeID& pin : _hg.pins(he)) {
            if (_flow_network.containsHypernode(pin)) {
              moveHypernode(pin, block);
            }
          }
        }
      }

      if (_flow_network.isSink(u)) {
        augmenting_path_exists = true;
        continue;
      }

      for (FlowEdge& e : _flow_network.incidentEdges(u)) {
        const NodeID v = e.target;
        if (!_visited[v] && _flow_network.residualCapacity(e)) {
          _parent.set(v, &e);
          _visited.set(v, true);
          _Q.push(v);
        }
      }
    }
    return augmenting_path_exists;
  }

 protected:
  template <typename T>
  FRIEND_TEST(AMaximumFlow, ChecksIfAugmentingPathExist);
  template <typename T>
  FRIEND_TEST(AMaximumFlow, AugmentAlongPath);

  Flow augment(const NodeID cur, const Flow min_flow = Network::kInfty) {
    if (_flow_network.isSource(cur) || min_flow == 0) {
      return min_flow;
    } else {
      FlowEdge* e = _parent.get(cur);
      const Flow f = augment(e->source, std::min(min_flow, _flow_network.residualCapacity(*e)));

      ASSERT([&]() {
          const Flow residual_forward_before = _flow_network.residualCapacity(*e);
          const Flow residual_backward_before = _flow_network.residualCapacity(_flow_network.reverseEdge(*e));
          _flow_network.increaseFlow(*e, f);
          Flow residual_forward_after = _flow_network.residualCapacity(*e);
          Flow residual_backward_after = _flow_network.residualCapacity(_flow_network.reverseEdge(*e));
          if (residual_forward_before != Network::kInfty && residual_forward_before != residual_forward_after + f) {
            LOG << "Residual capacity should be " << (residual_forward_before - f) << "!";
            return false;
          }
          if (residual_backward_before != Network::kInfty && residual_backward_before != residual_backward_after - f) {
            LOG << "Residual capacity should be " << (residual_backward_before + f) << "!";
            return false;
          }
          _flow_network.increaseFlow(_flow_network.reverseEdge(*e), f);
          residual_forward_after = _flow_network.residualCapacity(*e);
          residual_backward_after = _flow_network.residualCapacity(_flow_network.reverseEdge(*e));
          if (residual_forward_before != residual_forward_after ||
              residual_backward_before != residual_backward_after) {
            LOG << "Restoring original capacities failed!";
            return false;
          }
          return true;
        } (), "Flow is not increased correctly!");

      _flow_network.increaseFlow(*e, f);
      return f;
    }
  }

  void moveHypernode(const HypernodeID hn, const PartitionID to) {
    ASSERT(_hg.partID(hn) != -1, "Hypernode " << hn << " should be assigned to a part");
    const PartitionID from = _hg.partID(hn);
    if (from != to && !_hg.isFixedVertex(hn)) {
      _hg.changeNodePart(hn, from, to);
    }
  }

  Hypergraph& _hg;
  const Context& _context;
  Network& _flow_network;

  // Datastructure for BFS
  FastResetArray<FlowEdge*> _parent;
  FastResetFlagArray<> _visited;
  std::queue<NodeID> _Q;

  MostBalancedMinimumCut<Network> _mbmc;

  std::vector<PartitionID> _original_part_id;
};

template <class Network = Mandatory>
class BoykovKolmogorov : public MaximumFlow<Network>{
  using Base = MaximumFlow<Network>;
  using FlowGraph = maxflow::Graph<int, int, int>;

 public:
  BoykovKolmogorov(Hypergraph& hypergraph, const Context& context, Network& flow_network) :
    Base(hypergraph, context, flow_network),
    _flow_graph(static_cast<size_t>(hypergraph.initialNumNodes()) + 2 * hypergraph.initialNumEdges(),
                static_cast<size_t>(hypergraph.initialNumNodes()) + 2 * hypergraph.initialNumEdges()),
    _flow_network_mapping(static_cast<size_t>(hypergraph.initialNumNodes()) + 2 * hypergraph.initialNumEdges(), 0) { }

  ~BoykovKolmogorov() = default;

  BoykovKolmogorov(const BoykovKolmogorov&) = delete;
  BoykovKolmogorov& operator= (const BoykovKolmogorov&) = delete;

  BoykovKolmogorov(BoykovKolmogorov&&) = delete;
  BoykovKolmogorov& operator= (BoykovKolmogorov&&) = delete;

  Flow maximumFlow() {
    mapToExternalFlowNetwork();

    const Flow max_flow = _flow_graph.maxflow();

    FlowGraph::arc* a = _flow_graph.get_first_arc();
    while (a != _flow_graph.arc_last) {
      const Flow flow = a->flowEdge->capacity - _flow_graph.get_rcap(a);
      if (flow != 0) {
        a->flowEdge->increaseFlow(flow);
      }
      a = _flow_graph.get_next_arc(a);
    }

    ASSERT(!Base::bfs(), "Found augmenting path after flow computation finished!");
    return max_flow;
  }

 private:
  template <typename T>
  FRIEND_TEST(AMaximumFlow, ChecksIfAugmentingPathExist);
  template <typename T>
  FRIEND_TEST(AMaximumFlow, AugmentAlongPath);

  void mapToExternalFlowNetwork() {
    _flow_graph.reset();
    _visited.reset();
    const Flow infty = _flow_network.totalWeightHyperedges();

    for (const NodeID& node : _flow_network.nodes()) {
      const NodeID id = _flow_graph.add_node();
      _flow_network_mapping[node] = id;
      if (_flow_network.isSource(node)) {
        _flow_graph.add_tweights(id, infty, 0);
      }
      if (_flow_network.isSink(node)) {
        _flow_graph.add_tweights(id, 0, infty);
      }
    }

    for (const NodeID node : _flow_network.nodes()) {
      const NodeID u = _flow_network_mapping[node];
      for (FlowEdge& edge : _flow_network.incidentEdges(node)) {
        const NodeID v = _flow_network_mapping[edge.target];
        const Capacity c = edge.capacity;
        FlowEdge& rev_edge = _flow_network.reverseEdge(edge);
        const Capacity rev_c = rev_edge.capacity;
        if (!_visited[edge.target]) {
          FlowGraph::arc* a = _flow_graph.add_edge(u, v, c, rev_c);
          a->flowEdge = &edge;
          a->sister->flowEdge = &rev_edge;
        }
      }
      _visited.set(node, true);
    }
  }

  using Base::_hg;
  using Base::_context;
  using Base::_flow_network;
  using Base::_parent;
  using Base::_visited;

  FlowGraph _flow_graph;
  std::vector<NodeID> _flow_network_mapping;
};


template <class Network = Mandatory>
class IBFS : public MaximumFlow<Network>{
  using Base = MaximumFlow<Network>;
  using FlowGraph = maxflow::IBFSGraph;

 public:
  IBFS(Hypergraph& hypergraph, const Context& context, Network& flow_network) :
    Base(hypergraph, context, flow_network),
    _flow_graph(FlowGraph::IB_INIT_COMPACT),
    _flow_network_mapping(static_cast<size_t>(hypergraph.initialNumNodes()) +
                          2 * hypergraph.initialNumEdges(), 0) { }

  ~IBFS() = default;

  IBFS(const IBFS&) = delete;
  IBFS& operator= (const IBFS&) = delete;

  IBFS(IBFS&&) = delete;
  IBFS& operator= (IBFS&&) = delete;

  Flow maximumFlow() {
    mapToExternalFlowNetwork();

    _flow_graph.computeMaxFlow();
    const Flow max_flow = _flow_graph.getFlow();

    FlowGraph::Arc* a = _flow_graph.arcs;
    while (a != _flow_graph.arcEnd) {
      const Flow flow = a->flowEdge->capacity - a->rCap;
      if (flow != 0) {
        a->flowEdge->increaseFlow(flow);
      }
      a++;
    }

    ASSERT(!Base::bfs(), "Found augmenting path after flow computation finished!");
    return max_flow;
  }

 private:
  template <typename T>
  FRIEND_TEST(AMaximumFlow, ChecksIfAugmentingPathExist);
  template <typename T>
  FRIEND_TEST(AMaximumFlow, AugmentAlongPath);

  void mapToExternalFlowNetwork() {
    _flow_graph.initSize(_flow_network.numNodes(), _flow_network.numEdges() - _flow_network.numUndirectedEdges());
    _visited.reset();
    const Flow infty = _flow_network.totalWeightHyperedges();
    NodeID cur_id = 0;

    for (const NodeID& node : _flow_network.nodes()) {
      _flow_graph.addNode(cur_id,
                          _flow_network.isSource(node) ? infty : 0,
                          _flow_network.isSink(node) ? infty : 0);
      _flow_network_mapping[node] = cur_id;
      cur_id++;
    }

    for (const NodeID node : _flow_network.nodes()) {
      const NodeID u = _flow_network_mapping[node];
      for (FlowEdge& edge : _flow_network.incidentEdges(node)) {
        const NodeID v = _flow_network_mapping[edge.target];
        const Capacity c = edge.capacity;
        FlowEdge& rev_edge = _flow_network.reverseEdge(edge);
        const Capacity rev_c = rev_edge.capacity;
        if (!_visited[edge.target]) {
          _flow_graph.addEdge(u, v, c, rev_c, &edge, &rev_edge);
        }
      }
      _visited.set(node, true);
    }

    _flow_graph.initGraph();
  }

  using Base::_hg;
  using Base::_context;
  using Base::_flow_network;
  using Base::_parent;
  using Base::_visited;

  FlowGraph _flow_graph;
  std::vector<NodeID> _flow_network_mapping;
};
}  // namespace kahypar
