/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_

#include <boost/dynamic_bitset.hpp>
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

  FullHeavyEdgeCoarsener(Hypergraph& hypergraph, const Configuration& config) :
    HeavyEdgeCoarsenerBase<Rater>(hypergraph, config),
    _target(hypergraph.initialNumNodes()) { }

  ~FullHeavyEdgeCoarsener() { }

  void coarsenImpl(int limit) final {
    _pq.clear();

    NullMap null_map;
    rateAllHypernodes(_target, null_map);

    HypernodeID rep_node;
    HypernodeID contracted_node;
    Rating rating;
    boost::dynamic_bitset<uint64_t> rerated_hypernodes(_hg.initialNumNodes());
    // Used to prevent unnecessary re-rating of hypernodes that have been removed from
    // PQ because they are heavier than allowed.
    boost::dynamic_bitset<uint64_t> invalid_hypernodes(_hg.initialNumNodes());

    while (!_pq.empty() && _hg.numNodes() > limit) {
      rep_node = _pq.max();
      contracted_node = _target[rep_node];
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

      rating = _rater.rate(rep_node);
      rerated_hypernodes[rep_node] = 1;
      updatePQandContractionTargets(rep_node, rating, invalid_hypernodes);

      reRateAffectedHypernodes(rep_node, rerated_hypernodes, invalid_hypernodes);
    }
    gatherCoarseningStats();
  }

  void uncoarsenImpl(IRefiner& refiner) final {
    Base::doUncoarsen(refiner);
  }

  const Stats & statsImpl() const {
    return _stats;
  }

  std::string policyStringImpl() const final {
    return std::string(" ratingFunction=" + templateToString<Rater>());
  }

  private:
  void reRateAffectedHypernodes(HypernodeID rep_node,
                                boost::dynamic_bitset<uint64_t>& rerated_hypernodes,
                                boost::dynamic_bitset<uint64_t>& invalid_hypernodes) {
    Rating rating;
    for (auto && he : _hg.incidentEdges(rep_node)) {
      for (auto && pin : _hg.pins(he)) {
        if (!rerated_hypernodes[pin] && !invalid_hypernodes[pin]) {
          rating = _rater.rate(pin);
          rerated_hypernodes[pin] = 1;
          updatePQandContractionTargets(pin, rating, invalid_hypernodes);
        }
      }
    }
    rerated_hypernodes.reset();
  }


  void updatePQandContractionTargets(HypernodeID hn, const Rating& rating,
                                     boost::dynamic_bitset<uint64_t>& invalid_hypernodes) {
    if (rating.valid) {
      ASSERT(_pq.contains(hn),
             "Trying to update rating of HN " << hn << " which is not in PQ");
      _pq.updateKey(hn, rating.value);
      _target[hn] = rating.target;
    } else if (_pq.contains(hn)) {
      _pq.remove(hn);
      invalid_hypernodes[hn] = 1;
    }
  }

  std::vector<HypernodeID> _target;
};
} // namespace partition
#endif  // SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_
