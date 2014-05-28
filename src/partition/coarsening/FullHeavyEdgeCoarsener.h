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
#include "lib/datastructure/Hypergraph.h"
#include "partition/coarsening/HeavyEdgeCoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"

using datastructure::HypergraphType;
using datastructure::HypernodeID;

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
  using Base::rateAllHypernodes;
  using Base::performContraction;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;

  FullHeavyEdgeCoarsener(HypergraphType& hypergraph, const Configuration& config) :
    HeavyEdgeCoarsenerBase<Rater>(hypergraph, config) { }

  ~FullHeavyEdgeCoarsener() { }

  void coarsenImpl(int limit) final {
    _pq.clear();

    std::vector<HypernodeID> target(_hg.initialNumNodes());
    NullMap null_map;
    rateAllHypernodes(target, null_map);

    HypernodeID rep_node;
    HypernodeID contracted_node;
    Rating rating;
    boost::dynamic_bitset<uint64_t> rerated_hypernodes(_hg.initialNumNodes());
    boost::dynamic_bitset<uint64_t> invalid_hypernodes(_hg.initialNumNodes());

    while (!_pq.empty() && _hg.numNodes() > limit) {
      rep_node = _pq.max();
      contracted_node = target[rep_node];
      DBG(dbg_coarsening_coarsen, "Contracting: (" << rep_node << ","
          << target[rep_node] << ") prio: " << _pq.maxKey());

      ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(target[rep_node])
             <= _rater.thresholdNodeWeight(),
             "Trying to contract nodes violating maximum node weight");
      ASSERT(_pq.maxKey() == _rater.rate(rep_node).value,
             "Key in PQ != rating calculated by rater:" << _pq.maxKey() << "!="
             << _rater.rate(rep_node).value);

      performContraction(rep_node, contracted_node);
      _pq.remove(contracted_node);

      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      rating = _rater.rate(rep_node);
      rerated_hypernodes[rep_node] = 1;
      updatePQandContractionTargets(rep_node, rating, target, invalid_hypernodes);

      reRateAffectedHypernodes(rep_node, target, rerated_hypernodes, invalid_hypernodes);
    }
  }

  void uncoarsenImpl(IRefiner& refiner) final {
    Base::doUncoarsen(refiner);
  }

  std::string policyStringImpl() const final {
    return std::string(" ratingFunction=" + templateToString<Rater>());
  }

  private:
  void reRateAffectedHypernodes(HypernodeID rep_node,
                                std::vector<HypernodeID>& target,
                                boost::dynamic_bitset<uint64_t>& rerated_hypernodes,
                                boost::dynamic_bitset<uint64_t>& invalid_hypernodes) {
    Rating rating;
    forall_incident_hyperedges(he, rep_node, _hg) {
      forall_pins(pin, *he, _hg) {
        if (!rerated_hypernodes[*pin] && !invalid_hypernodes[*pin]) {
          rating = _rater.rate(*pin);
          rerated_hypernodes[*pin] = 1;
          updatePQandContractionTargets(*pin, rating, target, invalid_hypernodes);
        }
      } endfor
    } endfor
    rerated_hypernodes.reset();
  }


  void updatePQandContractionTargets(HypernodeID hn, const Rating& rating,
                                     std::vector<HypernodeID>& target,
                                     boost::dynamic_bitset<uint64_t>& invalid_hypernodes) {
    if (rating.valid) {
      ASSERT(_pq.contains(hn),
             "Trying to update rating of HN " << hn << " which is not in PQ");
      _pq.updateKey(hn, rating.value);
      target[hn] = rating.target;
    } else if (_pq.contains(hn)) {
      _pq.remove(hn);
      invalid_hypernodes[hn] = 1;
    }
  }
};
} // namespace partition
#endif  // SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_
