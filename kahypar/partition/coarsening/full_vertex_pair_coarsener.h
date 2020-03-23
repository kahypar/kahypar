/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_partition_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "kahypar/partition/coarsening/vertex_pair_coarsener_base.h"
#include "kahypar/partition/coarsening/vertex_pair_rater.h"

namespace kahypar {
template <class ScorePolicy = HeavyEdgeScore,
          class HeavyNodePenaltyPolicy = MultiplicativePenalty,
          class CommunityPolicy = UseCommunityStructure,
          class RatingPartitionPolicy = NormalPartitionPolicy,
          class AcceptancePolicy = BestRatingWithTieBreaking<>,
          class FixedVertexPolicy = AllowFreeOnFixedFreeOnFreeFixedOnFixed,
          typename RatingType = RatingType>
class FullVertexPairCoarsener final : public ICoarsener,
                                      private VertexPairCoarsenerBase<>{
 private:
  static constexpr bool debug = false;

  using Rater = VertexPairRater<ScorePolicy,
                                HeavyNodePenaltyPolicy,
                                CommunityPolicy,
                                RatingPartitionPolicy,
                                AcceptancePolicy,
                                FixedVertexPolicy,
                                RatingType>;

  using Base = VertexPairCoarsenerBase;
  using Rating = typename Rater::Rating;

 public:
  FullVertexPairCoarsener(Hypergraph& hypergraph, const Context& context,
                          const HypernodeWeight weight_of_heaviest_node) :
    VertexPairCoarsenerBase(hypergraph, context, weight_of_heaviest_node),
    _rater(_hg, _context),
    _target(hypergraph.initialNumNodes()) { }

  ~FullVertexPairCoarsener() override = default;

  FullVertexPairCoarsener(const FullVertexPairCoarsener&) = delete;
  FullVertexPairCoarsener& operator= (const FullVertexPairCoarsener&) = delete;

  FullVertexPairCoarsener(FullVertexPairCoarsener&&) = delete;
  FullVertexPairCoarsener& operator= (FullVertexPairCoarsener&&) = delete;

 private:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void coarsenImpl(const HypernodeID limit) override final {
    _pq.clear();

    rateAllHypernodes(_rater, _target);

    ds::FastResetFlagArray<> rerated_hypernodes(_hg.initialNumNodes());
    // Used to prevent unnecessary re-rating of hypernodes that have been removed from
    // PQ because they are heavier than allowed.
    ds::FastResetFlagArray<> invalid_hypernodes(_hg.initialNumNodes());

    while (!_pq.empty() && _hg.currentNumNodes() > limit) {
      const HypernodeID rep_node = _pq.top();
      const HypernodeID contracted_node = _target[rep_node];
      DBG << "Contracting: (" << rep_node << ","
          << _target[rep_node] << ") prio:" << _pq.topKey() <<
        " deg(" << rep_node << ")=" << _hg.nodeDegree(rep_node) <<
        " w(" << rep_node << ")=" << _hg.nodeWeight(rep_node) <<
        " deg(" << contracted_node << ")=" << _hg.nodeDegree(contracted_node) <<
        " w(" << contracted_node << ")=" << _hg.nodeWeight(contracted_node);


      ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(_target[rep_node])
             <= _rater.thresholdNodeWeight());
      ASSERT(_pq.topKey() == _rater.rate(rep_node).value,
             V(_pq.topKey()) << V(_rater.rate(rep_node).value));
      ASSERT(!invalid_hypernodes[rep_node], V(rep_node));
      ASSERT(!invalid_hypernodes[contracted_node], V(contracted_node));

      performContraction(rep_node, contracted_node);

      ASSERT(_pq.contains(contracted_node), V(contracted_node));
      _pq.remove(contracted_node);

      // We re-rate the representative HN here, because it might not have any incident HEs left.
      // In this case, it will not get re-rated by the call to reRateAffectedHypernodes.
      updatePQandContractionTarget(rep_node, _rater.rate(rep_node), invalid_hypernodes);
      rerated_hypernodes.set(rep_node, true);

      reRateAffectedHypernodes(rep_node, rerated_hypernodes, invalid_hypernodes);
    }
  }

  bool uncoarsenImpl(IRefiner& refiner) override final {
    return doUncoarsen(refiner);
  }

  void reRateAffectedHypernodes(const HypernodeID rep_node,
                                ds::FastResetFlagArray<>& rerated_hypernodes,
                                ds::FastResetFlagArray<>& invalid_hypernodes) {
    for (const HyperedgeID& he : _hg.incidentEdges(rep_node)) {
      for (const HypernodeID& pin : _hg.pins(he)) {
        if (!rerated_hypernodes[pin] && !invalid_hypernodes[pin]) {
          const Rating rating = _rater.rate(pin);
          rerated_hypernodes.set(pin, true);
          updatePQandContractionTarget(pin, rating, invalid_hypernodes);
        }
      }
    }
    rerated_hypernodes.reset();
  }


  void updatePQandContractionTarget(const HypernodeID hn, const Rating& rating,
                                    ds::FastResetFlagArray<>& invalid_hypernodes) {
    if (rating.valid) {
      ASSERT(_pq.contains(hn), V(hn));
      DBG << "Updating prio of HN" << hn << ":" << _pq.getKey(hn) << "(target="
          << _target[hn] << ") --- >" << rating.value << "(target" << rating.target << ")";
      _pq.updateKey(hn, rating.value);
      _target[hn] = rating.target;
    } else if (_pq.contains(hn)) {
      // explicit containment check is necessary because of V-cycles. In this case, not
      // all hypernodes will be inserted into the PQ at the beginning, because of the
      // restriction that only hypernodes within the same part can be contracted.
      _pq.remove(hn);
      invalid_hypernodes.set(hn, true);
      _target[hn] = std::numeric_limits<HypernodeID>::max();
      DBG << "Progress [" << _hg.currentNumNodes() << "/"
          << _hg.initialNumNodes() << "]:HN" << hn
          << "\t(w=" << _hg.nodeWeight(hn) << "," << "deg=" << _hg.nodeDegree(hn)
          << ") did not find valid contraction partner.";
    }
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_context;
  using Base::_history;
  using Base::_hypergraph_pruner;
  Rater _rater;
  std::vector<HypernodeID> _target;
};
}  // namespace kahypar
