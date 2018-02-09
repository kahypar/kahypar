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
#include <cmath>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/flow/2way_flow_refiner.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"
#include "kahypar/datastructure/fast_reset_array.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_set.h"

namespace kahypar {

using ds::SparseSet;
using ds::FastResetArray;

#define invalidHE std::numeric_limits<HyperedgeID>::max()
#define invalidHN std::numeric_limits<HypernodeID>::max()


template<class FlowNetworkPolicy = Mandatory,
         class FlowExecutionPolicy = Mandatory>
class KWayFlowRefiner final : public IRefiner {
using FlowNetwork = typename FlowNetworkPolicy::Network;

 public:
  KWayFlowRefiner(Hypergraph& hypergraph, const Context& context) :
    _hg(hypergraph),
    _context(context),
    _twoWayFlowRefiner(_hg, _context),
    _flowExecutionPolicy(),
    _originalPartId(_hg.initialNumNodes(), -1),
    _destinationPartId(_hg.initialNumNodes(), -1),
    _movedHNs(_hg.initialNumNodes()),
    _numImprovements(context.partition.k, std::vector<size_t>(context.partition.k, 0)),
    _rollback(false) {}

  KWayFlowRefiner(const KWayFlowRefiner&) = delete;
  KWayFlowRefiner(KWayFlowRefiner&&) = delete;
  KWayFlowRefiner& operator= (const KWayFlowRefiner&) = delete;
  KWayFlowRefiner& operator= (KWayFlowRefiner&&) = delete;

  std::pair<const NodeID *, const NodeID *> movedHypernodes() {
      return std::make_pair(_movedHNs.begin(), _movedHNs.end());
  }

  size_t numMovedNodes() {
      return _movedHNs.size();
  }

  PartitionID getDestinationPartID(const HypernodeID hn) const {
      return _destinationPartId.get(hn);
  }

  /*
   * The kway flow refiner can be used in combination with the
   * FM refiner. 
   *
   * FM Refiner:
   * FM refiners performs delta gain updates. Moving nodes inside the
   * flow refiner without notify the FM refiner invalidate the gain
   * cache. Therefore we need to rollback all changes after flow
   * refinement and let the FM refiner perform the moves with
   * the correct update of the gain cache.
   */
  void enableRollback() {
      _rollback = true;
  }

 private:
  friend class KWayFlowRefinerTest;
  static constexpr bool debug = false;

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                  const UncontractionGainChanges& changes,
                  Metrics& best_metrics) override final {
    if (!_flowExecutionPolicy.executeFlow(_hg)) {
        return false;
    }

    DBG << "############### Active Block Scheduling ###############";
    DBG << V(_hg.currentNumNodes()) << V(_hg.initialNumNodes());
    printMetric();

    // Stores original partition, if a rollback after
    // flow execution is required
    HyperedgeWeight old_km1 = best_metrics.km1;
    HyperedgeWeight old_cut = best_metrics.cut;
    double old_imbalance = best_metrics.imbalance;
    if (_rollback) {
        storeOriginalPartitionIDs();
    }

    // Initialize Quotient Graph
    // 1.) Contains edges between each adjacent block of the partition
    // 2.) Contains for each edge all hyperedges, which are cut in the
    //     corresponding bipartition
    // NOTE(heuer): If anything goes wrong in integration experiments,
    //              this should be moved inside while loop.
    QuotientGraphBlockScheduler scheduler(_hg, _context);
    scheduler.buildQuotientGraph();

    // Active Block Scheduling
    bool improvement = false;
    bool active_block_exist = true;
    std::vector<bool> active_blocks(_context.partition.k, true);
    size_t current_round = 1;
    while (active_block_exist) {
        scheduler.randomShuffleQoutientEdges();
        std::vector<bool> tmp_active_blocks(_context.partition.k, false);
        active_block_exist = false;
        for (const auto& e : scheduler.qoutientGraphEdges()) {
            PartitionID block_0 = e.first;
            PartitionID block_1 = e.second;

            // Heuristic: If a flow refinement never improved a bipartition,
            //            we ignore the refinement for these block in the
            //            second iteration of active block scheduling
            if (_context.local_search.flow.use_improvement_history &&
                current_round > 1 && _numImprovements[block_0][block_1] == 0 ) {
                continue;
            }

            if (active_blocks[block_0] || active_blocks[block_1]) {
                 _twoWayFlowRefiner.updateTwoWayFlowRefinementConfiguration(block_0, block_1,
                                                                            &scheduler, false, true);
                bool improved = _twoWayFlowRefiner.refine(refinement_nodes,
                                                          max_allowed_part_weights,
                                                          changes,
                                                          best_metrics);
                if (improved) {
                    DBG << "Improvement found beetween blocks " << block_0 << " and "
                                                                << block_1 << " in round #"
                                                                << current_round;
                    printMetric();
                    improvement = true;
                    active_block_exist = true;
                    tmp_active_blocks[block_0] = true;
                    tmp_active_blocks[block_1] = true;
                    _numImprovements[block_0][block_1]++;
                }
            }
        }
        current_round++;
        std::swap(active_blocks, tmp_active_blocks);
    }

    // Restore original partition, if a rollback after
    // flow execution is required
    if (_rollback) {
        restoreOriginalPartitionIDs();
        best_metrics.km1 = old_km1;
        best_metrics.cut = old_cut;
        best_metrics.imbalance = old_imbalance;
    }

    printMetric(true, true);


    return improvement;
  }


  void storeOriginalPartitionIDs() {
    _movedHNs.clear();
    _originalPartId.resetUsedEntries();
    _destinationPartId.resetUsedEntries();
    for (const HypernodeID& hn : _hg.nodes()) {
        _originalPartId.set(hn, _hg.partID(hn));
        _destinationPartId.set(hn, _hg.partID(hn));
    }
  }

  void restoreOriginalPartitionIDs() {
    for (const HypernodeID& hn : _hg.nodes()) {
        PartitionID from = _hg.partID(hn);
        PartitionID to = _originalPartId.get(hn);
        if (from != to) {
            _movedHNs.add(hn);
            _destinationPartId.set(hn, from);
            _hg.changeNodePart(hn, from, to);
        }
    }
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
    _flowExecutionPolicy.initialize(_hg, _context);
    _twoWayFlowRefiner.initialize(max_gain);
  }

  using IRefiner::_is_initialized;

  Hypergraph& _hg;
  const Context& _context;
  TwoWayFlowRefiner<FlowNetworkPolicy, FlowExecutionPolicy> _twoWayFlowRefiner;
  FlowExecutionPolicy _flowExecutionPolicy;
  FastResetArray<PartitionID> _originalPartId;
  FastResetArray<PartitionID> _destinationPartId;
  SparseSet<HypernodeID> _movedHNs;
  std::vector<std::vector<size_t>> _numImprovements;
  bool _rollback;
};
}  // namespace kahypar
