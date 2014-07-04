/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_

#include <string>
#include <vector>

#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/HyperedgeRatingPolicies.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using datastructure::PriorityQueue;
using datastructure::MetaKeyDouble;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;

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
                           public CoarsenerBase<HyperedgeCoarsener<RatingPolicy>,
                                                HyperedgeCoarseningMemento>{
  private:
  typedef HyperedgeRating Rating;
  typedef typename Hypergraph::ContractionMemento ContractionMemento;
  typedef CoarsenerBase<HyperedgeCoarsener<RatingPolicy>, HyperedgeCoarseningMemento> Base;

  public:
  using Base::_hg;
  using Base::_config;
  using Base::_history;
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;
  using Base::restoreParallelHyperedges;
  using Base::restoreSingleNodeHyperedges;
  using Base::performLocalSearch;
  using Base::initializeRefiner;

  HyperedgeCoarsener(Hypergraph& hypergraph, const Configuration& config) :
    Base(hypergraph, config),
    _pq(_hg.initialNumEdges()),
    _contraction_mementos() { }

  void coarsenImpl(int limit) final {
    _pq.clear();
    rateAllHyperedges();

    HyperedgeID he_to_contract;
    while (!_pq.empty() && _hg.numNodes() > limit) {
      he_to_contract = _pq.max();
      DBG(dbg_coarsening_coarsen, "Contracting HE" << he_to_contract << " prio: " << _pq.maxKey());
      DBG(dbg_coarsening_coarsen, "w(" << he_to_contract << ")=" << _hg.edgeWeight(he_to_contract));
      DBG(dbg_coarsening_coarsen, "|" << he_to_contract << "|=" << _hg.edgeSize(he_to_contract));
      //getchar();
      ASSERT([&]() {
               HypernodeWeight total_weight = 0;
               for (auto && pin : _hg.pins(he_to_contract)) {
                 total_weight += _hg.nodeWeight(pin);
               }
               return total_weight;
             } () <= _config.coarsening.threshold_node_weight,
             "Contracting HE " << he_to_contract << "leads to violation of node weight thsreshold");
      ASSERT(_pq.maxKey() == RatingPolicy::rate(he_to_contract, _hg,
                                                _config.coarsening.threshold_node_weight).value,
             "Key in PQ != rating calculated by rater:" << _pq.maxKey() << "!="
             << RatingPolicy::rate(he_to_contract, _hg, _config.coarsening.threshold_node_weight).value);

      //TODO(schlag): If contraction would lead to too few hypernodes, we are not allowed to contract
      //              this HE. Instead we just remove it from the PQ? -> make a testcase!
      //              Or do we just say we coarsen until there are no more than 150 nodes left?

      HypernodeID rep_node = performContraction(he_to_contract);
      _pq.remove(he_to_contract);

      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      reRateHyperedgesAffectedByContraction(rep_node);
    }
  }

  void uncoarsenImpl(IRefiner& refiner) final {
    double current_imbalance = metrics::imbalance(_hg);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
    initializeRefiner(refiner);
    std::vector<HypernodeID> refinement_nodes;
    refinement_nodes.reserve(_hg.initialNumNodes());
    size_t num_refinement_nodes = 0;
    while (!_history.empty()) {
      num_refinement_nodes = 0;
      restoreParallelHyperedges(_history.top());
      restoreSingleNodeHyperedges(_history.top());
      performUncontraction(_history.top(), refinement_nodes, num_refinement_nodes);
      performLocalSearch(refiner, refinement_nodes, num_refinement_nodes,
                         current_imbalance, current_cut);
      _history.pop();
    }
    ASSERT(current_imbalance <= _config.partitioning.epsilon,
           "balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg)
           << " > " << _config.partitioning.epsilon);
  }

  void removeHyperedgeFromPQ(HyperedgeID he) {
    if (_pq.contains(he)) {
      _pq.remove(he);
    }
  }

  std::string policyStringImpl() const final {
    return std::string(" ratingFunction=" + templateToString<RatingPolicy>());
  }

  private:
  FRIEND_TEST(AHyperedgeCoarsener, RemembersMementosOfNodeContractionsDuringOneCoarseningStep);
  FRIEND_TEST(AHyperedgeCoarsener, DoesNotEnqueueHyperedgesThatWouldViolateThresholdNodeWeight);
  FRIEND_TEST(HyperedgeCoarsener, DeleteRemovedSingleNodeHyperedgesFromPQ);
  FRIEND_TEST(HyperedgeCoarsener, DeleteRemovedParallelHyperedgesFromPQ);
  FRIEND_TEST(AHyperedgeCoarsener, UpdatesRatingsOfIncidentHyperedgesOfRepresentativeAfterContraction);
  FRIEND_TEST(AHyperedgeCoarsener, RemovesHyperedgesThatWouldViolateThresholdNodeWeightFromPQonUpdate);
  FRIEND_TEST(HyperedgeCoarsener, AddRepresentativeOnlyOnceToRefinementNodes);

  void rateAllHyperedges() {
    std::vector<HyperedgeID> permutation;
    permutation.reserve(_hg.initialNumNodes());
    for (auto && he : _hg.edges()) {
      permutation.push_back(he);
    }
    Randomize::shuffleVector(permutation, permutation.size());

    Rating rating;
    for (auto && he : permutation) {
      rating = RatingPolicy::rate(he, _hg, _config.coarsening.threshold_node_weight);
      if (rating.valid) {
        // HEs that would violate node_weight_treshold are not inserted
        // since their rating is set to invalid!
        DBG(dbg_coarsening_rating, "Inserting HE " << he << " rating=" << rating.value);
        _pq.insert(he, rating.value);
      }
    }
  }

  void reRateHyperedgesAffectedByContraction(HypernodeID representative) {
    Rating rating;
    for (auto && he : _hg.incidentEdges(representative)) {
      DBG(false, "Looking at HE " << he);
      if (_pq.contains(he)) {
        rating = RatingPolicy::rate(he, _hg, _config.coarsening.threshold_node_weight);
        if (rating.valid) {
          DBG(false, "Updating HE " << he << " rating=" << rating.value);
          _pq.updateKey(he, rating.value);
        } else {
          _pq.remove(he);
        }
      }
    }
  }

  HypernodeID performContraction(HyperedgeID he) {
    _history.emplace(HyperedgeCoarseningMemento());
    _history.top().mementos_begin = _contraction_mementos.size();
    IncidenceIterator pins_begin = _hg.pins(he).begin();
    IncidenceIterator pins_end = _hg.pins(he).end();
    HypernodeID representative = *pins_begin;
    ++pins_begin;
    //TODO(schlag): modify hypergraph DS such that we can do index-based iteration over incidence array
    //              via custom iterator (so that we do not need to collect the IDs to contract upfront).
    std::vector<HypernodeID> hns_to_contract;
    while (pins_begin != pins_end) {
      hns_to_contract.push_back(*pins_begin);
      ++pins_begin;
    }
    for (auto && hn_to_contract : hns_to_contract) {
      DBG(dbg_coarsening_coarsen, "Contracting (" << representative << "," << hn_to_contract
          << ") from HE " << he);
      _contraction_mementos.push_back(_hg.contract(representative, hn_to_contract));
      ++_history.top().mementos_size;
    }
    return representative;
  }

  void performUncontraction(const HyperedgeCoarseningMemento& memento,
                            std::vector<HypernodeID>& refinement_nodes,
                            size_t& num_refinement_nodes) {
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

  PriorityQueue<HyperedgeID, RatingType, MetaKeyDouble> _pq;
  std::vector<ContractionMemento> _contraction_mementos;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_
