/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_

#include <unordered_map>
#include <vector>

#include "lib/datastructure/Hypergraph.h"
#include "partition/coarsening/HeavyEdgeCoarsenerBase.h"

using datastructure::HypergraphType;
using datastructure::HypernodeID;

namespace partition {
template <class Rater>
class HeuristicHeavyEdgeCoarsener : public HeavyEdgeCoarsenerBase<Rater>{
  private:
  typedef HeavyEdgeCoarsenerBase<Rater> Base;
  typedef typename Rater::Rating HeavyEdgeRating;
  typedef std::unordered_multimap<HypernodeID, HypernodeID> TargetToSourcesMap;

  using Base::_pq;
  using Base::_hg;
  using Base::_rater;
  using Base::_history;
  using Base::_removed_parallel_hyperedges;
  using Base::_removed_single_node_hyperedges;
  using Base::rateAllHypernodes;
  using Base::performContraction;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;

  public:
  HeuristicHeavyEdgeCoarsener(HypergraphType& hypergraph, const Configuration& config) :
    HeavyEdgeCoarsenerBase<Rater>(hypergraph, config) { }

  ~HeuristicHeavyEdgeCoarsener() { }

  void coarsen(int limit) {
    _pq.clear();

    std::vector<HypernodeID> target(_hg.initialNumNodes());
    TargetToSourcesMap sources;

    rateAllHypernodes(target, sources);

    HypernodeID rep_node;
    HypernodeID contracted_node;
    HeavyEdgeRating rating;
    while (!_pq.empty() && _hg.numNodes() > limit) {
      rep_node = _pq.max();
      contracted_node = target[rep_node];
      DBG(dbg_coarsening_coarsen, "Contracting: (" << rep_node << ","
          << target[rep_node] << ") prio: " << _pq.maxKey());

      ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(target[rep_node])
             <= _rater.thresholdNodeWeight(),
             "Trying to contract nodes violating maximum node weight");

      performContraction(rep_node, contracted_node);
      _pq.remove(contracted_node);
      removeMappingEntryOfNode(contracted_node, target[contracted_node], sources);

      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      rating = _rater.rate(rep_node);
      updatePQandMappings(rep_node, rating, target, sources);

      reRateHypernodesAffectedByContraction(rep_node, contracted_node, target, sources);
      reRateHypernodesAffectedByContraction(contracted_node, rep_node, target, sources);

      reRateHypernodesAffectedByParallelHyperedgeRemoval(target, sources);
    }
  }

  private:
  void removeMappingEntryOfNode(HypernodeID hn, HypernodeID hn_target,
                                TargetToSourcesMap& sources) {
    auto range = sources.equal_range(hn_target);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if (iter->second == hn) {
        DBG(false, "removing node " << hn << " from entries with key " << hn_target);
        sources.erase(iter);
        break;
      }
    }
  }

  void reRateHypernodesAffectedByParallelHyperedgeRemoval(std::vector<HypernodeID>& target,
                                                          TargetToSourcesMap& sources) {
    HeavyEdgeRating rating;
    for (int i = _history.top().parallel_hes_begin; i != _history.top().parallel_hes_begin +
         _history.top().parallel_hes_size; ++i) {
      forall_pins(pin, _removed_parallel_hyperedges[i].representative_id, _hg) {
        rating = _rater.rate(*pin);
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
        DBG(false, "rerating HN " << source_it->second << " which had " << hn << " as target");
        rating = _rater.rate(source_it->second);
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
      ASSERT(_pq.contains(hn),
             "Trying to update rating of HN " << hn << " which is not in PQ");
      _pq.updateKey(hn, rating.value);
      if (rating.target != target[hn]) {
        updateMappings(hn, rating, target, sources);
      }
    } else if (_pq.contains(hn)) {
      _pq.remove(hn);
      removeMappingEntryOfNode(hn, target[hn], sources);
    }
  }

  void updateMappings(HypernodeID hn, const HeavyEdgeRating& rating,
                      std::vector<HypernodeID>& target, TargetToSourcesMap& sources) {
    removeMappingEntryOfNode(hn, target[hn], sources);
    target[hn] = rating.target;
    sources.insert({ rating.target, hn });
  }
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_
