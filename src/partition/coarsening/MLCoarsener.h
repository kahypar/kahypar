/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_MLCOARSENER_H_
#define SRC_PARTITION_COARSENING_MLCOARSENER_H_

#include <limits>
#include <string>
#include <vector>

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/SparseMap.h"
#include "lib/definitions.h"
#include "lib/macros.h"
#include "partition/coarsening/RatingTieBreakingPolicies.h"

using datastructure::FastResetBitVector;

namespace partition {
using defs::RatingType;

template <class Rater = Mandatory>
class MLCoarsener final : public ICoarsener,
                          private HeavyEdgeCoarsenerBase<Rater>{
 private:
  static const bool dbg_partition_rating = false;

  using Base = HeavyEdgeCoarsenerBase<Rater>;
  using Base::performContraction;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;

  static constexpr HypernodeID kInvalidTarget = std::numeric_limits<HypernodeID>::max();
  static constexpr RatingType kInvalidScore = std::numeric_limits<RatingType>::min();

  struct Rating {
    Rating(HypernodeID trgt, RatingType val, bool is_valid) noexcept :
      target(trgt),
      value(val),
      valid(is_valid) { }

    Rating() :
      target(kInvalidTarget),
      value(kInvalidScore),
      valid(false) { }

    Rating(const Rating&) = delete;
    Rating& operator= (const Rating&) = delete;

    Rating(Rating&&) = default;
    Rating& operator= (Rating&&) = delete;

    HypernodeID target;
    RatingType value;
    bool valid;
  };

 public:
  MLCoarsener(Hypergraph& hypergraph, const Configuration& config,
              const HypernodeWeight weight_of_heaviest_node) noexcept :
    Base(hypergraph, config, weight_of_heaviest_node),
    _tmp_ratings(_hg.initialNumNodes()) { }

  // TODO(schlag): i.e. min degree ordering

  virtual ~MLCoarsener() { }

  MLCoarsener(const MLCoarsener&) = delete;
  MLCoarsener& operator= (const MLCoarsener&) = delete;

  MLCoarsener(MLCoarsener&&) = delete;
  MLCoarsener& operator= (MLCoarsener&&) = delete;

 private:
  void coarsenImpl(const HypernodeID limit) noexcept override final {
    int pass_nr = 0;
    std::vector<HypernodeID> current_hns;
    FastResetBitVector<> already_matched(_hg.initialNumNodes(), false);
    while (_hg.currentNumNodes() > limit) {
      LOGVAR(pass_nr);
      LOGVAR(_hg.currentNumNodes());

      already_matched.resetAllBitsToFalse();
      current_hns.clear();

      const HypernodeID num_hns_before_pass = _hg.currentNumNodes();
      for (const HypernodeID hn : _hg.nodes()) {
        current_hns.push_back(hn);
      }
      Randomize::shuffleVector(current_hns, current_hns.size());
      // std::sort(std::begin(current_hns), std::end(current_hns),
      //           [&](const HypernodeID l, const HypernodeID r) {
      //             return _hg.nodeDegree(l) < _hg.nodeDegree(r);
      //           });

      for (const HypernodeID hn : current_hns) {
        if (_hg.nodeIsEnabled(hn)) {
          const Rating rating = contractionPartner(hn, already_matched);

          if (rating.target != kInvalidTarget) {
            already_matched.setBit(hn, true);
            already_matched.setBit(rating.target, true);
            performContraction(hn, rating.target);
            removeSingleNodeHyperedges(hn);
            removeParallelHyperedges(hn);
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
  }

  Rating contractionPartner(const HypernodeID u, const FastResetBitVector<>& already_matched) {
    DBG(dbg_partition_rating, "Calculating rating for HN " << u);
    const HypernodeWeight weight_u = _hg.nodeWeight(u);
    const PartitionID part_u = _hg.partID(u);
    for (const HyperedgeID he : _hg.incidentEdges(u)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      const RatingType score = static_cast<RatingType>(_hg.edgeWeight(he)) / (_hg.edgeSize(he) - 1);
      if (_hg.edgeSize(he) <= _config.partition.hyperedge_size_threshold) {
        for (const HypernodeID v : _hg.pins(he)) {
          if (v != u &&
              belowThresholdNodeWeight(weight_u, _hg.nodeWeight(v)) &&
              (part_u == _hg.partID(v))) {
            _tmp_ratings[v] += score;
          }
        }
      }
    }

    RatingType max_rating = std::numeric_limits<RatingType>::min();
    HypernodeID target = std::numeric_limits<HypernodeID>::max();
    for (auto it = _tmp_ratings.crbegin(); it != _tmp_ratings.crend(); ++it) {
      const HypernodeID tmp_target = it->key;
      const RatingType tmp = it->value;  //  /
      // (weight_u * _hg.nodeWeight(tmp_target));
      DBG(false, "r(" << u << "," << tmp_target << ")=" << tmp);
      if (acceptRating(tmp, max_rating, target, tmp_target, already_matched)) {
        max_rating = tmp;
        target = tmp_target;
      }
    }
    _tmp_ratings.clear();
    Rating ret;
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

  bool uncoarsenImpl(IRefiner& refiner) noexcept override final {
    return Base::doUncoarsen(refiner);
  }

  std::string policyStringImpl() const noexcept override final {
    return std::string(" ratingFunction=" + templateToString<Rater>());
  }

  bool belowThresholdNodeWeight(const HypernodeWeight weight_u,
                                const HypernodeWeight weight_v) const noexcept {
    return weight_v + weight_u <= _config.coarsening.max_allowed_node_weight;
  }

  bool acceptRating(const RatingType tmp, const RatingType max_rating,
                    const HypernodeID old_target, const HypernodeID new_target,
                    const FastResetBitVector<>& already_matched) const noexcept {
    return max_rating < tmp ||
           ((max_rating == tmp) &&
            ((already_matched[old_target] && !already_matched[new_target]) ||
             (already_matched[old_target] && already_matched[new_target] &&
              RandomRatingWins::acceptEqual()) ||
             (!already_matched[old_target] && !already_matched[new_target] &&
              RandomRatingWins::acceptEqual())
            ));
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_config;
  using Base::_rater;
  using Base::_history;
  SparseMap<HypernodeID, RatingType> _tmp_ratings;
};
}  // namespace partition

#endif  // SRC_PARTITION_COARSENING_MLCOARSENER_H_
