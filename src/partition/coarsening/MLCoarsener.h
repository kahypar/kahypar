/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

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

  virtual ~MLCoarsener() { }

  MLCoarsener(const MLCoarsener&) = delete;
  MLCoarsener& operator= (const MLCoarsener&) = delete;

  MLCoarsener(MLCoarsener&&) = delete;
  MLCoarsener& operator= (MLCoarsener&&) = delete;

 private:
  void coarsenImpl(const HypernodeID limit) noexcept override final {
    int pass_nr = 0;
    std::vector<HypernodeID> current_hns;
    FastResetBitVector<> already_matched(_hg.initialNumNodes());
    while (_hg.currentNumNodes() > limit) {
      LOGVAR(pass_nr);
      LOGVAR(_hg.currentNumNodes());

      already_matched.reset();
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
            already_matched.set(hn, true);
            already_matched.set(rating.target, true);
            // if (_hg.nodeDegree(hn) > _hg.nodeDegree(rating.target)) {
            performContraction(hn, rating.target);
            // } else {
            //   contract(rating.target, hn);
            // }
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
    for (const HyperedgeID he : _hg.incidentEdges(u)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      if (_hg.edgeSize(he) <= _config.partition.hyperedge_size_threshold) {
        const RatingType score = static_cast<RatingType>(_hg.edgeWeight(he)) / (_hg.edgeSize(he) - 1);
        for (const HypernodeID v : _hg.pins(he)) {
          if ((v != u && belowThresholdNodeWeight(weight_u, _hg.nodeWeight(v)))) {
            _tmp_ratings[v] += score;
          }
        }
      }
    }
    if (_tmp_ratings.contains(u)) {
      _tmp_ratings.remove(u);
    }

    RatingType max_rating = std::numeric_limits<RatingType>::min();
    HypernodeID target = std::numeric_limits<HypernodeID>::max();
    for (auto it = _tmp_ratings.crbegin(); it != _tmp_ratings.crend(); ++it) {
      const HypernodeID tmp_target = it->key;
      const RatingType tmp_rating = it->value;
      DBG(false, "r(" << u << "," << tmp_target << ")=" << tmp_rating);
      if (acceptRating(tmp_rating, max_rating, target, tmp_target, already_matched)) {
        max_rating = tmp_rating;
        target = tmp_target;
      }
    }

    Rating ret;
    if (max_rating != std::numeric_limits<RatingType>::min()) {
      ASSERT(target != std::numeric_limits<HypernodeID>::max(), "invalid contraction target");
      ASSERT(_tmp_ratings[target] == max_rating, V(target));
      ret.value = max_rating;
      ret.target = target;
      ret.valid = true;
    }

    _tmp_ratings.clear();

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

  using Base::_pq;
  using Base::_hg;
  using Base::_config;
  using Base::_rater;
  using Base::_history;
  SparseMap<HypernodeID, RatingType> _tmp_ratings;
};
}  // namespace partition
