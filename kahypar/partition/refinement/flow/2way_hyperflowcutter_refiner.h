/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Lars Gottesbüren <lars.gottesbueren@kit.edu>
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <vector>

#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"
#include "kahypar/partition/refinement/move.h"

namespace kahypar {
template <class FlowExecutionPolicy = Mandatory>
class TwoWayHyperFlowCutterRefiner final : public IRefiner,
                                           private FlowRefinerBase<FlowExecutionPolicy>{
  using Base = FlowRefinerBase<FlowExecutionPolicy>;

 private:
  static constexpr bool debug = false;

 public:
  TwoWayHyperFlowCutterRefiner(Hypergraph& hypergraph, const Context& context) :
    Base(hypergraph, context),
    _quotient_graph(nullptr),
    _block0(0),
    _block1(1),
    _ignore_flow_execution_policy(false) { }

  TwoWayHyperFlowCutterRefiner(const TwoWayHyperFlowCutterRefiner&) = delete;
  TwoWayHyperFlowCutterRefiner(TwoWayHyperFlowCutterRefiner&&) = delete;
  TwoWayHyperFlowCutterRefiner& operator= (const TwoWayHyperFlowCutterRefiner&) = delete;
  TwoWayHyperFlowCutterRefiner& operator= (TwoWayHyperFlowCutterRefiner&&) = delete;

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
    if (_context.local_search.algorithm == RefinementAlgorithm::twoway_fm_hyperflow_cutter) {
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

    LOG << "Refine " << V(_block0) << "and" << V(_block1);
    LOG << "2-Way Hyperflow Cutter";

    bool improvement = false;

    // Delete quotient graph
    if (delete_quotientgraph_after_flow) {
      delete _quotient_graph;
      _quotient_graph = nullptr;
    }

    return improvement;
  }

  void initializeImpl(const HyperedgeWeight) override final {
    _is_initialized = true;
    _flow_execution_policy.initialize(_hg, _context);
  }

  bool isRefinementOnLastLevel() {
    return _hg.currentNumNodes() == _hg.initialNumNodes();
  }

  using IRefiner::_is_initialized;
  using Base::_hg;
  using Base::_context;
  using Base::_original_part_id;
  using Base::_flow_execution_policy;
  QuotientGraphBlockScheduler* _quotient_graph;
  PartitionID _block0;
  PartitionID _block1;
  bool _ignore_flow_execution_policy;
};
}  // namespace kahypar
