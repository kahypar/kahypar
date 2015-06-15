/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_LAZYUPDATEHEAVYEDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_LAZYUPDATEHEAVYEDGECOARSENER_H_

#include <string>
#include <utility>
#include <vector>

#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/coarsening/HeavyEdgeCoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"

using datastructure::PriorityQueue;
using datastructure::MetaKeyDouble;
using defs::Hypergraph;
using defs::HypernodeID;
using utils::Stats;

namespace partition {
template <class Rater = Mandatory>
class LazyUpdateHeavyEdgeCoarsener : public ICoarsener,
                                     private HeavyEdgeCoarsenerBase<Rater>{
 private:
  using Base = HeavyEdgeCoarsenerBase<Rater>;
  using Base::rateAllHypernodes;
  using Base::performContraction;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;
  using Rating = typename Rater::Rating;

  class NullMap {
 public:
    void insert(std::pair<HypernodeID, HypernodeID>) { }
  };

 public:
  LazyUpdateHeavyEdgeCoarsener(const LazyUpdateHeavyEdgeCoarsener&) = delete;
  LazyUpdateHeavyEdgeCoarsener(LazyUpdateHeavyEdgeCoarsener&&) = delete;
  LazyUpdateHeavyEdgeCoarsener& operator= (const LazyUpdateHeavyEdgeCoarsener&) = delete;
  LazyUpdateHeavyEdgeCoarsener& operator= (LazyUpdateHeavyEdgeCoarsener&&) = delete;

  LazyUpdateHeavyEdgeCoarsener(Hypergraph& hypergraph, const Configuration& config,
                               const HypernodeWeight weight_of_heaviest_node) noexcept :
    Base(hypergraph, config, weight_of_heaviest_node),
    _outdated_rating(hypergraph.initialNumNodes(), false),
    _target(_hg.initialNumNodes())
  { }

  ~LazyUpdateHeavyEdgeCoarsener() { }

 private:
  FRIEND_TEST(ALazyUpdateCoarsener, InvalidatesAdjacentHypernodesInsteadOfReratingThem);

  void coarsenImpl(const HypernodeID limit) noexcept final {
    _pq.clear();

    NullMap null_map;
    rateAllHypernodes(_target, null_map);

    while (!_pq.empty() && _hg.numNodes() > limit) {
      const HypernodeID rep_node = _pq.max();

      if (_outdated_rating[rep_node]) {
        DBG(dbg_coarsening_coarsen,
            "Rating for HN" << rep_node << " is invalid: " << _pq.maxKey() << "--->"
            << _rater.rate(rep_node).value << " target=" << _rater.rate(rep_node).target
            << " valid= " << _rater.rate(rep_node).valid);
        updatePQandContractionTarget(rep_node, _rater.rate(rep_node));
      } else {
        const HypernodeID contracted_node = _target[rep_node];

        DBG(dbg_coarsening_coarsen, "Contracting: (" << rep_node << ","
            << _target[rep_node] << ") prio: " << _pq.maxKey());
        ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(_target[rep_node])
               <= _rater.thresholdNodeWeight(),
               "Trying to contract nodes violating maximum node weight");
        ASSERT(_pq.maxKey() == _rater.rate(rep_node).value,
               "Key in PQ != rating calculated by rater:" << _pq.maxKey() << "!="
               << _rater.rate(rep_node).value);

        performContraction(rep_node, contracted_node);
        ASSERT(_pq.contains(contracted_node), V(contracted_node));
        _pq.remove(contracted_node);

        removeSingleNodeHyperedges(rep_node);
        removeParallelHyperedges(rep_node);

        // this also invalidates rep_node, however rep_node
        // will be re-rated and updated afterwards
        invalidateAffectedHypernodes(rep_node);

        updatePQandContractionTarget(rep_node, _rater.rate(rep_node));
      }
    }
  }

  bool uncoarsenImpl(IRefiner& refiner) noexcept final {
    return Base::doUncoarsen(refiner);
  }

  std::string policyStringImpl() const noexcept final {
    return std::string(" ratingFunction=" + templateToString<Rater>());
  }

  void invalidateAffectedHypernodes(const HypernodeID rep_node) noexcept {
    for (const HyperedgeID he : _hg.incidentEdges(rep_node)) {
      for (const HypernodeID pin : _hg.pins(he)) {
        _outdated_rating[pin] = true;
      }
    }
  }

  void updatePQandContractionTarget(const HypernodeID hn, const Rating& rating) noexcept {
    _outdated_rating[hn] = false;
    if (rating.valid) {
      ASSERT(_pq.contains(hn),
             "Trying to update rating of HN " << hn << " which is not in PQ");
      _pq.updateKey(hn, rating.value);
      _target[hn] = rating.target;
    } else {
      // In this case, no explicit contaiment check is necessary because the
      // method is only called on rep_node, which is definetly in the PQ.
      ASSERT(_pq.contains(hn),
             "Trying to remove rating of HN " << hn << " which is not in PQ");
      _pq.remove(hn);
      DBG(dbg_coarsening_no_valid_contraction, "Progress [" << _hg.numNodes() << "/"
          << _hg.initialNumNodes() << "]:HN " << hn
          << " \t(w=" << _hg.nodeWeight(hn) << "," << " deg=" << _hg.nodeDegree(hn)
          << ") did not find valid contraction partner.");
      Stats::instance().add("numHNsWithoutValidContractionPartner", _config.partition.current_v_cycle, 1);
    }
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_config;
  using Base::_rater;
  using Base::_history;
  std::vector<bool> _outdated_rating;
  std::vector<HypernodeID> _target;
};
}              // namespace partition


#endif  // SRC_PARTITION_COARSENING_LAZYUPDATEHEAVYEDGECOARSENER_H_
