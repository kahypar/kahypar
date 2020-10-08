/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include "kahypar/datastructure/binary_heap.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/int_to_type.h"
#include "kahypar/partition/coarsening/coarsener_base.h"
#include "kahypar/partition/coarsening/vertex_pair_rater.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/utils/progress_bar.h"
#include "kahypar/utils/randomize.h"
#include "kahypar/utils/time_limit.h"

namespace kahypar {
template <class PrioQueue = ds::BinaryMaxHeap<HypernodeID, RatingType> >
class VertexPairCoarsenerBase : public CoarsenerBase {
 private:
  static constexpr bool debug = false;

 public:
  VertexPairCoarsenerBase(Hypergraph& hypergraph, const Context& context,
                          const HypernodeWeight weight_of_heaviest_node) :
    CoarsenerBase(hypergraph, context, weight_of_heaviest_node),
    _pq(_hg.initialNumNodes()) { }

  ~VertexPairCoarsenerBase() override = default;

  VertexPairCoarsenerBase(const VertexPairCoarsenerBase&) = delete;
  VertexPairCoarsenerBase& operator= (const VertexPairCoarsenerBase&) = delete;

  VertexPairCoarsenerBase(VertexPairCoarsenerBase&&) = delete;
  VertexPairCoarsenerBase& operator= (VertexPairCoarsenerBase&&) = delete;

 protected:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  bool doUncoarsen(IRefiner& refiner) {
    Metrics current_metrics = { metrics::hyperedgeCut(_hg),
                                metrics::km1(_hg),
                                metrics::imbalance(_hg, _context) };
    HyperedgeWeight initial_objective = std::numeric_limits<HyperedgeWeight>::min();

    switch (_context.partition.objective) {
      case Objective::cut:
        initial_objective = current_metrics.cut;
        break;
      case Objective::km1:
        initial_objective = current_metrics.km1;
        break;
      default:
        LOG << "Unknown Objective";
        exit(-1);
    }

    if (_context.type == ContextType::main) {
      _context.stats.set(StatTag::InitialPartitioning, "initialCut", current_metrics.cut);
      _context.stats.set(StatTag::InitialPartitioning, "initialKm1", current_metrics.km1);
      _context.stats.set(StatTag::InitialPartitioning, "initialImbalance",
                         current_metrics.imbalance);
    }

    CoarsenerBase::initializeRefiner(refiner);
    std::vector<HypernodeID> refinement_nodes(2, 0);
    UncontractionGainChanges changes;
    changes.representative.push_back(0);
    changes.contraction_partner.push_back(0);

    ProgressBar uncontraction_progress_bar(
      _hg.initialNumNodes(), current_metrics.getMetric(
        _context.partition.mode, _context.partition.objective),
      _context.partition.verbose_output && _context.type == ContextType::main);
    uncontraction_progress_bar += _hg.currentNumNodes();
    while (!_history.empty()) {
      if (time_limit::isSoftTimeLimitExceeded(_context, _history.size())) {
        /*
         * There are two ways to implement this time limit.
         * 1) skip refinement but perform full uncontractions, including updates. This can be slow
         * 2) only perform the partition projection of the uncontractions. This leaves the dynamic hypergraph in a "broken" state, i.e.
         * hyperedges are still as in the contracted hypergraph, whereas vertices are unpacked and activated.
         *
         * We implement 2) because all considered output measures are either already maintained (i.e. part weights and imbalance)
         * or can be computed on the state of hyperedges in the contracted hypergraph.
         * This means, this time limit should not be used to
         */
          while (!_history.empty()) {
            _hg.restoreMemento(_history.back().contraction_memento);
            _history.pop_back();
          }
        break;
      }

      refinement_nodes.clear();
      refinement_nodes.push_back(_history.back().contraction_memento.u);
      refinement_nodes.push_back(_history.back().contraction_memento.v);

      uncontract(changes);

      CoarsenerBase::performLocalSearch(refiner, refinement_nodes, current_metrics, changes);
      changes.representative[0] = 0;
      changes.contraction_partner[0] = 0;

      // Update Progress Bar
      uncontraction_progress_bar += 1;
      uncontraction_progress_bar.setObjective(current_metrics.getMetric(
        _context.partition.mode, _context.partition.objective));
    }

    // This currently cannot be guaranteed for RB-partitioning and k != 2^x, since it might be
    // possible that 2FM cannot re-adjust the part weights to be less than Lmax0 and Lmax1.
    // In order to guarantee this, 2FM would have to force rebalancing by sacrificing cut-edges.
    // ASSERT(current_imbalance <= _context.partition.epsilon,
    //        "balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg, _context)
    //        << ">" << __context.partition.epsilon);
    // _context.stats.set(StatTag::LocalSearch, "finalImbalance", current_metrics.imbalance);

    bool improvement_found = false;
    switch (_context.partition.objective) {
      case Objective::cut:
        // _context.stats.set(StatTag::LocalSearch, "finalCut", current_metrics.cut);
        improvement_found = current_metrics.cut < initial_objective;
        break;
      case Objective::km1:
        if (_context.partition.mode == Mode::recursive_bisection) {
          // In recursive bisection-based (initial) partitioning, km1
          // is optimized using TwoWayFM and cut-net splitting. Since
          // TwoWayFM optimizes cut, current_metrics.km1 is not updated
          // during local search (it is currently only updated/maintained
          // during k-way k-1 refinement). In order to provide correct outputs,
          // we explicitly calculated the metric after uncoarsening.
          current_metrics.km1 = metrics::km1(_hg);
        }
        // _context.stats.set(StatTag::LocalSearch, "finalKm1", current_metrics.km1);
        improvement_found = current_metrics.km1 < initial_objective;
        break;
      default:
        LOG << "Unknown Objective";
        exit(-1);
    }

    return improvement_found;
  }

  void uncontract(UncontractionGainChanges& changes) {
    DBG << "Uncontracting: (" << _history.back().contraction_memento.u << ","
        << _history.back().contraction_memento.v << ")";
    restoreParallelHyperedges();
    restoreSingleNodeHyperedges();

    if (_hg.currentNumNodes() > _max_hn_weights.back().num_nodes) {
      _max_hn_weights.pop_back();
    }

    if (_context.local_search.algorithm == RefinementAlgorithm::twoway_fm ||
        _context.local_search.algorithm == RefinementAlgorithm::twoway_fm_hyperflow_cutter) {
      _hg.uncontract(_history.back().contraction_memento, changes,
                     meta::Int2Type<static_cast<int>(RefinementAlgorithm::twoway_fm)>());
    } else {
      _hg.uncontract(_history.back().contraction_memento);
    }
    _history.pop_back();
  }

  template <typename Rater>
  void rateAllHypernodes(Rater& rater,
                         std::vector<HypernodeID>& target) {
    std::vector<HypernodeID> permutation;
    createHypernodePermutation(permutation);
    for (const HypernodeID hn : permutation) {
      const typename Rater::Rating rating = rater.rate(hn);
      if (rating.valid) {
        _pq.push(hn, rating.value);
        target[hn] = rating.target;
      }
    }
  }

  void createHypernodePermutation(std::vector<HypernodeID>& permutation) {
    permutation.reserve(_hg.initialNumNodes());
    for (const HypernodeID& hn : _hg.nodes()) {
      permutation.push_back(hn);
    }
    Randomize::instance().shuffleVector(permutation, permutation.size());
  }

  using CoarsenerBase::_hg;
  using CoarsenerBase::_context;
  PrioQueue _pq;
};
}  // namespace kahypar
