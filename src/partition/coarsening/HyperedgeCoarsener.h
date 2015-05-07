/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_

#include <string>
#include <vector>

#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"
#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/HyperedgeRatingPolicies.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using external::NoDataBinaryMaxHeap;
using datastructure::PriorityQueue;
using datastructure::MetaKeyDouble;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using utils::Stats;

namespace partition {
struct HyperedgeCoarseningMemento {
  int one_pin_hes_begin;        // start of removed single pin hyperedges
  int one_pin_hes_size;         // # removed single pin hyperedges
  int parallel_hes_begin;       // start of removed parallel hyperedges
  int parallel_hes_size;        // # removed parallel hyperedges
  int mementos_begin;           // start of mementos corresponding to HE contraction
  int mementos_size;            // # mementos
  explicit HyperedgeCoarseningMemento() :
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    mementos_begin(0),
    mementos_size(0) { }
};

template <class RatingPolicy = Mandatory>
class HyperedgeCoarsener : public ICoarsener,
                           public CoarsenerBase<HyperedgeCoarseningMemento>{
 private:
  using Base = CoarsenerBase<HyperedgeCoarseningMemento>;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;
  using Base::restoreParallelHyperedges;
  using Base::restoreSingleNodeHyperedges;
  using Base::performLocalSearch;
  using Base::initializeRefiner;
  using Base::gatherCoarseningStats;
  using Rating = HyperedgeRating;
  using ContractionMemento = typename Hypergraph::ContractionMemento;

 public:
  HyperedgeCoarsener(const HyperedgeCoarsener&) = delete;
  HyperedgeCoarsener(HyperedgeCoarsener&&) = delete;
  HyperedgeCoarsener& operator = (const HyperedgeCoarsener&) = delete;
  HyperedgeCoarsener& operator = (HyperedgeCoarsener&&) = delete;

  HyperedgeCoarsener(Hypergraph& hypergraph, const Configuration& config) noexcept :
    Base(hypergraph, config),
    _pq(_hg.initialNumEdges()),
    _contraction_mementos() { }

 private:
  FRIEND_TEST(AHyperedgeCoarsener, RemembersMementosOfNodeContractionsDuringOneCoarseningStep);
  FRIEND_TEST(AHyperedgeCoarsener, DoesNotEnqueueHyperedgesThatWouldViolateThresholdNodeWeight);
  FRIEND_TEST(HyperedgeCoarsener, DeleteRemovedSingleNodeHyperedgesFromPQ);
  FRIEND_TEST(HyperedgeCoarsener, DeleteRemovedParallelHyperedgesFromPQ);
  FRIEND_TEST(AHyperedgeCoarsener, UpdatesRatingsOfIncidentHyperedgesOfRepresentativeAfterContraction);
  FRIEND_TEST(AHyperedgeCoarsener, RemovesHyperedgesThatWouldViolateThresholdNodeWeightFromPQonUpdate);
  FRIEND_TEST(HyperedgeCoarsener, AddRepresentativeOnlyOnceToRefinementNodes);

  void coarsenImpl(const HypernodeID limit) noexcept final {
    _pq.clear();
    rateAllHyperedges();

    while (!_pq.empty() && _hg.numNodes() > limit) {
      const HyperedgeID he_to_contract = _pq.max();
      DBG(dbg_coarsening_coarsen, "Contracting HE" << he_to_contract << " prio: " << _pq.maxKey());
      DBG(dbg_coarsening_coarsen, "w(" << he_to_contract << ")=" << _hg.edgeWeight(he_to_contract));
      DBG(dbg_coarsening_coarsen, "|" << he_to_contract << "|=" << _hg.edgeSize(he_to_contract));

      ASSERT([&]() {
          HypernodeWeight total_weight = 0;
          for (const HypernodeID pin : _hg.pins(he_to_contract)) {
            total_weight += _hg.nodeWeight(pin);
          }
          return total_weight;
        } () <= _config.coarsening.max_allowed_node_weight,
             "Contracting HE " << he_to_contract << "leads to violation of node weight threshold");
      ASSERT(_pq.maxKey() == RatingPolicy::rate(he_to_contract, _hg,
                                                _config.coarsening.max_allowed_node_weight).value,
             "Key in PQ != rating calculated by rater:" << _pq.maxKey() << "!="
             << RatingPolicy::rate(he_to_contract, _hg, _config.coarsening.max_allowed_node_weight).value);

      //TODO(schlag): If contraction would lead to too few hypernodes, we are not allowed to contract
      //              this HE. Instead we just remove it from the PQ? -> make a testcase!
      //              Or do we just say we coarsen until there are no more than 150 nodes left?

      const HypernodeID rep_node = performContraction(he_to_contract);
      _pq.remove(he_to_contract);

      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      deleteRemovedSingleNodeHyperedgesFromPQ();
      deleteRemovedParallelHyperedgesFromPQ();

      reRateHyperedgesAffectedByContraction(rep_node);
    }
    gatherCoarseningStats();
  }

  bool uncoarsenImpl(IRefiner& refiner) noexcept final {
    double current_imbalance = metrics::imbalance(_hg);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
    const HyperedgeWeight initial_cut = current_cut;

    initializeRefiner(refiner);
    std::vector<HypernodeID> refinement_nodes;
    refinement_nodes.reserve(_hg.initialNumNodes());
    size_t num_refinement_nodes = 0;
    while (!_history.empty()) {
      num_refinement_nodes = 0;
      restoreParallelHyperedges();
      restoreSingleNodeHyperedges();
      performUncontraction(_history.back(), refinement_nodes, num_refinement_nodes);
      performLocalSearch(refiner, refinement_nodes, num_refinement_nodes,
                         current_imbalance, current_cut);
      _history.pop_back();
    }
    return current_cut < initial_cut;
    // ASSERT(current_imbalance <= _config.partition.epsilon,
    //        "balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg)
    //        << " > " << _config.partition.epsilon);
  }

  const Stats & statsImpl() const noexcept {
    return _stats;
  }

  void removeHyperedgeFromPQ(const HyperedgeID he) noexcept {
    if (_pq.contains(he)) {
      _pq.remove(he);
    }
  }

  std::string policyStringImpl() const noexcept final {
    return std::string(" ratingFunction=" + templateToString<RatingPolicy>());
  }

  void deleteRemovedSingleNodeHyperedgesFromPQ() noexcept {
    const auto& removed_single_node_hyperedges = _hypergraph_pruner.removedSingleNodeHyperedges();
    for (int i = _history.back().one_pin_hes_begin; i != _history.back().one_pin_hes_begin +
         _history.back().one_pin_hes_size; ++i) {
      removeHyperedgeFromPQ(removed_single_node_hyperedges[i]);
    }
  }

  void deleteRemovedParallelHyperedgesFromPQ() noexcept {
    const auto& removed_parallel_hyperedges = _hypergraph_pruner.removedParallelHyperedges();
    for (int i = _history.back().parallel_hes_begin; i != _history.back().parallel_hes_begin +
         _history.back().parallel_hes_size; ++i) {
      removeHyperedgeFromPQ(removed_parallel_hyperedges[i].removed_id);
    }
  }


  void rateAllHyperedges() noexcept {
    std::vector<HyperedgeID> permutation;
    permutation.reserve(_hg.initialNumNodes());
    for (const HyperedgeID he : _hg.edges()) {
      permutation.push_back(he);
    }
    Randomize::shuffleVector(permutation, permutation.size());

    Rating rating;
    for (const HyperedgeID he : permutation) {
      rating = RatingPolicy::rate(he, _hg, _config.coarsening.max_allowed_node_weight);
      if (rating.valid) {
        // HEs that would violate node_weight_treshold are not inserted
        // since their rating is set to invalid!
        DBG(dbg_coarsening_rating, "Inserting HE " << he << " rating=" << rating.value);
        _pq.insert(he, rating.value);
      }
    }
  }

  void reRateHyperedgesAffectedByContraction(const HypernodeID representative) noexcept {
    Rating rating;
    for (const HyperedgeID he : _hg.incidentEdges(representative)) {
      DBG(false, "Looking at HE " << he);
      if (_pq.contains(he)) {
        rating = RatingPolicy::rate(he, _hg, _config.coarsening.max_allowed_node_weight);
        if (rating.valid) {
          DBG(false, "Updating HE " << he << " rating=" << rating.value);
          _pq.updateKey(he, rating.value);
        } else {
          _pq.remove(he);
        }
      }
    }
  }

  HypernodeID performContraction(const HyperedgeID he) noexcept {
    _history.emplace_back(HyperedgeCoarseningMemento());
    _history.back().mementos_begin = _contraction_mementos.size();
    auto pins_begin = _hg.pins(he).first;
    auto pins_end = _hg.pins(he).second;
    HypernodeID representative = *pins_begin;
    ++pins_begin;
    //TODO(schlag): modify hypergraph DS such that we can do index-based iteration over incidence array
    //              via custom iterator (so that we do not need to collect the IDs to contract upfront).
    std::vector<HypernodeID> hns_to_contract;
    while (pins_begin != pins_end) {
      hns_to_contract.push_back(*pins_begin);
      ++pins_begin;
    }
    for (const HypernodeID hn_to_contract : hns_to_contract) {
      DBG(dbg_coarsening_coarsen, "Contracting (" << representative << "," << hn_to_contract
          << ") from HE " << he);
      _contraction_mementos.emplace_back(_hg.contract(representative, hn_to_contract));
      ++_history.back().mementos_size;
    }
    return representative;
  }

  void performUncontraction(const HyperedgeCoarseningMemento& memento,
                            std::vector<HypernodeID>& refinement_nodes,
                            size_t& num_refinement_nodes) noexcept {
    // Hypergraphs can contain hyperedges of size 1. These HEs will get contracted without
    // a contraction memento. Thus the HyperedgeCoarseningMemento will only contain information
    // about removed single-node hyperedges and nothing should be done here.
    if (memento.mementos_size > 0) {
      refinement_nodes[num_refinement_nodes++] = _contraction_mementos[memento.mementos_begin
                                                                       + memento.mementos_size - 1].u;
      for (int i = memento.mementos_begin + memento.mementos_size - 1;
           i >= memento.mementos_begin; --i) {
        ASSERT(_hg.nodeIsEnabled(_contraction_mementos[i].u),
               "Representative HN " << _contraction_mementos[i].u << " is disabled ");
        ASSERT(!_hg.nodeIsEnabled(_contraction_mementos[i].v),
               "Representative HN " << _contraction_mementos[i].v << " is enabled");
        DBG(dbg_coarsening_uncoarsen, "Uncontracting: (" << _contraction_mementos[i].u << ","
            << _contraction_mementos[i].v << ")");
        _hg.uncontract(_contraction_mementos[i]);
        refinement_nodes[num_refinement_nodes++] = _contraction_mementos[i].v;
      }
    }
  }

  using Base::_hg;
  using Base::_config;
  using Base::_history;
  using Base::_stats;
  using Base::_hypergraph_pruner;
  PriorityQueue<NoDataBinaryMaxHeap<HyperedgeID, RatingType, MetaKeyDouble> > _pq;
  std::vector<ContractionMemento> _contraction_mementos;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_
