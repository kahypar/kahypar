/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_

#include <boost/dynamic_bitset.hpp>

#include <string>
#include <unordered_map>
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
static const bool dbg_coarsening_removed_hes = false;

template <class Rater = Mandatory>
class HeuristicHeavyEdgeCoarsener : public ICoarsener,
                                    private HeavyEdgeCoarsenerBase<Rater>{
  private:
  typedef HeavyEdgeCoarsenerBase<Rater> Base;
  typedef typename Rater::Rating Rating;
  typedef std::unordered_multimap<HypernodeID, HypernodeID> TargetToSourcesMap;

  public:
  using Base::_pq;
  using Base::_hg;
  using Base::_rater;
  using Base::_history;
  using Base::_stats;
  using Base::_hypergraph_pruner;
  using Base::rateAllHypernodes;
  using Base::performContraction;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;
  using Base::gatherCoarseningStats;

  HeuristicHeavyEdgeCoarsener(Hypergraph& hypergraph, const Configuration& config) :
    HeavyEdgeCoarsenerBase<Rater>(hypergraph, config),
    _target(hypergraph.initialNumNodes()),
    _sources(hypergraph.initialNumNodes()),
    _just_updated(_hg.initialNumNodes()) { }

  ~HeuristicHeavyEdgeCoarsener() { }

  void coarsenImpl(int limit) final {
    _pq.clear();
    _sources.clear();

    rateAllHypernodes(_target, _sources);

    while (!_pq.empty() && _hg.numNodes() > limit) {
      const HypernodeID rep_node = _pq.max();
      const HypernodeID contracted_node = _target[rep_node];
      DBG(dbg_coarsening_coarsen, "Contracting: (" << rep_node << ","
          << _target[rep_node] << ") prio: " << _pq.maxKey());

      ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(_target[rep_node])
             <= _rater.thresholdNodeWeight(),
             "Trying to contract nodes violating maximum node weight");
      // The heuristic heavy edge coarsener does not update __all__ hypernodes
      // that should be updated. It only updates those nodes, which have the
      // contracted hypernode as target. Therefore this assertion does __not__
      // hold for the heuristic coarsener!
      // ASSERT(_pq.maxKey() == _rater.rate(rep_node).value,
      //        "Key in PQ != rating calculated by rater:" << _pq.maxKey() << "!="
      //        << _rater.rate(rep_node).value);

      performContraction(rep_node, contracted_node);
      _pq.remove(contracted_node);
      removeMappingEntryOfNode(contracted_node, _target[contracted_node]);

      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      updatePQandMappings(rep_node, _rater.rate(rep_node));

      reRateHypernodesAffectedByContraction(rep_node, contracted_node);
      reRateHypernodesAffectedByContraction(contracted_node, rep_node);

      reRateHypernodesAffectedByParallelHyperedgeRemoval();
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

  private:
  void removeMappingEntryOfNode(HypernodeID hn, HypernodeID hn_target) {
    auto range = _sources.equal_range(hn_target);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if (iter->second == hn) {
        DBG(false, "removing node " << hn << " from entries with key " << hn_target);
        _sources.erase(iter);
        break;
      }
    }
  }

  void reRateHypernodesAffectedByParallelHyperedgeRemoval() {
    Rating rating;
    _just_updated.reset();
    const auto& removed_parallel_hyperedges = _hypergraph_pruner.removedParallelHyperedges();
    for (int i = _history.top().parallel_hes_begin; i != _history.top().parallel_hes_begin +
         _history.top().parallel_hes_size; ++i) {
      for (const HypernodeID pin : _hg.pins(removed_parallel_hyperedges[i].representative_id)) {
        if (!_just_updated[pin]) {
          rating = _rater.rate(pin);
          updatePQandMappings(pin, rating);
          _just_updated[pin] = true;
        }
      }
    }
  }

  void reRateHypernodesAffectedByContraction(HypernodeID hn, HypernodeID contraction_node) {
    Rating rating;
    auto source_range = _sources.equal_range(hn);
    auto source_it = source_range.first;
    while (source_it != source_range.second) {
      if (source_it->second == contraction_node) {
        _sources.erase(source_it++);
      } else {
        DBG(false, "rerating HN " << source_it->second << " which had " << hn << " as target");
        rating = _rater.rate(source_it->second);
        // updatePQandMappings might invalidate source_it.
        HypernodeID source_hn = source_it->second;
        ++source_it;
        updatePQandMappings(source_hn, rating);
      }
    }
  }

  void updatePQandMappings(HypernodeID hn, const Rating& rating) {
    if (rating.valid) {
      ASSERT(_pq.contains(hn),
             "Trying to update rating of HN " << hn << " which is not in PQ");
      _pq.updateKey(hn, rating.value);
      if (rating.target != _target[hn]) {
        updateMappings(hn, rating);
      }
    } else if (_pq.contains(hn)) {
      // explicit containment check is necessary because of V-cycles. In this case, not
      // all hypernodes will be inserted into the PQ at the beginning, because of the
      // restriction that only hypernodes within the same part can be contracted.
      _pq.remove(hn);
      removeMappingEntryOfNode(hn, _target[hn]);
    }
  }

  void updateMappings(HypernodeID hn, const Rating& rating) {
    removeMappingEntryOfNode(hn, _target[hn]);
    _target[hn] = rating.target;
    _sources.insert({ rating.target, hn });
  }

  std::vector<HypernodeID> _target;
  TargetToSourcesMap _sources;
  boost::dynamic_bitset<uint64_t> _just_updated;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_
