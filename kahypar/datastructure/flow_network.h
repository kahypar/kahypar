/*******************************************************************************
 * This file is part of KaHyPar.

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

#include "kahypar/datastructure/fast_reset_array.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/utils/randomize.h"
#include "kahypar/utils/stats.h"

namespace kahypar {

using ds::begin;
using ds::end;
using ds::FastResetFlagArray;
using ds::FastResetArray;
using ds::SparseSet;
using NodeID = uint32_t;
using Flow = int32_t;
using Capacity = int32_t;

#define INFTY std::numeric_limits<Flow>::max() / 2
#define INVALID_NODE std::numeric_limits<NodeID>::max() / 2

namespace ds {

struct FlowEdge {
  NodeID source, target;
  Flow flow;
  Capacity capacity;
  size_t reverseEdge;

  void increaseFlow(const Flow deltaFlow) {
    ASSERT(flow + deltaFlow <= capacity, "Cannot increase flow above capacity!");
    flow += deltaFlow;
  }
};

template <class Derived = Mandatory>
class FlowNetwork {
  using AdjacentList = std::vector<std::vector<FlowEdge> >;
  using ConstIncidenceIterator = std::vector<FlowEdge>::const_iterator;
  using IncidenceIterator = std::vector<FlowEdge>::iterator;
  using NodeIterator = std::pair<const NodeID*, const NodeID*>;
  using HypernodeIterator = std::pair<const HypernodeID*, const HypernodeID*>;

 public:
  FlowNetwork(Hypergraph& hypergraph, const Context& context, const size_t size) :
    _hg(hypergraph),
    _context(context),
    _initialSize(size),
    _numNodes(0),
    _numEdges(0),
    _numHyperedges(0),
    _numUndirectedEdges(0),
    _totalWeightHyperedges(0),
    _nodes(size),
    _sources(size),
    _sinks(size),
    _hypernodes(hypergraph.initialNumNodes()),
    _removedHypernodes(hypergraph.initialNumNodes()),
    _pinsBlock0(hypergraph.initialNumEdges(), 0),
    _pinsBlock1(hypergraph.initialNumEdges(), 0),
    _curBlock0(0),
    _curBlock1(1),
    _containsGraphHyperedges(hypergraph.initialNumNodes()),
    _flowGraph(size, std::vector<FlowEdge>()),
    _visited(size),
    _heVisited(_hg.initialNumEdges()) { }

  ~FlowNetwork() = default;

  FlowNetwork(const FlowNetwork&) = delete;
  FlowNetwork& operator= (const FlowNetwork&) = delete;

  FlowNetwork(FlowNetwork &&) = delete;
  FlowNetwork& operator= (FlowNetwork &&) = delete;

  // ################### Flow Network Stat ###################

  size_t numNodes() const {
    return _numNodes;
  }

  size_t numEdges() const {
    return _numEdges;
  }

  size_t numUndirectedEdges() const {
    return _numUndirectedEdges;
  }

  HyperedgeWeight totalWeightHyperedges() {
    return _totalWeightHyperedges;
  }

  size_t initialSize() const {
    return _initialSize;
  }

  // ################### Flow Network Construction ###################

  void buildFlowGraph() {
    static_cast<Derived*>(this)->buildFlowGraphImpl();
  }

  HyperedgeWeight build(const PartitionID block_0, const PartitionID block_1) {
    buildFlowGraph();

    ASSERT([&]() {
             for (const HypernodeID& hn : hypernodes()) {
               if (containsNode(hn) && _removedHypernodes.contains(hn)) {
                 LOG << "Flow network contains HN " << hn << " but it is marked as removed!";
                 return false;
               } else if (!containsNode(hn) && !_removedHypernodes.contains(hn)) {
                 LOG << "Flow network not contains HN " << hn << " and it is not marked as removed!";
                 return false;
               }
             }
             return true;
           } (), "Hypernodes not correctly added or removed from flow network!");

    HyperedgeWeight cut = buildSourcesAndSinks(block_0, block_1);
    return cut;
  }

  void addHypernode(const HypernodeID hn) {
    ASSERT(!containsHypernode(hn), "HN " << hn << " already contained in flow problem!");
    _hypernodes.add(hn);
    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      if (_hg.edgeSize(he) == 2) {
        _containsGraphHyperedges.set(hn, true);
      }
      if (_hg.partID(hn) == _curBlock0) {
        _pinsBlock0.update(he, 1);
      } else if (_hg.partID(hn) == _curBlock1) {
        _pinsBlock1.update(he, 1);
      }
    }
  }

  void reset(const PartitionID block0 = 0, const PartitionID block1 = 1) {
    _numNodes = 0;
    _numEdges = 0;
    _numHyperedges = 0;
    _numUndirectedEdges = 0;
    _totalWeightHyperedges = 0;
    _nodes.clear();
    _sources.clear();
    _sinks.clear();
    _hypernodes.clear();
    _removedHypernodes.clear();
    _pinsBlock0.resetUsedEntries();
    _pinsBlock1.resetUsedEntries();
    _curBlock0 = block0;
    _curBlock1 = block1;
    _containsGraphHyperedges.reset();
    _visited.reset();
  }

  bool isTrivialFlow() const {
    size_t num_hyperedges_s_t = _sources.size() + _sinks.size();
    return num_hyperedges_s_t == 2 * _numHyperedges;
  }

  // ################### Flow Network <-> Hypergraph Mapping ###################

  bool isHypernode(const NodeID node) const {
    return node < _hg.initialNumNodes();
  }

  bool containsHypernode(const HypernodeID hn) {
    return _hypernodes.contains(hn);
  }

  std::pair<const HypernodeID*, const HypernodeID*> hypernodes() {
    return std::make_pair(_hypernodes.begin(), _hypernodes.end());
  }

  bool interpreteHypernode(const NodeID node) const {
    return node < _hg.initialNumNodes() && _nodes.contains(node);
  }

  NodeID mapToIncommingHyperedgeID(const HyperedgeID he) const {
    return _hg.initialNumNodes() + he;
  }

  NodeID mapToOutgoingHyperedgeID(const HyperedgeID he) const {
    return _hg.initialNumNodes() + _hg.initialNumEdges() + he;
  }

  HyperedgeID mapToHyperedgeID(const NodeID node) const {
    // ASSERT(node >= _hg.initialNumNodes(), "Node " << node << " is not a hyperedge node!");
    if (node >= _hg.initialNumNodes() + _hg.initialNumEdges()) {
      return node - _hg.initialNumNodes() - _hg.initialNumEdges();
    } else {
      return node - _hg.initialNumNodes();
    }
    return node;
  }

  bool interpreteHyperedge(const NodeID node, bool outgoing = true) const {
    return (outgoing && node >= _hg.initialNumNodes() + _hg.initialNumEdges()) ||
           (!outgoing && node >= _hg.initialNumNodes() &&
            node < _hg.initialNumNodes() + _hg.initialNumEdges());
  }

  std::pair<const HypernodeID*, const HypernodeID*> removedHypernodes() const {
    return std::make_pair(_removedHypernodes.begin(), _removedHypernodes.end());
  }

  bool isRemovedHypernode(const HypernodeID hn) const {
    return _removedHypernodes.contains(hn);
  }

  bool isHyperedgeOfSize(const HyperedgeID he, const size_t size) {
    return _pinsBlock0.get(he) + _pinsBlock1.get(he) == size;
  }

  size_t pinsNotInFlowProblem(const HyperedgeID he, const PartitionID block) {
    if (block == _curBlock0) {
      return _hg.pinCountInPart(he, _curBlock0) - _pinsBlock0.get(he);
    } else {
      return _hg.pinCountInPart(he, _curBlock1) - _pinsBlock1.get(he);
    }
  }

  // ################### Flow Network Nodes ###################

  NodeIterator nodes() {
    return std::make_pair(_nodes.begin(), _nodes.end());
  }

  void addNode(const NodeID node) {
    if (!containsNode(node)) {
      _nodes.add(node);
      _flowGraph[node].clear();
      _numNodes++;
    }
  }

  bool containsNode(const NodeID node) {
    return _nodes.contains(node);
  }

  // ################### Flow Network Edges ###################

  Flow residualCapacity(const FlowEdge& e) {
    return e.capacity - e.flow;
  }

  void increaseFlow(FlowEdge& e, const Flow deltaFlow) {
    e.increaseFlow(deltaFlow);
    FlowEdge& revEdge = reverseEdge(e);
    revEdge.increaseFlow(-deltaFlow);
  }

  FlowEdge & reverseEdge(const FlowEdge& e) {
    return _flowGraph[e.target][e.reverseEdge];
  }

  std::pair<IncidenceIterator, IncidenceIterator> incidentEdges(const NodeID u) {
    ASSERT(_nodes.contains(u), "Node " << u << " is not part of the flow graph!");
    return std::make_pair(_flowGraph[u].begin(), _flowGraph[u].end());
  }


  std::pair<ConstIncidenceIterator, ConstIncidenceIterator> incidentEdges(const NodeID u) const {
    ASSERT(_nodes.contains(u), "Node " << u << " is not part of the flow graph!");
    return std::make_pair(_flowGraph[u].cbegin(), _flowGraph[u].cend());
  }

  // ################### Source And Sink ###################

  void addSource(const NodeID u) {
    _sources.add(u);
  }

  void addSink(const NodeID u) {
    _sinks.add(u);
  }

  size_t numSources() {
    return _sources.size();
  }

  size_t numSinks() {
    return _sinks.size();
  }

  bool isSource(const NodeID node) {
    return _sources.contains(node);
  }

  bool isSink(const NodeID node) {
    return _sinks.contains(node);
  }

  NodeIterator sources() {
    return std::make_pair(_sources.begin(), _sources.end());
  }

  NodeIterator sinks() {
    return std::make_pair(_sinks.begin(), _sinks.end());
  }

 protected:
  HyperedgeWeight buildSourcesAndSinks(const PartitionID block_0, const PartitionID block_1) {
    _visited.reset();
    HyperedgeWeight cut = 0;
    for (const HypernodeID& hn : hypernodes()) {
      for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
        if (!_visited[he]) {
          size_t pinsUBlock0 = _pinsBlock0.get(he);
          size_t pinsUBlock1 = _pinsBlock1.get(he);
          size_t pinsNotUBlock0 = _hg.pinCountInPart(he, block_0) - pinsUBlock0;
          size_t pinsNotUBlock1 = _hg.pinCountInPart(he, block_1) - pinsUBlock1;
          size_t connectivity = (pinsUBlock0 + pinsNotUBlock0 > 0) + (pinsUBlock1 + pinsNotUBlock1 > 0);

          if (_context.partition.objective == Objective::cut &&
              !isRemovableFromCut(he, block_0, block_1)) {
            // Case 1: Hyperedge he cannot be removed from cut
            //         of k-way partition.
            //         E.g., if he contains a block not equal to
            //         block_0 and block_1
            //         => add incoming hyperedge node as source
            //            and outgoing hyperedge node as sink
            ASSERT(containsNode(mapToIncommingHyperedgeID(he)), "Source is not contained in flow problem!");
            ASSERT(containsNode(mapToOutgoingHyperedgeID(he)), "Sink is not contained in flow problem!");
            addSource(mapToIncommingHyperedgeID(he));
            addSink(mapToOutgoingHyperedgeID(he));
          } else {
            // Hyperedge he is a cut hyperedge of the hypergraph.
            // => if he contains pins from block_0 not contained
            //   in the flow problem, we add the incoming hyperedge
            //   node as source
            // => if he contains pins from block_1 not contained
            //   in the flow problem, we add the outgoing hyperedge
            //   node as sink
            if (pinsNotUBlock0 > 0) {
              ASSERT(containsNode(mapToIncommingHyperedgeID(he)), "Source is not contained in flow problem!");
              addSource(mapToIncommingHyperedgeID(he));
            }
            if (pinsNotUBlock1 > 0) {
              ASSERT(containsNode(mapToOutgoingHyperedgeID(he)), "Sink is not contained in flow problem!");
              addSink(mapToOutgoingHyperedgeID(he));
            }
          }

          // Sum up the weight of all cut edges of the bipartition (block0,block1)
          if ((_context.partition.objective == Objective::km1 && connectivity > 1) ||
              (_context.partition.objective == Objective::cut && _hg.connectivity(he) > 1)) {
            cut += _hg.edgeWeight(he);
          }

          _visited.set(he, true);
        }
      }
    }

    return cut;
  }

  bool isRemovableFromCut(const HyperedgeID he, const PartitionID block_0, const PartitionID block_1) {
    if (_hg.connectivity(he) > 2) return false;
    for (const PartitionID& part : _hg.connectivitySet(he)) {
      if (part != block_0 && part != block_1) return false;
    }
    return true;
  }

  void addEdge(const NodeID u, const NodeID v, const Capacity capacity, bool undirected = false) {
    addNode(u);
    addNode(v);
    FlowEdge e1;
    e1.source = u;
    e1.target = v;
    e1.flow = 0;
    e1.capacity = capacity;
    FlowEdge e2;
    e2.source = v;
    e2.target = u;
    e2.flow = 0;
    e2.capacity = (undirected ? capacity : 0);
    _flowGraph[u].push_back(e1);
    _flowGraph[v].push_back(e2);
    size_t e1_idx = _flowGraph[u].size() - 1;
    size_t e2_idx = _flowGraph[v].size() - 1;
    _flowGraph[u][e1_idx].reverseEdge = e2_idx;
    _flowGraph[v][e2_idx].reverseEdge = e1_idx;
    _numEdges += (undirected ? 2 : 1);
    _numUndirectedEdges += (undirected ? 1 : 0);
    _totalWeightHyperedges += (capacity < INFTY ? capacity : 0);
  }

  /*
   * In following with denote with:
   *   - v -> a hypernode node
   *   - e' -> a incomming hyperedge node
   *   - e'' -> a outgoing hyperedge node
   */

  /*
   * Add a hyperedge to flow problem
   * Edge e' -> e''
   */
  void addHyperedge(const HyperedgeID he, bool ignoreHESizeOne = false) {
    NodeID u = mapToIncommingHyperedgeID(he);
    NodeID v = mapToOutgoingHyperedgeID(he);
    if (!ignoreHESizeOne && isHyperedgeOfSize(he, 1)) {
      // Check if he is really a hyperedge of size 1 in flow problem
      ASSERT([&]() {
               size_t numFlowHN = 0;
               for (const HypernodeID& pin : _hg.pins(he)) {
                 if (containsHypernode(pin)) {
                   numFlowHN++;
                 }
               }
               return (numFlowHN == 1 ? true : false);
             } (), "Hyperedge " << he << " is not a size 1 hyperedge in flow problem!");

      if ((pinsNotInFlowProblem(he, _curBlock0) > 0 &&
           pinsNotInFlowProblem(he, _curBlock1) > 0) ||
          (_context.partition.objective == Objective::cut &&
           !isRemovableFromCut(he, _curBlock0, _curBlock1))) {
        addEdge(u, v, _hg.edgeWeight(he));
      } else {
        if (pinsNotInFlowProblem(he, _curBlock0) > 0) {
          addNode(u);
        } else if (pinsNotInFlowProblem(he, _curBlock1) > 0) {
          addNode(v);
        }
      }
    } else {
      addEdge(u, v, _hg.edgeWeight(he));
    }

    if (containsNode(u) || containsNode(v)) {
      _numHyperedges++;
    }
  }

  void addGraphEdge(const HyperedgeID he) {
    ASSERT(_hg.edgeSize(he) == 2, "Hyperedge " << he << " is not a graph edge!");

    NodeID u = INVALID_NODE;
    NodeID v = INVALID_NODE;
    for (const HypernodeID& pin : _hg.pins(he)) {
      if (u == INVALID_NODE) {
        u = pin;
      } else {
        v = pin;
      }
    }

    ASSERT(containsHypernode(u) || containsHypernode(v),
           "Hyperedge " << he << " is not in contained in the flow problem!");
    if (containsHypernode(v)) std::swap(u, v);
    addNode(u);

    if (containsHypernode(v)) {
      addEdge(u, v, _hg.edgeWeight(he), true);
    }
  }

  /*
   * Connecting a hyperedge with its pin
   * Edges e'' -> v and v -> e'
   */
  void addPin(const HyperedgeID he, const HypernodeID pin) {
    NodeID u = mapToIncommingHyperedgeID(he);
    NodeID v = mapToOutgoingHyperedgeID(he);
    if (containsNode(u) && containsNode(v)) {
      addEdge(pin, u, INFTY);
      addEdge(v, pin, INFTY);
    } else {
      if (containsNode(u)) {
        ASSERT(_flowGraph[u].size() == 0, "Pin of size 1 hyperedge already added in flow graph!");
        addEdge(u, pin, _hg.edgeWeight(he));
      } else if (containsNode(v)) {
        ASSERT(_flowGraph[v].size() == 0, "Pin of size 1 hyperedge already added in flow graph!");
        addEdge(pin, v, _hg.edgeWeight(he));
      }
    }
    ASSERT(!(containsNode(u) || containsNode(v)) || containsNode(pin),
           "Pin " << pin << " should be added to flow graph!");
  }

  /*
   * Connecting the outgoing hyperedge node of he to all incomming
   * hyperedge nodes of incident hyperedges of hn.
   */
  void addClique(const HyperedgeID he, const HypernodeID hn) {
    ASSERT(_hg.nodeDegree(hn) <= 3,
           "Hypernode " << hn << " should not be removed from flow problem!");
    _removedHypernodes.add(hn);
    _heVisited.set(he, true);
    for (const HyperedgeID& target : _hg.incidentEdges(hn)) {
      if (!_heVisited[target]) {
        addEdge(mapToOutgoingHyperedgeID(he), mapToIncommingHyperedgeID(target), INFTY);
        _heVisited.set(target, true);
      }
    }
  }

  Hypergraph& _hg;
  const Context& _context;

  size_t _initialSize;
  size_t _numNodes;
  size_t _numEdges;
  size_t _numHyperedges;
  size_t _numUndirectedEdges;
  HyperedgeWeight _totalWeightHyperedges;

  SparseSet<NodeID> _nodes;
  SparseSet<NodeID> _sources;
  SparseSet<NodeID> _sinks;
  SparseSet<NodeID> _hypernodes;
  SparseSet<HypernodeID> _removedHypernodes;
  FastResetArray<size_t> _pinsBlock0;
  FastResetArray<size_t> _pinsBlock1;
  PartitionID _curBlock0;
  PartitionID _curBlock1;
  FastResetFlagArray<> _containsGraphHyperedges;
  AdjacentList _flowGraph;

  FastResetFlagArray<> _visited;
  FastResetFlagArray<> _heVisited;
};

class LawlerNetwork final : public FlowNetwork<LawlerNetwork>{
  using Base = FlowNetwork<LawlerNetwork>;
  friend Base;

 public:
  LawlerNetwork(Hypergraph & hypergraph, const Context &context) :
    Base(hypergraph, context,
         hypergraph.initialNumNodes() + 2 * hypergraph.initialNumEdges()) { }

  ~LawlerNetwork() = default;

  LawlerNetwork(const LawlerNetwork &) = delete;
  LawlerNetwork& operator= (const LawlerNetwork&) = delete;

  LawlerNetwork(LawlerNetwork &&) = delete;
  LawlerNetwork& operator= (LawlerNetwork &&) = delete;

 private:
  void buildFlowGraphImpl() {
    _visited.reset();
    for (const HypernodeID& hn : hypernodes()) {
      for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
        if (!_visited[he]) {
          addHyperedge(he, true);
          _visited.set(he, true);
        }
        addPin(he, hn);
      }
    }
  }


  using Base::_hg;
  using Base::_context;
  using Base::_visited;
};

class HeuerNetwork final : public FlowNetwork<HeuerNetwork>{
  using Base = FlowNetwork<HeuerNetwork>;
  friend Base;

 public:
  HeuerNetwork(Hypergraph & hypergraph, const Context &context) :
    Base(hypergraph, context,
         hypergraph.initialNumNodes() + 2 * hypergraph.initialNumEdges()) { }

  ~HeuerNetwork() = default;

  HeuerNetwork(const HeuerNetwork &) = delete;
  HeuerNetwork& operator= (const HeuerNetwork&) = delete;

  HeuerNetwork(HeuerNetwork &&) = delete;
  HeuerNetwork& operator= (HeuerNetwork &&) = delete;

 private:
  void buildFlowGraphImpl() {
    _visited.reset();

    for (const HypernodeID& hn : hypernodes()) {
      for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
        if (!_visited[he]) {
          // Add a directed flow edge between the incomming
          // and outgoing hyperedge node of he with
          // capacity w(he)
          addHyperedge(he, true);

          // If degree of hypernode 'pin' is smaller than 3
          // add clique between all incident hyperedges of pin.
          // Otherwise connect pin with hyperedge in flow network.
          _heVisited.reset();
          for (const HypernodeID& pin : _hg.pins(he)) {
            if (_hypernodes.contains(pin)) {
              if (_hg.nodeDegree(pin) <= 3) {
                ASSERT(!containsNode(pin),
                       "Pin " << pin << " of HE " << he << " is already contained in flow problem!");
                addClique(he, pin);
              } else {
                addPin(he, pin);
              }
            }
          }
          // Invariant: All outgoing edges of hyperedge 'he'
          //            are contained in the flow problem.

          _visited.set(he, true);
        }
      }
    }
  }


  using Base::_hg;
  using Base::_context;
  using Base::_visited;
  using Base::_heVisited;
};

class WongNetwork final : public FlowNetwork<WongNetwork>{
  using Base = FlowNetwork<WongNetwork>;
  friend Base;

 public:
  WongNetwork(Hypergraph & hypergraph, const Context &context) :
    Base(hypergraph, context,
         hypergraph.initialNumNodes() + 2 * hypergraph.initialNumEdges()) { }

  ~WongNetwork() = default;

  WongNetwork(const WongNetwork &) = delete;
  WongNetwork& operator= (const WongNetwork&) = delete;

  WongNetwork(WongNetwork &&) = delete;
  WongNetwork& operator= (WongNetwork &&) = delete;

 private:
  void buildFlowGraphImpl() {
    _visited.reset();

    for (const HypernodeID& hn : hypernodes()) {
      for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
        if (!_visited[he]) {
          if (isHyperedgeOfSize(he, 1) || _hg.edgeSize(he) != 2) {
            // Add a directed flow edge between the incomming
            // and outgoing hyperedge node of he with
            // capacity w(he)
            addHyperedge(he);
          } else {
          // Treat hyperedges of size 2 as graph edges
          // => remove hyperedge nodes in flow network
            addGraphEdge(he);
          }

          _visited.set(he, true);
        }

        if (isHyperedgeOfSize(he, 1) || _hg.edgeSize(he) != 2) {
          addPin(he, hn);
        }
      }
    }
  }

  using Base::_hg;
  using Base::_visited;
};


class HybridNetwork final : public FlowNetwork<HybridNetwork>{
  using Base = FlowNetwork<HybridNetwork>;
  friend Base;

 public:
  HybridNetwork(Hypergraph & hypergraph, const Context &context) :
    Base(hypergraph, context,
         hypergraph.initialNumNodes() + 2 * hypergraph.initialNumEdges()) { }

  ~HybridNetwork() = default;

  HybridNetwork(const HybridNetwork &) = delete;
  HybridNetwork& operator= (const HybridNetwork&) = delete;

  HybridNetwork(HybridNetwork &&) = delete;
  HybridNetwork& operator= (HybridNetwork &&) = delete;

 protected:
  void buildFlowGraphImpl() {
    _visited.reset();
    for (const HypernodeID& hn : hypernodes()) {
      for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
        if (!_visited[he]) {
          if (isHyperedgeOfSize(he, 1)) {
            addHyperedge(he);
            addPin(he, hn);
            _containsGraphHyperedges.set(hn, true);
          } else if (_hg.edgeSize(he) == 2) {
            // Treat hyperedges of size 2 as graph edges
            // => remove hyperedge nodes in flow network
            addGraphEdge(he);
          }  else {
            // Add a directed flow edge between the incomming
            // and outgoing hyperedge node of he with
            // capacity w(he)
            addHyperedge(he);
          }

          _visited.set(he, true);
        }
      }
    }

    for (const NodeID& node : nodes()) {
      if (interpreteHyperedge(node, false)) {
        HyperedgeID he = mapToHyperedgeID(node);
        if (_hg.edgeSize(he) != 2 && !isHyperedgeOfSize(he, 1)) {
          // If degree of hypernode 'pin' is smaller than 3
          // add clique between all incident hyperedges of pin.
          // Otherwise connect pin with hyperedge in flow network.
          _heVisited.reset();
          for (const HypernodeID& pin : _hg.pins(he)) {
            if (_hypernodes.contains(pin)) {
              if (!_containsGraphHyperedges[pin] &&
                  _hg.nodeDegree(pin) <= 3) {
                ASSERT(!containsNode(pin), "Pin " << pin << " of HE " << he
                                                  << " is already contained in flow problem!");
                addClique(he, pin);
              } else {
                addPin(he, pin);
                ASSERT(containsNode(pin), "Pin is not contained in flow problem!");
              }
            }
          }
          // Invariant: All outgoing edges of hyperedge 'he'
          //            are contained in the flow problem.
        }
      }
    }
  }


  using Base::_hg;
  using Base::_context;
  using Base::_visited;
  using Base::_heVisited;
};
}  // namespace ds
}  // namespace kahypar
