/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_

#include <boost/dynamic_bitset.hpp>
#include <vector>

#include "partition/coarsening/HeavyEdgeCoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"

namespace partition {
template <class Hypergraph, class Rater>
class FullHeavyEdgeCoarsener : public ICoarsener<Hypergraph>,
                               public HeavyEdgeCoarsenerBase<Hypergraph, Rater>{
  private:
  typedef HeavyEdgeCoarsenerBase<Hypergraph, Rater> Base;
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Rater::Rating HeavyEdgeRating;

  public:
  FullHeavyEdgeCoarsener(Hypergraph& hypergraph, const Configuration<Hypergraph>& config) :
    HeavyEdgeCoarsenerBase<Hypergraph, Rater>(hypergraph, config) { }

  ~FullHeavyEdgeCoarsener() { }

  void coarsen(int limit) {
    ASSERT(Base::_pq.empty(), "coarsen() can only be called once");

    std::vector<HypernodeID> target(Base::_hg.initialNumNodes());
    rateAllHypernodes(target);

    HypernodeID rep_node;
    HypernodeID contracted_node;
    HeavyEdgeRating rating;
    boost::dynamic_bitset<uint64_t> rerated_hypernodes(Base::_hg.initialNumNodes());
    boost::dynamic_bitset<uint64_t> invalid_hypernodes(Base::_hg.initialNumNodes());

    while (!Base::_pq.empty() && Base::_hg.numNodes() > limit) {
      rep_node = Base::_pq.max();
      contracted_node = target[rep_node];
      // PRINT("Contracting: (" << rep_node << ","
      //       << target[rep_node] << ") prio: " << Base::_pq.maxKey());

      ASSERT(Base::_hg.nodeWeight(rep_node) + Base::_hg.nodeWeight(target[rep_node])
             <= Base::_rater.thresholdNodeWeight(),
             "Trying to contract nodes violating maximum node weight");

      Base::performContraction(rep_node, contracted_node);
      Base::_pq.remove(contracted_node);

      Base::removeSingleNodeHyperedges(rep_node);
      Base::removeParallelHyperedges(rep_node);

      rating = Base::_rater.rate(rep_node);
      rerated_hypernodes[rep_node] = 1;
      updatePQandContractionTargets(rep_node, rating, target, invalid_hypernodes);

      reRateAffectedHypernodes(rep_node, target, rerated_hypernodes, invalid_hypernodes);
    }
  }

  void uncoarsen(std::unique_ptr<IRefiner<Hypergraph> >& refiner) {
    Base::uncoarsen(refiner);
  }

  private:
  void rateAllHypernodes(std::vector<HypernodeID>& target) {
    HeavyEdgeRating rating;
    forall_hypernodes(hn, Base::_hg) {
      rating = Base::_rater.rate(*hn);
      if (rating.valid) {
        Base::_pq.insert(*hn, rating.value);
        target[*hn] = rating.target;
      }
    } endfor
  }

  void reRateAffectedHypernodes(HypernodeID rep_node,
                                std::vector<HypernodeID>& target,
                                boost::dynamic_bitset<uint64_t>& rerated_hypernodes,
                                boost::dynamic_bitset<uint64_t>& invalid_hypernodes) {
    HeavyEdgeRating rating;
    forall_incident_hyperedges(he, rep_node, Base::_hg) {
      forall_pins(pin, *he, Base::_hg) {
        if (!rerated_hypernodes[*pin] && !invalid_hypernodes[*pin]) {
          rating = Base::_rater.rate(*pin);
          rerated_hypernodes[*pin] = 1;
          updatePQandContractionTargets(*pin, rating, target, invalid_hypernodes);
        }
      } endfor
    } endfor
    rerated_hypernodes.reset();
  }


  void updatePQandContractionTargets(HypernodeID hn, const HeavyEdgeRating& rating,
                                     std::vector<HypernodeID>& target,
                                     boost::dynamic_bitset<uint64_t>& invalid_hypernodes) {
    if (rating.valid) {
      Base::_pq.update(hn, rating.value);
      target[hn] = rating.target;
    } else if (Base::_pq.contains(hn)) {
      Base::_pq.remove(hn);
      invalid_hypernodes[hn] = 1;
    }
  }
};
} // namespace partition
#endif  // SRC_PARTITION_COARSENING_FULLHEAVYEDGECOARSENER_H_
