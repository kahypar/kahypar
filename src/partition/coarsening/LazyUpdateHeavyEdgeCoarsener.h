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
  typedef HeavyEdgeCoarsenerBase<Rater> Base;
  typedef typename Rater::Rating Rating;

  class NullMap {
    public:
    void insert(std::pair<HypernodeID, HypernodeID>) { }
  };

  public:
  using Base::_pq;
  using Base::_hg;
  using Base::_rater;
  using Base::_history;
  using Base::_stats;
  using Base::rateAllHypernodes;
  using Base::performContraction;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;
  using Base::gatherCoarseningStats;

  LazyUpdateHeavyEdgeCoarsener(Hypergraph& hypergraph, const Configuration& config) :
    Base(hypergraph, config),
    _outdated_rating(hypergraph.initialNumNodes(), false),
    _target(_hg.initialNumNodes())
  { }

  ~LazyUpdateHeavyEdgeCoarsener() { }

  private:
  FRIEND_TEST(ALazyUpdateCoarsener, InvalidatesAdjacentHypernodesInsteadOfReratingThem);

  void coarsenImpl(int limit) final {
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
        _pq.remove(contracted_node);

        removeSingleNodeHyperedges(rep_node);
        removeParallelHyperedges(rep_node);

        // this also invalidates rep_node, however rep_node
        // will be re-rated and updated afterwards
        invalidateAffectedHypernodes(rep_node);

        updatePQandContractionTarget(rep_node, _rater.rate(rep_node));
      }
    }
    gatherCoarseningStats();
  }

  bool uncoarsenImpl(IRefiner& refiner) final {
    return Base::doUncoarsen(refiner);
  }

  const Stats & statsImpl() const {
    return _stats;
  }

  std::string policyStringImpl() const final {
    return std::string(" ratingFunction=" + templateToString<Rater>());
  }

  void invalidateAffectedHypernodes(HypernodeID rep_node) {
    for (const HyperedgeID he : _hg.incidentEdges(rep_node)) {
      for (const HypernodeID pin : _hg.pins(he)) {
        _outdated_rating[pin] = true;
      }
    }
  }

  void updatePQandContractionTarget(HypernodeID hn, const Rating& rating) {
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
    }
  }

  std::vector<bool> _outdated_rating;
  std::vector<HypernodeID> _target;
};
}              // namespace partition


#endif  // SRC_PARTITION_COARSENING_LAZYUPDATEHEAVYEDGECOARSENER_H_
