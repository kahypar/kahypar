/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/coarsening/HeavyEdgeCoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"

using defs::Hypergraph;
using defs::HypernodeID;
using datastructure::FastResetBitVector;
using utils::Stats;

namespace partition {
static const bool dbg_coarsening_removed_hes = false;

template <class Rater = Mandatory>
class HeuristicHeavyEdgeCoarsener final : public ICoarsener,
                                          private HeavyEdgeCoarsenerBase<Rater>{
 private:
  using Base = HeavyEdgeCoarsenerBase<Rater>;
  using Base::rateAllHypernodes;
  using Base::performContraction;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;
  using Rating = typename Rater::Rating;
  using TargetToSourcesMap = std::unordered_multimap<HypernodeID, HypernodeID>;

 public:
  HeuristicHeavyEdgeCoarsener(Hypergraph& hypergraph, const Configuration& config,
                              const HypernodeWeight weight_of_heaviest_node) noexcept :
    HeavyEdgeCoarsenerBase<Rater>(hypergraph, config, weight_of_heaviest_node),
    _target(hypergraph.initialNumNodes()),
    _sources(hypergraph.initialNumNodes()),
    _just_updated(_hg.initialNumNodes(), false) { }

  virtual ~HeuristicHeavyEdgeCoarsener() { }

  HeuristicHeavyEdgeCoarsener(const HeuristicHeavyEdgeCoarsener&) = delete;
  HeuristicHeavyEdgeCoarsener& operator= (const HeuristicHeavyEdgeCoarsener&) = delete;

  HeuristicHeavyEdgeCoarsener(HeuristicHeavyEdgeCoarsener&&) = delete;
  HeuristicHeavyEdgeCoarsener& operator= (HeuristicHeavyEdgeCoarsener&&) = delete;

 private:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void coarsenImpl(const HypernodeID limit) noexcept override final {
    _pq.clear();
    _sources.clear();

    rateAllHypernodes(_target, _sources);

    while (!_pq.empty() && _hg.numNodes() > limit) {
      const HypernodeID rep_node = _pq.getMax();
      const HypernodeID contracted_node = _target[rep_node];
      DBG(dbg_coarsening_coarsen, "Contracting: (" << rep_node << ","
          << _target[rep_node] << ") prio: " << _pq.getMaxKey());

      ASSERT(_hg.nodeWeight(rep_node) + _hg.nodeWeight(_target[rep_node])
             <= _rater.thresholdNodeWeight(),
             "Trying to contract nodes violating maximum node weight");
      // The heuristic heavy edge coarsener does not update __all__ hypernodes
      // that should be updated. It only updates those nodes, which have the
      // contracted hypernode as target. Therefore this assertion does __not__
      // hold for the heuristic coarsener!
      // ASSERT(_pq.getMaxKey() == _rater.rate(rep_node).value,
      //        "Key in PQ != rating calculated by rater:" << _pq.getMaxKey() << "!="
      //        << _rater.rate(rep_node).value);

      performContraction(rep_node, contracted_node);

      ASSERT(_pq.contains(contracted_node), V(contracted_node));
      _pq.deleteNode(contracted_node);
      removeMappingEntryOfNode(contracted_node, _target[contracted_node]);

      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      updatePQandMappings(rep_node, _rater.rate(rep_node));

      reRateHypernodesAffectedByContraction(rep_node, contracted_node);
      reRateHypernodesAffectedByContraction(contracted_node, rep_node);

      reRateHypernodesAffectedByParallelHyperedgeRemoval();
    }
  }

  bool uncoarsenImpl(IRefiner& refiner) noexcept override final {
    return Base::doUncoarsen(refiner);
  }

  std::string policyStringImpl() const noexcept override final {
    return std::string(" ratingFunction=" + templateToString<Rater>());
  }

  void removeMappingEntryOfNode(const HypernodeID hn, const HypernodeID hn_target) noexcept {
    auto range = _sources.equal_range(hn_target);
    for (auto iter = range.first; iter != range.second; ++iter) {
      if (iter->second == hn) {
        DBG(false, "removing node " << hn << " from entries with key " << hn_target);
        _sources.erase(iter);
        break;
      }
    }
  }

  void reRateHypernodesAffectedByParallelHyperedgeRemoval() noexcept {
    _just_updated.resetAllBitsToFalse();
    const auto& removed_parallel_hyperedges = _hypergraph_pruner.removedParallelHyperedges();
    for (int i = _history.back().parallel_hes_begin; i != _history.back().parallel_hes_begin +
         _history.back().parallel_hes_size; ++i) {
      for (const HypernodeID pin : _hg.pins(removed_parallel_hyperedges[i].representative_id)) {
        if (!_just_updated[pin]) {
          const Rating rating = _rater.rate(pin);
          updatePQandMappings(pin, rating);
          _just_updated.setBit(pin, true);
        }
      }
    }
  }

  void reRateHypernodesAffectedByContraction(const HypernodeID hn,
                                             const HypernodeID contraction_node) noexcept {
    auto source_range = _sources.equal_range(hn);
    auto source_it = source_range.first;
    while (source_it != source_range.second) {
      if (source_it->second == contraction_node) {
        _sources.erase(source_it++);
      } else {
        DBG(false, "rerating HN " << source_it->second << " which had " << hn << " as target");
        const Rating rating = _rater.rate(source_it->second);
        // updatePQandMappings might invalidate source_it.
        const HypernodeID source_hn = source_it->second;
        ++source_it;
        updatePQandMappings(source_hn, rating);
      }
    }
  }

  void updatePQandMappings(const HypernodeID hn, const Rating& rating) noexcept {
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
      _pq.deleteNode(hn);
      removeMappingEntryOfNode(hn, _target[hn]);
      DBG(dbg_coarsening_no_valid_contraction, "Progress [" << _hg.numNodes() << "/"
          << _hg.initialNumNodes() << "]:HN " << hn
          << " \t(w=" << _hg.nodeWeight(hn) << "," << " deg=" << _hg.nodeDegree(hn)
          << ") did not find valid contraction partner.");
#ifdef GATHER_STATS
      Stats::instance().add(config, "numHNsWithoutValidContractionPartner", 1);
#endif
    }
  }

  void updateMappings(const HypernodeID hn, const Rating& rating) noexcept {
    removeMappingEntryOfNode(hn, _target[hn]);
    _target[hn] = rating.target;
    _sources.insert({ rating.target, hn });
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_config;
  using Base::_rater;
  using Base::_history;
  using Base::_hypergraph_pruner;
  std::vector<HypernodeID> _target;
  TargetToSourcesMap _sources;
  FastResetBitVector<> _just_updated;
};
}  // namespace partition

#endif  // SRC_PARTITION_COARSENING_HEURISTICHEAVYEDGECOARSENER_H_
