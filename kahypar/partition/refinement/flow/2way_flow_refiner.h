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

#include <array>
#include <string>
#include <utility>
#include <queue>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/meta/abstract_factory.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/datastructure/fast_reset_array.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/partition/refinement/flow/maximum_flow.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"
#include "kahypar/partition/refinement/flow/policies/flow_region_build_policy.h"

namespace kahypar {

template<class Network = Mandatory>
using FlowAlgorithmFactory = meta::Factory<FlowAlgorithm,
                                           MaximumFlow<Network>* (*)(Hypergraph&, const Context&, Network&)>;

using ds::SparseSet;
using ds::FastResetArray;

#define invalidHE std::numeric_limits<HyperedgeID>::max()
#define invalidHN std::numeric_limits<HypernodeID>::max()

template<class FlowNetworkPolicy = Mandatory,
         class FlowExecutionPolicy = Mandatory>
class TwoWayFlowRefiner final : public IRefiner {
using Network = typename FlowNetworkPolicy::Network;

 public:
  TwoWayFlowRefiner(Hypergraph& hypergraph, const Context& context) :
    _hg(hypergraph),
    _context(context),
    _flowNetwork(_hg, _context),
    _maximumFlow(FlowAlgorithmFactory<Network>::getInstance().createObject(
                 _context.local_search.flow.algorithm, hypergraph, _context, _flowNetwork)),
    _flowExecutionPolicy(),
    _quotientGraph(nullptr),
    _visited(_hg.initialNumNodes() + _hg.initialNumEdges()),
    _originalPartId(_hg.initialNumNodes(), -1),
    _movedHNs(_hg.initialNumNodes()),
    _block0(0),
    _block1(1),
    _rollback(false),
    _ignoreFlowExecutionPolicy(false) {}

  TwoWayFlowRefiner(const TwoWayFlowRefiner&) = delete;
  TwoWayFlowRefiner(TwoWayFlowRefiner&&) = delete;
  TwoWayFlowRefiner& operator= (const TwoWayFlowRefiner&) = delete;
  TwoWayFlowRefiner& operator= (TwoWayFlowRefiner&&) = delete;

  std::pair<const NodeID *, const NodeID *> movedHypernodes() {
      return std::make_pair(_movedHNs.begin(), _movedHNs.end());
  }

  /*
   * The 2way flow refiner can be used in combination with other
   * refiners. 
   *
   * k-Way Flow Refiner:
   * Performs active block scheduling on the quotient graph. Therefore
   * the quotient graph is already initialized and we have to pass
   * the two blocks which are selected for a pairwise refinement.
   *
   * FM Refiner:
   * FM refiners performs delta gain updates. Moving nodes inside the
   * flow refiner without notifying the FM refiner invalidate the gain
   * cache. Therefore we need to rollback all changes after flow
   * refinement and let the FM refiner perform the moves with
   * the correct update of the gain cache.
   */
  void updateConfiguration(const PartitionID block0,
                           const PartitionID block1,
                           QuotientGraphBlockScheduler* quotientGraph,
                           bool rollback,
                           bool ignoreFlowExecutionPolicy) {
    _block0 = block0;
    _block1 = block1;
    _quotientGraph = quotientGraph;
    _rollback = rollback;
    _ignoreFlowExecutionPolicy = ignoreFlowExecutionPolicy;
  }

 private:
  friend class TwoWayFlowRefinerTest;

  static constexpr bool debug = false;

  bool refineImpl(std::vector<HypernodeID>&,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics& best_metrics) override final {
    if (!_flowExecutionPolicy.executeFlow(_hg) && !_ignoreFlowExecutionPolicy) {
        return false;
    }

    // Construct quotient graph, if it is not set before
    bool deleteQuotientGraphAfterFlow = false;
    if (!_quotientGraph) {
        deleteQuotientGraphAfterFlow = true;
        _quotientGraph = new QuotientGraphBlockScheduler(_hg, _context);
        _quotientGraph->buildQuotientGraph();
    }

    // Stores original partition, if a rollback after
    // flow execution is required
    HyperedgeWeight old_km1 = best_metrics.km1;
    HyperedgeWeight old_cut = best_metrics.cut;
    double old_imbalance = best_metrics.imbalance;
    if (_rollback) {
        storeOriginalPartitionIDs();
    }

    DBG << V(metrics::imbalance(_hg, _context))
        << V(_context.partition.objective)
        << V(metrics::objective(_hg, _context.partition.objective));
    DBG << "Refine " << V(_block0) << "and" << V(_block1);

    bool improvement = false;
    double alpha = _context.local_search.flow.alpha * 2.0;

    // Adaptive Flow Iterations
    do {
        alpha /= 2.0;
        _flowNetwork.reset(_block0, _block1);

        DBG << "";
        DBG << V(alpha);

        // Initialize set of cut hyperedges for blocks '_block0' and '_block1'
        std::vector<HyperedgeID> cut_hes;
        HyperedgeWeight cut_weight = 0;
        for (const HyperedgeID& he : _quotientGraph->blockPairCutHyperedges(_block0, _block1)) {
            cut_weight += _hg.edgeWeight(he);
            cut_hes.push_back(he);
        }

        // Heurist 1: Don't execute 2way flow refinement for adjacent blocks
        //            in the quotient graph with a small cut
        if (_context.local_search.flow.ignore_small_hyperedge_cut &&
            cut_weight <= 10 && !isRefinementOnLastLevel()) {
            return false;
        }

        // If cut is 0 no improvement is possible
        if (cut_hes.size() == 0) {
            DBG << "Cut is zero";
            break;
        }

        std::random_shuffle(cut_hes.begin(), cut_hes.end());

        // Build Flow Problem
        CutBuildPolicy::buildFlowNetwork(_hg, _context, _flowNetwork,
                                         cut_hes, alpha, _block0, _block1,
                                         _visited);
        HyperedgeWeight cut_flow_network_before = _flowNetwork.build(_block0, _block1);
        DBG << V(_flowNetwork.numNodes()) << V(_flowNetwork.numEdges());

        printMetric();

        // Find minimum (S,T)-bipartition
        HyperedgeWeight cut_flow_network_after = _maximumFlow->minimumSTCut(_block0, _block1);

        // Maximum Flow algorithm returns infinity, if all
        // hypernodes contained in the flow problem are either
        // sources or sinks
        if (cut_flow_network_after == INFTY) {
            DBG << "Trivial Cut";
            break;
        }

        HyperedgeWeight delta = cut_flow_network_before - cut_flow_network_after;
        ASSERT(cut_flow_network_before >= cut_flow_network_after,
                "Flow calculation should not increase cut!"
                << V(cut_flow_network_before) << V(cut_flow_network_after));
        ASSERT(best_metrics.getMetric(_context.partition.objective) - delta
               == metrics::objective(_hg, _context.partition.objective),
               "Maximum Flow is not the minimum cut!"
               << V(_context.partition.objective)
               << V(best_metrics.getMetric(_context.partition.objective))
               << V(delta)
               << V(metrics::objective(_hg, _context.partition.objective)));

        double current_imbalance = metrics::imbalance(_hg, _context);
        HyperedgeWeight old_metric = best_metrics.getMetric(_context.partition.objective);
        HyperedgeWeight current_metric = old_metric - delta;

        DBG << V(cut_flow_network_before)
            << V(cut_flow_network_after)
            << V(delta)
            << V(old_metric)
            << V(current_metric);

        printMetric();

        bool equal_metric = current_metric == best_metrics.getMetric(_context.partition.objective);
        bool improved_metric = current_metric < best_metrics.getMetric(_context.partition.objective);
        bool improved_imbalance = current_imbalance < best_metrics.imbalance;
        bool is_feasible_partition = current_imbalance <= _context.partition.epsilon;

        bool current_improvement = false;
        if ( (improved_metric && (is_feasible_partition || improved_imbalance)) ||
            (equal_metric && improved_imbalance) ) {
            best_metrics.updateMetric(current_metric, _context.partition.objective);
            best_metrics.imbalance = current_imbalance;
            improvement = true;
            current_improvement = true;

            alpha *= (alpha == _context.local_search.flow.alpha ? 2.0 : 4.0);
        }

        _maximumFlow->rollback(current_improvement);

        // Perform moves in quotient graph in order to update
        // cut hyperedges between adjacent blocks.
        if (current_improvement) {
            for (const HypernodeID& hn : _flowNetwork.hypernodes()) {
                PartitionID from = _hg.partID(hn);
                PartitionID to = _maximumFlow->getOriginalPartition(hn);
                if ( from != to ) {
                    _quotientGraph->changeNodePart(hn, from, to);
                }
            }
        }

        // Heuristic 2: If no improvement was found, but the cut before and
        //              after is equal, we assume that the partition is close
        //              to the optimum and break the adaptive flow iterations.
        if (_context.local_search.flow.use_adaptive_alpha_stopping_rule &&
            !improvement && cut_flow_network_before == cut_flow_network_after) {
            break;
        }
    } while (alpha > 1.0);

    // Restore original partition, if a rollback after
    // flow execution is required
    if (_rollback) {
        restoreOriginalPartitionIDs();
        best_metrics.km1 = old_km1;
        best_metrics.cut = old_cut;
        best_metrics.imbalance = old_imbalance;
    }

    printMetric(true, true);

    // Delete quotient graph
    if (deleteQuotientGraphAfterFlow) {
        delete _quotientGraph;
        _quotientGraph = nullptr;
    }

    return improvement;
  }

  void storeOriginalPartitionIDs() {
    _movedHNs.clear();
    _originalPartId.resetUsedEntries();
    for (const HypernodeID& hn : _hg.nodes()) {
        _originalPartId.set(hn, _hg.partID(hn));
    }
  }

  void restoreOriginalPartitionIDs() {
    for (const HypernodeID& hn : _hg.nodes()) {
        PartitionID from = _hg.partID(hn);
        PartitionID to = _originalPartId.get(hn);
        if (from != to) {
            _movedHNs.add(hn);
            _hg.changeNodePart(hn, from, to);
        }
    }
  }

  bool isRefinementOnLastLevel() {
      return _hg.currentNumNodes() == _hg.initialNumNodes();
  }

  void printMetric(bool newline = false, bool endline = false) {
    if (newline) {
        DBG << "";
    }
    DBG << V(metrics::imbalance(_hg, _context))
        << V(_context.partition.objective)
        << V(metrics::objective(_hg, _context.partition.objective));
    if (endline) {
        DBG << "-------------------------------------------------------------";
    }
  }

  void initializeImpl(const HyperedgeWeight) override final {
    _is_initialized = true;
    _flowExecutionPolicy.initialize(_hg, _context);
  }

  using IRefiner::_is_initialized;

  Hypergraph& _hg;
  const Context& _context;
  Network _flowNetwork;
  std::unique_ptr<MaximumFlow<Network>> _maximumFlow;
  FlowExecutionPolicy _flowExecutionPolicy;
  QuotientGraphBlockScheduler* _quotientGraph;
  FastResetFlagArray<> _visited;
  FastResetArray<PartitionID> _originalPartId;
  SparseSet<HypernodeID> _movedHNs;
  PartitionID _block0;
  PartitionID _block1;
  bool _rollback;
  bool _ignoreFlowExecutionPolicy;
};
}  // namespace kahypar
