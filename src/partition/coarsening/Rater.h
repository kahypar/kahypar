/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_RATER_H_
#define SRC_PARTITION_COARSENING_RATER_H_

#include <limits>
#include <stack>
#include <vector>

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/definitions.h"
#include "lib/macros.h"
#include "partition/Configuration.h"
#include "partition/coarsening/RatingTieBreakingPolicies.h"

using datastructure::FastResetBitVector;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;

namespace partition {
static const bool dbg_partition_rating = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
// See Modern C++ Design for the reason why _TiebreakingPolicy has protected non-virtual destructor
template <typename _RatingType, class _TieBreakingPolicy>
class Rater {
 public:
  using RatingType = _RatingType;

 private:
  using TieBreakingPolicy = _TieBreakingPolicy;

  struct HeavyEdgeRating {
    HeavyEdgeRating(HypernodeID trgt, RatingType val, bool is_valid) noexcept :
      target(trgt),
      value(val),
      valid(is_valid) { }

    HeavyEdgeRating() :
      target(std::numeric_limits<HypernodeID>::max()),
      value(std::numeric_limits<RatingType>::min()),
      valid(false) { }

    HeavyEdgeRating(const HeavyEdgeRating&) = delete;
    HeavyEdgeRating& operator= (const HeavyEdgeRating&) = delete;

    HeavyEdgeRating(HeavyEdgeRating&&) = default;
    HeavyEdgeRating& operator= (HeavyEdgeRating&&) = delete;

    HypernodeID target;
    RatingType value;
    bool valid;
  };

 public:
  using Rating = HeavyEdgeRating;

  Rater(Hypergraph& hypergraph, const Configuration& config) noexcept :
    _hg(hypergraph),
    _config(config),
    _tmp_ratings(_hg.initialNumNodes()),
    _used_entries(),
    _visited_hypernodes(_hg.initialNumNodes(), false) {
    _used_entries.reserve(_hg.initialNumNodes());
  }

  Rater(const Rater&) = delete;
  Rater& operator= (const Rater&) = delete;

  Rater(Rater&&) = delete;
  Rater& operator= (Rater&&) = delete;

  HeavyEdgeRating rate(const HypernodeID u) noexcept {
    ASSERT(_used_entries.empty(), "Stack is not empty");
    ASSERT([&]() {
        for (HypernodeID hn = 0; hn < _hg.initialNumNodes(); ++hn) {
          if (_visited_hypernodes[hn]) {
            return false;
          }
        }
        return true;
      } (), "Bitset not empty");
    DBG(dbg_partition_rating, "Calculating rating for HN " << u);
    const HypernodeWeight weight_u = _hg.nodeWeight(u);
    const PartitionID part_u = _hg.partID(u);
    for (const HyperedgeID he : _hg.incidentEdges(u)) {
      const RatingType score = static_cast<RatingType>(_hg.edgeWeight(he)) / (_hg.edgeSize(he) - 1);
      for (const HypernodeID v : _hg.pins(he)) {
        if (v != u &&
            belowThresholdNodeWeight(weight_u, _hg.nodeWeight(v)) &&
            (part_u == _hg.partID(v))) {
          _tmp_ratings[v] += score;
          if (!_visited_hypernodes[v]) {
            _visited_hypernodes.setBit(v, true);
            _used_entries.push_back(v);
          }
        }
      }
    }

    RatingType max_rating = std::numeric_limits<RatingType>::min();
    HypernodeID target = std::numeric_limits<HypernodeID>::max();
    while (!_used_entries.empty()) {
      const HypernodeID tmp_target = _used_entries.back();
      _used_entries.pop_back();
      const RatingType tmp = _tmp_ratings[tmp_target] /
                             (weight_u * _hg.nodeWeight(tmp_target));
      _tmp_ratings[tmp_target] = 0.0;
      DBG(false, "r(" << u << "," << tmp_target << ")=" << tmp);
      if (acceptRating(tmp, max_rating)) {
        max_rating = tmp;
        target = tmp_target;
      }
      _visited_hypernodes.setBit(tmp_target, false);
    }
    HeavyEdgeRating ret;
    if (max_rating != std::numeric_limits<RatingType>::min()) {
      ASSERT(target != std::numeric_limits<HypernodeID>::max(), "invalid contraction target");
      ret.value = max_rating;
      ret.target = target;
      ret.valid = true;
    }
    ASSERT([&]() {
        bool flag = true;
        if (ret.valid && (_hg.partID(u) != _hg.partID(ret.target))) {
          flag = false;
        }
        return flag;
      } (), "Representative " << u << " & contraction target " << ret.target
           << " are in different parts!");
    DBG(dbg_partition_rating, "rating=(" << ret.value << "," << ret.target << ","
        << ret.valid << ")");
    return ret;
  }

  HypernodeWeight thresholdNodeWeight() const noexcept {
    return _config.coarsening.max_allowed_node_weight;
  }

 private:
  bool belowThresholdNodeWeight(const HypernodeWeight weight_u,
                                const HypernodeWeight weight_v) const noexcept {
    return weight_v + weight_u <= _config.coarsening.max_allowed_node_weight;
  }

  bool acceptRating(const RatingType tmp, const RatingType max_rating) const noexcept {
    return max_rating < tmp || (max_rating == tmp && TieBreakingPolicy::acceptEqual());
  }

  Hypergraph& _hg;
  const Configuration& _config;
  std::vector<RatingType> _tmp_ratings;
  std::vector<HypernodeID> _used_entries;
  FastResetBitVector<> _visited_hypernodes;
};
#pragma GCC diagnostic pop
}  // namespace partition

#endif  // SRC_PARTITION_COARSENING_RATER_H_
