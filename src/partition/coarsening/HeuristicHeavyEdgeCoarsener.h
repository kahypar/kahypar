#ifndef PARTITION_COARSENING_HEAVYEDGECOARSENER_H_
#define PARTITION_COARSENING_HEAVYEDGECOARSENER_H_

#include <vector>
#include <unordered_map>

#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/HeavyEdgeCoarsenerBase.h"
#include "partition/refinement/IRefiner.h"

namespace partition {

template <class Hypergraph, class Rater>
class HeuristicHeavyEdgeCoarsener : public ICoarsener<Hypergraph>, public HeavyEdgeCoarsenerBase<Hypergraph, Rater> {
 private:
  typedef HeavyEdgeCoarsenerBase<Hypergraph,Rater> Base;
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Rater::Rating HeavyEdgeRating;
  typedef std::unordered_multimap<HypernodeID, HypernodeID> TargetToSourcesMap;
 public:
  HeuristicHeavyEdgeCoarsener(Hypergraph& hypergraph, const Configuration<Hypergraph>& config) :
      HeavyEdgeCoarsenerBase<Hypergraph, Rater>(hypergraph, config) {}

    ~HeuristicHeavyEdgeCoarsener() {}
  
  void coarsen(int limit) {
   ASSERT(Base::_pq.empty(), "coarsen() can only be called once");

    std::vector<HypernodeID> target(Base::_hg.initialNumNodes());
    TargetToSourcesMap sources;
    
    rateAllHypernodes(target, sources);

    HypernodeID rep_node;
    HypernodeID contracted_node;
    HeavyEdgeRating rating;
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
      removeMappingEntryOfNode(contracted_node, target[contracted_node], sources);

      Base::removeSingleNodeHyperedges(rep_node);
      Base::removeParallelHyperedges(rep_node);

      rating = Base::_rater.rate(rep_node);
      updatePQandMappings(rep_node, rating, target, sources);

      reRateHypernodesAffectedByContraction(rep_node, contracted_node, target, sources);
      reRateHypernodesAffectedByContraction(contracted_node, rep_node, target, sources);

      reRateHypernodesAffectedByParallelHyperedgeRemoval(target, sources);
    }
  } 
  void uncoarsen(std::unique_ptr<IRefiner<Hypergraph>>& refiner) {
    Base::uncoarsen(refiner);
  }

 private:
  void removeMappingEntryOfNode(HypernodeID hn, HypernodeID hn_target,
                                TargetToSourcesMap& sources) {
    auto range = sources.equal_range(hn_target);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if (iter->second == hn) {
        //PRINT("********** removing node " << hn << " from entries with key " << target[hn]);
        sources.erase(iter);
        break;
      }
    }
  }

  void rateAllHypernodes(std::vector<HypernodeID>& target, TargetToSourcesMap& sources) {
    HeavyEdgeRating rating;
    forall_hypernodes(hn, Base::_hg) {
      rating = Base::_rater.rate(*hn);
      if (rating.valid) {
        Base::_pq.insert(*hn, rating.value);
        target[*hn] = rating.target;
        sources.insert({rating.target, *hn});
      }
    } endfor 
          
  }

   void reRateHypernodesAffectedByParallelHyperedgeRemoval(std::vector<HypernodeID>& target,
                                                          TargetToSourcesMap& sources) {
    HeavyEdgeRating rating;
    for (int i = Base::_history.top().parallel_hes_begin; i != Base::_history.top().parallel_hes_begin +
                 Base::_history.top().parallel_hes_size; ++i) {
      forall_pins(pin, Base::_removed_parallel_hyperedges[i].representative_id, Base::_hg) {
          rating = Base::_rater.rate(*pin);
          updatePQandMappings(*pin, rating, target, sources);
      } endfor
    }
  }

    void reRateHypernodesAffectedByContraction(HypernodeID hn, HypernodeID contraction_node,
                                             std::vector<HypernodeID>& target,
                                             TargetToSourcesMap& sources) {
    HeavyEdgeRating rating;
    auto source_range = sources.equal_range(hn);
    auto source_it = source_range.first;
    while (source_it != source_range.second) {
      if (source_it->second == contraction_node) {
        sources.erase(source_it++);
      } else {
        //PRINT("rerating HN " << source_it->second << " which had " << hn << " as target");
        rating = Base::_rater.rate(source_it->second);
        // updatePQandMappings might invalidate source_it.
        HypernodeID source_hn = source_it->second;
        ++source_it;
        updatePQandMappings(source_hn, rating, target, sources);
      }
    }
  }

   void updatePQandMappings(HypernodeID hn, const HeavyEdgeRating& rating,
                           std::vector<HypernodeID>& target, TargetToSourcesMap& sources) {
    if (rating.valid) {
      Base::_pq.updateKey(hn, rating.value);
      if (rating.target != target[hn]) {
        updateMappings(hn, rating, target, sources);
      }
    } else if (Base::_pq.contains(hn)) {
      Base::_pq.remove(hn);
      removeMappingEntryOfNode(hn, target[hn], sources);
    }    
  }

  void updateMappings(HypernodeID hn, const HeavyEdgeRating& rating,
                      std::vector<HypernodeID>& target, TargetToSourcesMap& sources) {
    removeMappingEntryOfNode(hn, target[hn], sources);
    target[hn] = rating.target;
    sources.insert({rating.target, hn});
  }

};








  

} // namespace partition

#endif  // PARTITION_COARSENING_HEAVYEDGECOARSENER_H_
