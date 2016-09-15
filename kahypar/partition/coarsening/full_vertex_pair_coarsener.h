/***************************************************************************
 *  Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "datastructure/fast_reset_vector.h"
#include "definitions.h"
#include "meta/mandatory.h"
#include "meta/template_parameter_to_string.h"
#include "partition/coarsening/i_coarsener.h"
#include "partition/coarsening/vertex_pair_coarsener_base.h"
#include "utils/stats.h"

using utils::Stats;
using datastructure::FastResetFlagVector;

namespace partition {
template <class Rater = Mandatory>
class FullVertexPairCoarsener final : public ICoarsener,
                                      private VertexPairCoarsenerBase<>{
 private:
  using Base = VertexPairCoarsenerBase;
  using Rating = typename Rater::Rating;

 public:
  FullVertexPairCoarsener(Hypergraph& hypergraph, const Configuration& config,
                          const HypernodeWeight weight_of_heaviest_node) :
    VertexPairCoarsenerBase(hypergraph, config, weight_of_heaviest_node),
    _rater(_hg, _config),
    _target(hypergraph.initialNumNodes()) { }

  virtual ~FullVertexPairCoarsener() { }

  FullVertexPairCoarsener(const FullVertexPairCoarsener&) = delete;
  FullVertexPairCoarsener& operator= (const FullVertexPairCoarsener&) = delete;

  FullVertexPairCoarsener(FullVertexPairCoarsener&&) = delete;
  FullVertexPairCoarsener& operator= (FullVertexPairCoarsener&&) = delete;

 private:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void coarsenImpl(const HypernodeID limit) override final {
    _pq.clear();

    rateAllHypernodes(_rater, _target);

    FastResetFlagVector<> rerated_hypernodes(_hg.initialNumNodes());
    // Used to prevent unnecessary re-rating of hypernodes that have been removed from
    // PQ because they are heavier than allowed.
    FastResetFlagVector<> invalid_hypernodes(_hg.initialNumNodes());

    while (!_pq.empty() && _hg.currentNumNodes() > limit) {
      const HypernodeID rep_node = _pq.top();
      const HypernodeID contracted_node = _target[rep_node];
      DBG(dbg_coarsening_coarsen, "Contracting: (" << rep_node << ","
          << _target[rep_node] << ") prio: " << _pq.topKey() <<
          " deg(" << rep_node << ")=" << _hg.nodeDegree(rep_node) <<
          " w(" << rep_node << ")=" << _hg.nodeWeight(rep_node) <<
          " deg(" << contracted_node << ")=" << _hg.nodeDegree(contracted_node) <<
          " w(" << contracted_node << ")=" << _hg.nodeWeight(contracted_node));


      ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(_target[rep_node])
             <= _rater.thresholdNodeWeight(),
             "Trying to contract nodes violating maximum node weight");
      ASSERT(_pq.topKey() == _rater.rate(rep_node).value,
             "Key in PQ != rating calculated by rater:" << _pq.topKey() << "!="
             << _rater.rate(rep_node).value);
      ASSERT(!invalid_hypernodes[rep_node], "Representative HN " << rep_node << " is invalid");
      ASSERT(!invalid_hypernodes[contracted_node], "Contract HN " << contracted_node << " is invalid");

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

  std::string policyStringImpl() const override final {
    return std::string(" ratingFunction=" + templateToString<Rater>());
  }


  void reRateAffectedHypernodes(const HypernodeID rep_node,
                                FastResetFlagVector<>& rerated_hypernodes,
                                FastResetFlagVector<>& invalid_hypernodes) {
    for (const HyperedgeID he : _hg.incidentEdges(rep_node)) {
      for (const HypernodeID pin : _hg.pins(he)) {
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
                                    FastResetFlagVector<>& invalid_hypernodes) {
    if (rating.valid) {
      ASSERT(_pq.contains(hn),
             "Trying to update rating of HN " << hn << " which is not in PQ");
      DBG(false, "Updating prio of HN " << hn << ": " << _pq.getKey(hn) << " (target=" << _target[hn]
          << ") --- >" << rating.value << "(target" << rating.target << ")");
      _pq.updateKey(hn, rating.value);
      _target[hn] = rating.target;
    } else if (_pq.contains(hn)) {
      // explicit containment check is necessary because of V-cycles. In this case, not
      // all hypernodes will be inserted into the PQ at the beginning, because of the
      // restriction that only hypernodes within the same part can be contracted.
      _pq.remove(hn);
      invalid_hypernodes.set(hn, true);
      _target[hn] = std::numeric_limits<HypernodeID>::max();
      DBG(dbg_coarsening_no_valid_contraction, "Progress [" << _hg.currentNumNodes() << "/"
          << _hg.initialNumNodes() << "]:HN " << hn
          << " \t(w=" << _hg.nodeWeight(hn) << "," << " deg=" << _hg.nodeDegree(hn)
          << ") did not find valid contraction partner.");
#ifdef GATHER_STATS
      Stats::instance().add(_config, "numHNsWithoutValidContractionPartner", 1);
#endif
    }
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_config;
  using Base::_history;
  using Base::_hypergraph_pruner;
  Rater _rater;
  std::vector<HypernodeID> _target;
};
}  // namespace partition
