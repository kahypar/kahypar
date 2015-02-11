/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/coarsening/HeavyEdgeCoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"

using defs::Hypergraph;
using defs::HypernodeID;
using utils::Stats;

namespace partition {
template <class Rater = Mandatory>
class FullHeavyEdgeCoarsener : public ICoarsener,
                               private HeavyEdgeCoarsenerBase<Rater>{
  private:
  typedef HeavyEdgeCoarsenerBase<Rater> Base;
  typedef typename Rater::Rating Rating;

  class NullMap {
    public:
    void insert(std::pair<HypernodeID, HypernodeID>) { }
  };

  public:
  FullHeavyEdgeCoarsener(Hypergraph& hypergraph, const Configuration& config) :
    HeavyEdgeCoarsenerBase<Rater>(hypergraph, config),
    _target(hypergraph.initialNumNodes()) { }

  ~FullHeavyEdgeCoarsener() { }

  private:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  using Base::removeParallelHyperedges;
  using Base::removeSingleNodeHyperedges;
  using Base::rateAllHypernodes;
  using Base::performContraction;
  using Base::gatherCoarseningStats;

  void coarsenImpl(const HypernodeID limit) final {
    _pq.clear();

    NullMap null_map;
    rateAllHypernodes(_target, null_map);

    std::vector<bool> rerated_hypernodes(_hg.initialNumNodes());
    // Used to prevent unnecessary re-rating of hypernodes that have been removed from
    // PQ because they are heavier than allowed.
    std::vector<bool> invalid_hypernodes(_hg.initialNumNodes());

    while (!_pq.empty() && _hg.numNodes() > limit) {
      const HypernodeID rep_node = _pq.max();
      const HypernodeID contracted_node = _target[rep_node];
      DBG(dbg_coarsening_coarsen, "Contracting: (" << rep_node << ","
          << _target[rep_node] << ") prio: " << _pq.maxKey());

      ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(_target[rep_node])
             <= _rater.thresholdNodeWeight(),
             "Trying to contract nodes violating maximum node weight");
      ASSERT(_pq.maxKey() == _rater.rate(rep_node).value,
             "Key in PQ != rating calculated by rater:" << _pq.maxKey() << "!="
             << _rater.rate(rep_node).value);
      ASSERT(!invalid_hypernodes[rep_node], "Representative HN " << rep_node << " is invalid");
      ASSERT(!invalid_hypernodes[contracted_node], "Contract HN " << contracted_node << " is invalid");

      performContraction(rep_node, contracted_node);
      _pq.remove(contracted_node);

      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      // We re-rate the representative HN here, because it might not have any incident HEs left.
      // In this case, it will not get re-rated by the call to reRateAffectedHypernodes.
      updatePQandContractionTarget(rep_node, _rater.rate(rep_node), invalid_hypernodes);
      rerated_hypernodes[rep_node] = 1;

      reRateAffectedHypernodes(rep_node, rerated_hypernodes, invalid_hypernodes);
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


  void reRateAffectedHypernodes(const HypernodeID rep_node,
                                std::vector<bool>& rerated_hypernodes,
                                std::vector<bool>& invalid_hypernodes) {
    for (const HyperedgeID he : _hg.incidentEdges(rep_node)) {
      for (const HypernodeID pin : _hg.pins(he)) {
        if (!rerated_hypernodes[pin] && !invalid_hypernodes[pin]) {
          const Rating rating = _rater.rate(pin);
          rerated_hypernodes[pin] = 1;
          updatePQandContractionTarget(pin, rating, invalid_hypernodes);
        }
      }
    }
    rerated_hypernodes.assign(rerated_hypernodes.size(), false);
  }


  void updatePQandContractionTarget(const HypernodeID hn, const Rating& rating,
                                    std::vector<bool>& invalid_hypernodes) {
    if (rating.valid) {
      ASSERT(_pq.contains(hn),
             "Trying to update rating of HN " << hn << " which is not in PQ");
      DBG(false, "Updating prio of HN " << hn << ": " << _pq.key(hn) << " (target=" << _target[hn]
          << ") --- >" << rating.value << "(target" << rating.target << ")");
      _pq.updateKey(hn, rating.value);
      _target[hn] = rating.target;
    } else if (_pq.contains(hn)) {
      // explicit containment check is necessary because of V-cycles. In this case, not
      // all hypernodes will be inserted into the PQ at the beginning, because of the
      // restriction that only hypernodes within the same part can be contracted.
      _pq.remove(hn);
      invalid_hypernodes[hn] = 1;
      _target[hn] = std::numeric_limits<HypernodeID>::max();
      DBG(dbg_coarsening_no_valid_contraction, "Progress [" << _hg.numNodes() << "/"
          << _hg.initialNumNodes() << "]:HN " << hn
          << " \t(w=" << _hg.nodeWeight(hn) << "," << " deg=" << _hg.nodeDegree(hn)
          << ") did not find valid contraction partner.");
      _stats.add("numHNsWithoutValidContractionPartner", _config.partition.current_v_cycle, 1);
    }
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_config;
  using Base::_rater;
  using Base::_history;
  using Base::_stats;
  using Base::_hypergraph_pruner;
  std::vector<HypernodeID> _target;

  private:
  DISALLOW_COPY_AND_ASSIGN(FullHeavyEdgeCoarsener);
};
} // namespace partition
#endif  // SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_
