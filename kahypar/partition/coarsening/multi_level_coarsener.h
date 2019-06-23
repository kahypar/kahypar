/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <limits>
#include <string>
#include <vector>


#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_partition_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "kahypar/partition/coarsening/vertex_pair_rater.h"

namespace kahypar {
template <class ScorePolicy = HeavyEdgeScore,
          class HeavyNodePenaltyPolicy = NoWeightPenalty,
          class CommunityPolicy = UseCommunityStructure,
          class RatingPartitionPolicy = NormalPartitionPolicy,
          class AcceptancePolicy = BestRatingPreferringUnmatched<>,
          class FixedVertexPolicy = AllowFreeOnFixedFreeOnFreeFixedOnFixed,
          typename RatingType = RatingType>
class MultiLevelCoarsener final : public ICoarsener,
                                  private VertexPairCoarsenerBase<>{
 private:
  static constexpr bool debug = false;

  static constexpr HypernodeID kInvalidTarget = std::numeric_limits<HypernodeID>::max();

  using Base = VertexPairCoarsenerBase;
  using Rater = VertexPairRater<ScorePolicy,
                                HeavyNodePenaltyPolicy,
                                CommunityPolicy,
                                RatingPartitionPolicy,
                                AcceptancePolicy,
                                FixedVertexPolicy,
                                RatingType>;
  using Rating = typename Rater::Rating;

 public:
  MultiLevelCoarsener(Hypergraph& hypergraph, const Context& context,
                      const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _levels(),
    _rater(_hg, _context) { }

  ~MultiLevelCoarsener() override = default;

  MultiLevelCoarsener(const MultiLevelCoarsener&) = delete;
  MultiLevelCoarsener& operator= (const MultiLevelCoarsener&) = delete;

  MultiLevelCoarsener(MultiLevelCoarsener&&) = delete;
  MultiLevelCoarsener& operator= (MultiLevelCoarsener&&) = delete;

 private:
  void coarsenImpl(const HypernodeID limit) override final {
    int pass_nr = 0;
    std::vector<HypernodeID> current_hns;

    _levels.push_back(0);
    HypernodeID next_level_at = ceil(static_cast<double>(_hg.currentNumNodes()) /
                                     _context.coarsening.contraction_factor);
    DBG1 << V(next_level_at);

    while (_hg.currentNumNodes() > limit) {
      DBG << V(pass_nr);
      DBG << V(_hg.currentNumNodes());
      DBG << V(_hg.currentNumEdges());
      _rater.resetMatches();
      current_hns.clear();
      const HypernodeID num_hns_before_pass = _hg.currentNumNodes();
      for (const HypernodeID& hn : _hg.nodes()) {
        current_hns.push_back(hn);
      }
      Randomize::instance().shuffleVector(current_hns, current_hns.size());

      for (const HypernodeID& hn : current_hns) {
        if (_hg.nodeIsEnabled(hn)) {
          const Rating rating = _rater.rate(hn);

          if (rating.target != kInvalidTarget) {
            _rater.markAsMatched(hn);
            _rater.markAsMatched(rating.target);
            performContraction(hn, rating.target);
          }

          if (_hg.currentNumNodes() == next_level_at) {
            DBG1 << "New Level starting with " << V(_hg.currentNumNodes()) << V(_history.size());
            _levels.push_back(_history.size());
            next_level_at = ceil(static_cast<double>(_hg.currentNumNodes()) /
                                 _context.coarsening.contraction_factor);
          }

          if (_hg.currentNumNodes() <= limit) {
            break;
          }
        }
      }

      if (num_hns_before_pass == _hg.currentNumNodes()) {
        break;
      }
      ++pass_nr;
    }
    _context.stats.add(StatTag::Coarsening, "HnsAfterCoarsening", _hg.currentNumNodes());
  }

  bool uncoarsenImpl(IRefiner& refiner) override final {
    Metrics current_metrics = { metrics::hyperedgeCut(_hg),
                                metrics::km1(_hg),
                                metrics::imbalance(_hg, _context) };
    HyperedgeWeight initial_objective = std::numeric_limits<HyperedgeWeight>::min();

    switch (_context.partition.objective) {
      case Objective::cut:
        initial_objective = current_metrics.cut;
        _context.stats.set(StatTag::InitialPartitioning, "inititalCut", initial_objective);
        break;
      case Objective::km1:
        initial_objective = current_metrics.km1;
        _context.stats.set(StatTag::InitialPartitioning, "inititalKm1", initial_objective);
        break;
      default:
        LOG << "Unknown Objective";
        exit(-1);
    }

    _context.stats.set(StatTag::InitialPartitioning, "initialImbalance", current_metrics.imbalance);

    initializeRefiner(refiner);
    UncontractionGainChanges changes;
    changes.representative.push_back(0);
    changes.contraction_partner.push_back(0);
    std::vector<HypernodeID> refinement_nodes;
    ds::FastResetFlagArray<> contained(_hg.initialNumNodes());
    while (!_levels.empty()) {
      const size_t prev_level = _levels.back();
      _levels.pop_back();
      ASSERT(!_history.empty());
      DBG1 << "Undoing until " << V(prev_level);
      refinement_nodes.clear();
      contained.reset();
      while (_history.size() >= prev_level && !_history.empty()) {
        restoreParallelHyperedges();
        restoreSingleNodeHyperedges();

        DBG << "Uncontracting: (" << _history.back().contraction_memento.u << ","
            << _history.back().contraction_memento.v << ")";
        const HypernodeID u = _history.back().contraction_memento.u;
        const HypernodeID v = _history.back().contraction_memento.v;
        if (!contained[u]) {
          refinement_nodes.push_back(_history.back().contraction_memento.u);
          contained.set(u, true);
        }
        if (!contained[v]) {
          refinement_nodes.push_back(_history.back().contraction_memento.v);
          contained.set(v, true);
        }

        if (_hg.currentNumNodes() > _max_hn_weights.back().num_nodes) {
          _max_hn_weights.pop_back();
        }

        _hg.uncontract(_history.back().contraction_memento);

        _history.pop_back();
      }
      performLocalSearch(refiner, refinement_nodes, current_metrics, changes);
    }
    ASSERT(_history.empty());
    // This currently cannot be guaranteed for RB-partitioning and k != 2^x, since it might be
    // possible that 2FM cannot re-adjust the part weights to be less than Lmax0 and Lmax1.
    // In order to guarantee this, 2FM would have to force rebalancing by sacrificing cut-edges.
    // ASSERT(current_imbalance <= _context.partition.epsilon,
    //        "balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg, _context)
    //        << ">" << __context.partition.epsilon);
    _context.stats.set(StatTag::LocalSearch, "finalImbalance", current_metrics.imbalance);

    bool improvement_found = false;
    switch (_context.partition.objective) {
      case Objective::cut:
        _context.stats.set(StatTag::LocalSearch, "finalCut", current_metrics.cut);
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
        _context.stats.set(StatTag::LocalSearch, "finalKm1", current_metrics.km1);
        improvement_found = current_metrics.km1 < initial_objective;
        break;
      default:
        LOG << "Unknown Objective";
        exit(-1);
    }

    return improvement_found;
  }

  std::vector<size_t> _levels;
  using Base::_pq;
  using Base::_hg;
  using Base::_context;
  using Base::_history;
  Rater _rater;
};
}  // namespace kahypar
