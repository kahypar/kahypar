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
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_array.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/abstract_factory.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/flow/flow_refiner_base.h"
#include "kahypar/partition/refinement/flow/maximum_flow.h"
#include "kahypar/partition/refinement/flow/policies/flow_region_build_policy.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
template <class Network = Mandatory>
using FlowAlgorithmFactory = meta::Factory<FlowAlgorithm,
                                           MaximumFlow<Network>* (*)(Hypergraph&, const Context&, Network&)>;

using ds::SparseSet;
using ds::FastResetArray;

template <class FlowNetworkPolicy = Mandatory,
          class FlowExecutionPolicy = Mandatory>
class TwoWayFlowRefiner final : public IRefiner,
                                private FlowRefinerBase<FlowExecutionPolicy>{
  using Network = typename FlowNetworkPolicy::Network;
  using Base = FlowRefinerBase<FlowExecutionPolicy>;

 private:
  static constexpr bool enable_heavy_assert = false;
  static constexpr bool debug = false;

 public:
  TwoWayFlowRefiner(Hypergraph& hypergraph, const Context& context) :
    Base(hypergraph, context),
    _flow_network(_hg, _context),
    _maximum_flow(FlowAlgorithmFactory<Network>::getInstance().createObject(
                    _context.local_search.flow.algorithm, hypergraph, _context, _flow_network)),
    _quotient_graph(nullptr),
    _visited(static_cast<size_t>(_hg.initialNumNodes()) + _hg.initialNumEdges()),
    _block0(0),
    _block1(1),
    _ignore_flow_execution_policy(false) { }

  TwoWayFlowRefiner(const TwoWayFlowRefiner&) = delete;
  TwoWayFlowRefiner(TwoWayFlowRefiner&&) = delete;
  TwoWayFlowRefiner& operator= (const TwoWayFlowRefiner&) = delete;
  TwoWayFlowRefiner& operator= (TwoWayFlowRefiner&&) = delete;

  /*
   * The 2way flow refiner can be used in combination with other
   * refiners.
   *
   * k-Way Flow Refiner:
   * Performs active block scheduling on the quotient graph. Therefore
   * the quotient graph is already initialized and we have to pass
   * the two blocks which are selected for a pairwise refinement.
   */
  void updateConfiguration(const PartitionID block0,
                           const PartitionID block1,
                           QuotientGraphBlockScheduler* quotientGraph,
                           bool ignoreFlowExecutionPolicy) {
    _block0 = block0;
    _block1 = block1;
    _quotient_graph = quotientGraph;
    _ignore_flow_execution_policy = ignoreFlowExecutionPolicy;
  }

 private:
  friend class TwoWayFlowRefinerTest;

  std::vector<Move> rollbackImpl() override final {
    return Base::rollback();
  }

  bool refineImpl(std::vector<HypernodeID>&,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics& best_metrics) override final {
    if (!_flow_execution_policy.executeFlow(_hg) && !_ignore_flow_execution_policy) {
      return false;
    }

    // Store original partition for rollback, because we have to update
    // gain cache of twoway fm refiner
    if (_context.local_search.algorithm == RefinementAlgorithm::twoway_fm_flow) {
      Base::storeOriginalPartitionIDs();
    }

    // Construct quotient graph, if it is not set before
    bool delete_quotientgraph_after_flow = false;
    if (!_quotient_graph) {
      delete_quotientgraph_after_flow = true;
      _quotient_graph = new QuotientGraphBlockScheduler(_hg, _context);
      _quotient_graph->buildQuotientGraph();
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
      _flow_network.reset(_block0, _block1);

      DBG << "";
      DBG << V(alpha);

      // Initialize set of cut hyperedges for blocks '_block0' and '_block1'
      std::vector<HyperedgeID> cut_hes;
      HyperedgeWeight cut_weight = 0;
      for (const HyperedgeID& he : _quotient_graph->blockPairCutHyperedges(_block0, _block1)) {
        cut_weight += _hg.edgeWeight(he);
        cut_hes.push_back(he);
      }

      // Heurist 1: Don't execute 2way flow refinement for adjacent blocks
      //            in the quotient graph with a small cut
      if (_context.local_search.flow.ignore_small_hyperedge_cut &&
          cut_weight <= 10 && !isRefinementOnLastLevel()) {
        return improvement;
      }

      // If cut is 0 no improvement is possible
      if (cut_hes.size() == 0) {
        DBG << "Cut is zero";
        break;
      }

      std::shuffle(cut_hes.begin(), cut_hes.end(),Randomize::instance().getGenerator());

      // Build Flow Problem
      CutBuildPolicy::buildFlowNetwork(_hg, _context, _flow_network,
                                       cut_hes, alpha, _block0, _block1,
                                       _visited);
      const HyperedgeWeight cut_flow_network_before = _flow_network.build(_block0, _block1);
      DBG << V(_flow_network.numNodes()) << V(_flow_network.numEdges());

      printMetric();

      // Find minimum (S,T)-bipartition
      const HyperedgeWeight cut_flow_network_after = _maximum_flow->minimumSTCut(_block0, _block1);

      // Maximum Flow algorithm returns infinity, if all
      // hypernodes contained in the flow problem are either
      // sources or sinks
      if (cut_flow_network_after == Network::kInfty) {
        DBG << "Trivial Cut";
        break;
      }

      const HyperedgeWeight delta = cut_flow_network_before - cut_flow_network_after;
      ASSERT(cut_flow_network_before >= cut_flow_network_after,
             "Flow calculation should not increase cut!"
             << V(cut_flow_network_before) << V(cut_flow_network_after));
      HEAVY_REFINEMENT_ASSERT(best_metrics.getMetric(_context.partition.mode, _context.partition.objective) - delta
             == metrics::objective(_hg, _context.partition.objective),
             "Maximum Flow is not the minimum cut!"
             << V(_context.partition.objective)
             << V(best_metrics.getMetric(_context.partition.mode, _context.partition.objective))
             << V(delta)
             << V(metrics::objective(_hg, _context.partition.objective)));

      const double current_imbalance = metrics::imbalance(_hg, _context);
      const HyperedgeWeight old_metric = best_metrics.getMetric(_context.partition.mode, _context.partition.objective);
      const HyperedgeWeight current_metric = old_metric - delta;

      DBG << V(cut_flow_network_before)
          << V(cut_flow_network_after)
          << V(delta)
          << V(old_metric)
          << V(current_metric);

      printMetric();

      const bool equal_metric = current_metric == best_metrics.getMetric(_context.partition.mode, _context.partition.objective);
      const bool improved_metric = current_metric < best_metrics.getMetric(_context.partition.mode, _context.partition.objective);
      const bool improved_imbalance = current_imbalance < best_metrics.imbalance;
      const bool is_feasible_partition = current_imbalance <= _context.partition.epsilon;

      bool current_improvement = false;
      if ((improved_metric && (is_feasible_partition || improved_imbalance)) ||
          (equal_metric && improved_imbalance)) {
        best_metrics.updateMetric(current_metric, _context.partition.mode, _context.partition.objective);
        best_metrics.imbalance = current_imbalance;
        improvement = true;
        current_improvement = true;

        alpha *= (alpha == _context.local_search.flow.alpha ? 2.0 : 4.0);
      }

      _maximum_flow->rollback(current_improvement);

      // Perform moves in quotient graph in order to update
      // cut hyperedges between adjacent blocks.
      if (current_improvement) {
        for (const HypernodeID& hn : _flow_network.hypernodes()) {
          const PartitionID from = _hg.partID(hn);
          const PartitionID to = _maximum_flow->getOriginalPartition(hn);
          if (from != to) {
            _quotient_graph->changeNodePart(hn, from, to);
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

    printMetric(true, true);

    // Delete quotient graph
    if (delete_quotientgraph_after_flow) {
      delete _quotient_graph;
      _quotient_graph = nullptr;
    }

    return improvement;
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
    _flow_execution_policy.initialize(_hg, _context);
  }

  using IRefiner::_is_initialized;

  using Base::_hg;
  using Base::_context;
  using Base::_original_part_id;
  using Base::_flow_execution_policy;

  Network _flow_network;
  std::unique_ptr<MaximumFlow<Network> > _maximum_flow;
  QuotientGraphBlockScheduler* _quotient_graph;
  FastResetFlagArray<> _visited;
  PartitionID _block0;
  PartitionID _block1;
  bool _ignore_flow_execution_policy;
};
}  // namespace kahypar
