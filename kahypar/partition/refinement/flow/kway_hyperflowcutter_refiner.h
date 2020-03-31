/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Lars Gottesbüren <lars.gottesbueren@kit.edu>
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
#include <cmath>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_array.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/flow/2way_hyperflowcutter_refiner.h"
#include "kahypar/partition/refinement/flow/flow_refiner_base.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/utils/time_limit.h"

namespace kahypar {
using ds::SparseSet;
using ds::FastResetArray;

// TODO(schlag): After successful tests, we need to consolidate this with kway_flow_refiner.h
template <class FlowExecutionPolicy = Mandatory>
class KWayHyperFlowCutterRefiner final : public IRefiner,
                                         private FlowRefinerBase<FlowExecutionPolicy>{
 private:
  using Base = FlowRefinerBase<FlowExecutionPolicy>;
  static constexpr bool debug = false;

 public:
  KWayHyperFlowCutterRefiner(Hypergraph& hypergraph, const Context& context) :
    Base(hypergraph, context),
    _twoway_flow_refiner(_hg, _context),
    _num_improvements(context.partition.k, std::vector<size_t>(context.partition.k, 0)) { }

  KWayHyperFlowCutterRefiner(const KWayHyperFlowCutterRefiner&) = delete;
  KWayHyperFlowCutterRefiner(KWayHyperFlowCutterRefiner&&) = delete;
  KWayHyperFlowCutterRefiner& operator= (const KWayHyperFlowCutterRefiner&) = delete;
  KWayHyperFlowCutterRefiner& operator= (KWayHyperFlowCutterRefiner&&) = delete;

 private:
// friend class KWayHyperFlowCutterRefinerTest;

  std::vector<Move> rollbackImpl() override final {
    return Base::rollback();
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                  const UncontractionGainChanges& changes, Metrics& best_metrics) override final {
    if (!_flow_execution_policy.executeFlow(_hg)) {
      return false;
    }

    // Store original partition for rollback, because we have to update
    // gain cache of kway fm refiner
    if (_context.local_search.algorithm == RefinementAlgorithm::kway_fm_hyperflow_cutter_km1 ||
        _context.local_search.algorithm == RefinementAlgorithm::kway_fm_hyperflow_cutter) {
      Base::storeOriginalPartitionIDs();
    }

    DBG << "############### Active Block Scheduling ###############";
    DBG << V(_hg.currentNumNodes()) << V(_hg.initialNumNodes());
    printMetric();

    QuotientGraphBlockScheduler scheduler(_hg, _context);
    scheduler.buildQuotientGraph();

    // Active Block Scheduling
    bool improvement = false;
    bool active_block_exist = true;
    std::vector<bool> active_blocks(_context.partition.k, true);
    size_t current_round = 1;
    while (active_block_exist && !time_limit::isSoftTimeLimitExceeded(_context)) {
      scheduler.randomShuffleQuotientEdges();
      std::vector<bool> tmp_active_blocks(_context.partition.k, false);
      active_block_exist = false;
      for (const auto& e : scheduler.quotientGraphEdges()) {
        const PartitionID block_0 = e.first;
        const PartitionID block_1 = e.second;

        // Heuristic: If a flow refinement never improved a bipartition,
        //            we ignore the refinement for these blocks in the
        //            second iteration of active block scheduling
        if (current_round > 1 && _num_improvements[block_0][block_1] == 0)
          continue;

        if (active_blocks[block_0] || active_blocks[block_1]) {
          _twoway_flow_refiner.updateConfiguration(block_0, block_1, &scheduler, true);
          const bool improved = _twoway_flow_refiner.refine(refinement_nodes, max_allowed_part_weights, changes, best_metrics);
          if (improved) {
            // DBG << "Improvement found beetween blocks " << block_0 << " and " << block_1 << " in round #" << current_round;
            // printMetric();
            improvement = true;
            if (_twoway_flow_refiner.refinement_result >= RefinementResult::GlobalBalanceImproved) {
              // don't mark blocks as active, if cut stayed the same and global balance did not improve.
              // haven't observed it yet, but only reducing the weight difference between block_0 and block_1
              // might lead to an infinite loop for k > 2
              active_block_exist = true;
              tmp_active_blocks[block_0] = true;
              tmp_active_blocks[block_1] = true;
              _num_improvements[block_0][block_1]++;
            }
          }
        }

        if (_context.partition.time_limit_triggered) {
          break;
        }
      }
      current_round++;
      std::swap(active_blocks, tmp_active_blocks);
    }

    if (_hg.currentNumNodes() == _hg.initialNumNodes()) {            // on last level
      _twoway_flow_refiner.reportRunningTime();
    }
    // printMetric(true, true);
    return improvement;
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

  void initializeImpl(const HyperedgeWeight max_gain) override final {
    _is_initialized = true;
    _flow_execution_policy.initialize(_hg, _context);
    _twoway_flow_refiner.initialize(max_gain);
  }

  using IRefiner::_is_initialized;

  using Base::_hg;
  using Base::_context;
  using Base::_original_part_id;
  using Base::_flow_execution_policy;

  TwoWayHyperFlowCutterRefiner<FlowExecutionPolicy> _twoway_flow_refiner;
  std::vector<std::vector<size_t> > _num_improvements;
};
}  // namespace kahypar
