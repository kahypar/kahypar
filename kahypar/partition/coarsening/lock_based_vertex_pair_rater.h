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

#include <algorithm>
#include <limits>
#include <stack>
#include <vector>
#include <shared_mutex>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_map.h"
#include "kahypar/datastructure/mutex_vector.h"
#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_partition_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "kahypar/partition/context.h"

namespace kahypar {
template <class ScorePolicy = HeavyEdgeScore,
          class HeavyNodePenaltyPolicy = MultiplicativePenalty,
          class CommunityPolicy = UseCommunityStructure,
          class RatingPartitionPolicy = NormalPartitionPolicy,
          class AcceptancePolicy = BestRatingWithTieBreaking<>,
          class FixedVertexPolicy = AllowFreeOnFixedFreeOnFreeFixedOnFixed,
          typename RatingType = RatingType,
          typename HypergraphT = Hypergraph>
class LockBasedVertexPairRater {
 private:
  static constexpr bool debug = false;

  static constexpr size_t MAP_SIZE = 10000;

  class VertexPairRating {
 public:
    VertexPairRating(HypernodeID trgt, RatingType val, bool is_valid) :
      target(trgt),
      value(val),
      valid(is_valid) { }

    VertexPairRating() :
      target(std::numeric_limits<HypernodeID>::max()),
      value(std::numeric_limits<RatingType>::min()),
      valid(false) { }

    VertexPairRating(const VertexPairRating&) = delete;
    VertexPairRating& operator= (const VertexPairRating&) = delete;

    VertexPairRating(VertexPairRating&&) = default;
    VertexPairRating& operator= (VertexPairRating&&) = delete;

    ~VertexPairRating() = default;

    HypernodeID target;
    RatingType value;
    bool valid;
  };

 public:
  using Rating = VertexPairRating;
  using Mutex = std::shared_timed_mutex;
  template<typename Key>
  using MutexVector = kahypar::ds::MutexVector<Key, Mutex>;

  LockBasedVertexPairRater(HypergraphT& hypergraph, 
                           const Context& context,
                           MutexVector<HypernodeID>& hn_mutex,
                           MutexVector<HyperedgeID>& he_mutex ) :
    _hg(hypergraph),
    _context(context),
    _community_id(-1),
    _tmp_ratings(hypergraph.initialNumNodes()),
    _hn_mutex(hn_mutex),
    _he_mutex(he_mutex) { }

  LockBasedVertexPairRater(const LockBasedVertexPairRater&) = delete;
  LockBasedVertexPairRater& operator= (const LockBasedVertexPairRater&) = delete;

  LockBasedVertexPairRater(LockBasedVertexPairRater&&) = delete;
  LockBasedVertexPairRater& operator= (LockBasedVertexPairRater&&) = delete;

  ~LockBasedVertexPairRater() = default;

  void setCommunity(const PartitionID community_id) {
    _community_id = community_id;
  }

  VertexPairRating rateSingleThreaded(const HypernodeID u, kahypar::ds::FastResetFlagArray<>& already_matched) {
    DBG << "Calculating rating for HN" << u;
    HypernodeWeight weight_u = 0;
    ASSERT(_hg.nodeIsEnabled(u), "Hypernode" << u << "is disabled");
    weight_u = _hg.nodeWeight(u);
    for (const HyperedgeID& he : _hg.incidentEdges(u)) {
      ASSERT(_hg.edgeIsEnabled(he, _community_id), "Hyperedge" << he << "is disabled");
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
        const RatingType score = ScorePolicy::score(_hg, he, _context, _community_id); 
        for (const HypernodeID& v : _hg.pins(he, _community_id)) {
          if (v != u && belowThresholdNodeWeight(weight_u, _hg.nodeWeight(v)) ) {
            _tmp_ratings[v] += score;
          }
        }
      }
    }

    RatingType max_rating = std::numeric_limits<RatingType>::min();
    HypernodeID target = std::numeric_limits<HypernodeID>::max();
    HypernodeID community_target = std::numeric_limits<HypernodeID>::max();
    for (auto it = _tmp_ratings.end() - 1; it >= _tmp_ratings.begin(); --it) {
      const HypernodeID tmp_target = it->key;
      ASSERT(_hg.nodeIsEnabled(u), "Hypernode" << u << "is disabled");
      ASSERT(_hg.nodeIsEnabled(tmp_target), "Hypernode" << tmp_target << "is disabled");
      const HypernodeWeight target_weight = _hg.nodeWeight(tmp_target);
      HypernodeWeight penalty = HeavyNodePenaltyPolicy::penalty(weight_u,
                                                                target_weight);
      penalty = penalty == 0 ? std::max(std::max(weight_u, target_weight), 1) : penalty;
      const RatingType tmp_rating = it->value / static_cast<double>(penalty);
      if (CommunityPolicy::sameCommunity(_hg.communities(), u, tmp_target) &&
          AcceptancePolicy::acceptRating(tmp_rating, max_rating,
                                          community_target, 
                                          _hg.communityNodeID(tmp_target),
                                          already_matched) &&
          RatingPartitionPolicy::accept(_hg, _context, u, tmp_target) /*&&
          FixedVertexPolicy::acceptContraction(_hg, _context, u, tmp_target)*/) {
        max_rating = tmp_rating;
        target = tmp_target;
        community_target = _hg.communityNodeID(tmp_target);
      }
      // _tmp_ratings.remove_last_entry();
    }

    VertexPairRating ret;
    if (max_rating != std::numeric_limits<RatingType>::min()) {
      ASSERT(target != std::numeric_limits<HypernodeID>::max(), "invalid contraction target");
      ret.value = max_rating;
      ret.target = target;
      ret.valid = true;
      ASSERT(_hg.communities()[u] == _hg.communities()[ret.target]);
    }
    _tmp_ratings.clear();
    DBG << "rating=(" << ret.value << "," << ret.target << "," << ret.valid << ")";
    return ret;
  }

  VertexPairRating rateLockBased(const HypernodeID u, kahypar::ds::FastResetFlagArray<>& already_matched) {
    DBG << "Calculating rating for HN" << u;
    std::shared_lock<Mutex> hn_lock(_hn_mutex[u]);
    HypernodeWeight weight_u = 0;
    if ( _hg.nodeIsEnabled(u) ) {
      weight_u = _hg.nodeWeight(u);
      for (const HyperedgeID& he : _hg.incidentEdges(u)) {
        std::shared_lock<Mutex> he_lock(_he_mutex[he]);
        if ( _hg.edgeIsEnabled(he, _community_id) ) {
          ASSERT(_hg.edgeSize(he) > 1, V(he));
          if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
            const RatingType score = ScorePolicy::score(_hg, he, _context, _community_id); 
            for (const HypernodeID& v : _hg.pins(he, _community_id)) {
              if (v != u && belowThresholdNodeWeight(weight_u, _hg.nodeWeight(v)) ) {
                _tmp_ratings[v] += score;
              }
            }
          }
        }
      }
    }

    RatingType max_rating = std::numeric_limits<RatingType>::min();
    HypernodeID target = std::numeric_limits<HypernodeID>::max();
    for (auto it = _tmp_ratings.end() - 1; it >= _tmp_ratings.begin(); --it) {
      const HypernodeID tmp_target = it->key;
      if ( u > tmp_target ) {
        hn_lock.unlock();
      }
      std::shared_lock<Mutex> target_lock(_hn_mutex[tmp_target]);
      if ( u > tmp_target ) {
        hn_lock.lock();
      }
      if ( _hg.nodeIsEnabled(u) && _hg.nodeIsEnabled(tmp_target) ) {
        const HypernodeWeight target_weight = _hg.nodeWeight(tmp_target);
        HypernodeWeight penalty = HeavyNodePenaltyPolicy::penalty(weight_u,
                                                                  target_weight);
        penalty = penalty == 0 ? std::max(std::max(weight_u, target_weight), 1) : penalty;
        const RatingType tmp_rating = it->value / static_cast<double>(penalty);
        DBG << "r(" << u << "," << tmp_target << ")=" << tmp_rating;
        if (CommunityPolicy::sameCommunity(_hg.communities(), u, tmp_target) &&
            AcceptancePolicy::acceptRating(tmp_rating, max_rating,
                                          _hg.communityNodeID(target), 
                                          _hg.communityNodeID(tmp_target),
                                          already_matched) &&
            RatingPartitionPolicy::accept(_hg, _context, u, tmp_target) /*&&
            FixedVertexPolicy::acceptContraction(_hg, _context, u, tmp_target)*/) {
          max_rating = tmp_rating;
          target = tmp_target;
        }
        // _tmp_ratings.remove_last_entry();
      } else if ( !_hg.nodeIsEnabled(u) ) {
        max_rating = std::numeric_limits<RatingType>::min();
        target = std::numeric_limits<HypernodeID>::max();
        // _tmp_ratings.remove_last_entry();
        break;
      }
    }
    hn_lock.unlock();

    VertexPairRating ret;
    if (max_rating != std::numeric_limits<RatingType>::min()) {
      ASSERT(target != std::numeric_limits<HypernodeID>::max(), "invalid contraction target");
      ret.value = max_rating;
      ret.target = target;
      ret.valid = true;
      ASSERT(_hg.communities()[u] == _hg.communities()[ret.target]);
    }
    _tmp_ratings.clear();
    DBG << "rating=(" << ret.value << "," << ret.target << "," << ret.valid << ")";
    return ret;
  }

  HypernodeWeight thresholdNodeWeight() const {
    return _context.coarsening.max_allowed_node_weight;
  }

 private:

  bool belowThresholdNodeWeight(const HypernodeWeight weight_u,
                                const HypernodeWeight weight_v) const {
    return weight_v + weight_u <= _context.coarsening.max_allowed_node_weight;
  }

  HypergraphT& _hg;
  const Context& _context;
  PartitionID _community_id;
  ds::SparseMap<HypernodeID, RatingType> _tmp_ratings;
  MutexVector<HypernodeID>& _hn_mutex;
  MutexVector<HyperedgeID>& _he_mutex;  
};
}  // namespace kahypar
