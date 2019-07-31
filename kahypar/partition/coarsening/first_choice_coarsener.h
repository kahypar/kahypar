/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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

#include "kahypar/datastructure/union_find.h"
#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/coarsening/vertex_pair_coarsener_base.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/level_policy.h"
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
          class CoarseningLevelPolicy = nLevel,
          typename RatingType = RatingType>
class FirstChoiceCoarsener final : public ICoarsener,
                                   private VertexPairCoarsenerBase<CoarseningLevelPolicy>{
 private:
  static constexpr bool debug = true;

  static constexpr HypernodeID kInvalidTarget = std::numeric_limits<HypernodeID>::max();

  using Base = VertexPairCoarsenerBase<CoarseningLevelPolicy>;
  using Rater = VertexPairRater<ScorePolicy,
                                HeavyNodePenaltyPolicy,
                                CommunityPolicy,
                                RatingPartitionPolicy,
                                AcceptancePolicy,
                                FixedVertexPolicy,
                                RatingType>;
  using Rating = typename Rater::Rating;

  struct Contraction {
    const HypernodeID representive_node;
    const HypernodeID contraction_partner;

    Contraction(const HypernodeID rep_node, const HypernodeID contr_partner) :
      representive_node(rep_node),
      contraction_partner(contr_partner) { }
  };

 public:
  FirstChoiceCoarsener(Hypergraph& hypergraph, const Context& context,
                       const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _rater(_hg, _context),
    _uf(_hg.initialNumNodes()),
    _target(_hg.initialNumNodes()),
    _enableRandomization(true) { }

  ~FirstChoiceCoarsener() override = default;

  FirstChoiceCoarsener(const FirstChoiceCoarsener&) = delete;
  FirstChoiceCoarsener& operator= (const FirstChoiceCoarsener&) = delete;

  FirstChoiceCoarsener(FirstChoiceCoarsener&&) = delete;
  FirstChoiceCoarsener& operator= (FirstChoiceCoarsener&&) = delete;

  void deactivateRandomization() {
    _enableRandomization = false;
  }

 private:
  FRIEND_TEST(AFirstChoiceCoarsener, ComputesCorrectContractionTargets);
  FRIEND_TEST(AFirstChoiceCoarsener, ComputesContractionForAHypernode);
  FRIEND_TEST(AFirstChoiceCoarsener, ComputesContractionForSeveralHypernodes);

  void coarsenImpl(const HypernodeID limit) override final {
    int pass_nr = 0;
    std::vector<HypernodeID> current_hns;
    _coarsening_levels.initialize(_hg, _context);

    while (_hg.currentNumNodes() > limit) {
      DBG << V(pass_nr);
      DBG << V(_hg.currentNumNodes());
      DBG << V(_hg.currentNumEdges());
      
      _rater.resetMatches();
      current_hns.clear();
      const HypernodeID num_hns_before_pass = _hg.currentNumNodes();

      // Compute target nodes for all vertices based on the
      // current hypergraph
      computeContractionTargetForAllHypernodes(current_hns);

      if ( _enableRandomization ) {
        Randomize::instance().shuffleVector(current_hns, current_hns.size());
      }

      for (const HypernodeID& hn : current_hns) {
        
        if ( _target[hn] != kInvalidTarget && _hg.nodeIsEnabled(hn) ) {
          Contraction contraction = computeContractionForHypernode(hn);
          Base::performContraction(contraction.representive_node, contraction.contraction_partner);

          // If number of nodes is smaller than contraction limit or we reach a new
          // level in the multi level policy, we terminate the current pass over all
          // nodes
          bool new_level = _coarsening_levels.update(_hg, _context, _history);
          if (  _hg.currentNumNodes() <= limit || 
               (new_level && _coarsening_levels.type() == Hierarchy::multi_level)  ) {
            break;
          }
        }

      }

      _uf.reset();

      if (num_hns_before_pass == _hg.currentNumNodes()) {
        break;
      }
      ++pass_nr;
    }
  }

  bool uncoarsenImpl(IRefiner& refiner) override final {
    return Base::doUncoarsen(refiner);
  }

  inline Contraction computeContractionForHypernode(const HypernodeID hn) {
    HypernodeID uf_target = _uf.find(_target[hn]);
    ASSERT( _uf.find(hn) == hn, "Hypernode " << hn << " must be the representative of a contraction set");
    // By the nature how we link two nodes in the union find data structure, we can
    // show that, if hn == uf_target, than hn must be visited before, which is not 
    // possible
    ASSERT( hn != uf_target, "Cannot contract hypernode " << hn << " on itself" );
    ASSERT( _hg.nodeIsEnabled(uf_target), "Contraction partner " << uf_target << " must be an active node" );
    
    // Representive node for contraction is the representive in the contraction set
    // of the union find data structure after linking both vertices
    HypernodeID rep_node = _uf.link(hn, uf_target);
    if ( rep_node == uf_target ) {
      uf_target = hn;
    }
    return Contraction(rep_node, uf_target);
  } 

  inline void computeContractionTargetForAllHypernodes( std::vector<HypernodeID>& current_hypernodes ) {
    for (const HypernodeID& hn : _hg.nodes()) {
      current_hypernodes.push_back(hn);
      Rating rating = _rater.rate(hn);
      if ( rating.valid ) {
        _target[ hn ] = rating.target;
      } else {
        _target[ hn ] = kInvalidTarget;
      }
    }
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_context;
  using Base::_history;
  using Base::_coarsening_levels;
  Rater _rater;
  ds::UnionFind _uf;
  std::vector<HypernodeID> _target;
  bool _enableRandomization;
};
}  // namespace kahypar
