/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_
#define SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_

#include <vector>

#include "lib/datastructure/Hypergraph.h"
#include "lib/datastructure/PriorityQueue.h"
#include "partition/Configuration.h"
#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using datastructure::HypergraphType;
using datastructure::PriorityQueue;
using datastructure::MetaKeyDouble;
using datastructure::HypernodeID;
using datastructure::HyperedgeID;

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

template <class Rater>
class HyperedgeCoarsener : public ICoarsener,
                           public CoarsenerBase<HyperedgeCoarsener<Rater>,
                                                HyperedgeCoarseningMemento>{
  private:
  typedef typename Rater::Rating Rating;
  typedef typename Rater::RatingType RatingType;
  typedef typename HypergraphType::ContractionMemento ContractionMemento;
  typedef CoarsenerBase<HyperedgeCoarsener<Rater>, HyperedgeCoarseningMemento> Base;
  bool dbg_coarsening_coarsen = true;

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

  HyperedgeCoarsener(HypergraphType& hypergraph, const Configuration& config) :
    Base(hypergraph, config),
    _rater(),
    _pq(_hg.initialNumEdges()),
    _contraction_mementos() { }

  void coarsen(int limit) {
    _pq.clear();
    rateAllHyperedges();

    HyperedgeID he_to_contract;
    while (!_pq.empty() && _hg.numNodes() > limit) {
      he_to_contract = _pq.max();
      DBG(dbg_coarsening_coarsen, "Contracting HE" << he_to_contract << " prio: " << _pq.maxKey());

      ASSERT([&]() {
               HypernodeWeight total_weight = 0;
               forall_pins(pin, he_to_contract, _hg) {
                 total_weight += _hg.nodeWeight(*pin);
               } endfor
               return total_weight;
             } () <= _config.coarsening.threshold_node_weight,
             "Contracting HE " << he_to_contract << "leads to violation of node weight thsreshold");
      ASSERT(_pq.maxKey() == _rater.rate(he_to_contract, _hg,
                                         _config.coarsening.threshold_node_weight).value,
             "Key in PQ != rating calculated by rater:" << _pq.maxKey() << "!="
             << _rater.rate(he_to_contract, _hg, _config.coarsening.threshold_node_weight).value);

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

  void uncoarsen(IRefiner& refiner) {
    initializeRefiner(refiner);
    std::vector<HypernodeID> refinement_nodes;
    refinement_nodes.reserve(_hg.initialNumNodes());
    size_t num_refinement_nodes = 0;
    while (!_history.empty()) {
      restoreParallelHyperedges(_history.top());
      restoreSingleNodeHyperedges(_history.top());
      performUncontraction(_history.top(), refinement_nodes, num_refinement_nodes);
      performLocalSearch(refiner, refinement_nodes, num_refinement_nodes);
      _history.pop();
    }
  }

  void removeHyperedgeFromPQ(HyperedgeID he) {
    if (_pq.contains(he)) {
      _pq.remove(he);
    }
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
    forall_hyperedges(he, _hg) {
      permutation.push_back(*he);
    } endfor
    Randomize::shuffleVector(permutation);

    Rating rating;
    for (auto he : permutation) {
      rating = _rater.rate(he, _hg, _config.coarsening.threshold_node_weight);
      if (rating.valid) {
        // HEs that would violate node_weight_treshold are not inserted
        // since their rating is set to invalid!
        DBG(true, "Inserting HE " << he << " rating=" << rating.value);
        _pq.insert(he, rating.value);
      }
    }
  }

  void reRateHyperedgesAffectedByContraction(HypernodeID representative) {
    Rating rating;
    forall_incident_hyperedges(he, representative, _hg) {
      DBG(true, "Looking at HE " << *he);
      if (_pq.contains(*he)) {
        rating = _rater.rate(*he, _hg, _config.coarsening.threshold_node_weight);
        if (rating.valid) {
          DBG(true, "Updating HE " << *he << " rating=" << rating.value);
          _pq.updateKey(*he, rating.value);
        } else {
          _pq.remove(*he);
        }
      }
    } endfor
  }

  HypernodeID performContraction(HyperedgeID he) {
    _history.emplace(HyperedgeCoarseningMemento());
    _history.top().mementos_begin = _contraction_mementos.size();
    IncidenceIterator pins_begin, pins_end;
    std::tie(pins_begin, pins_end) = _hg.pins(he);
    HypernodeID representative = *pins_begin;
    ++pins_begin;
    //TODO(schlag): modify hypergraph DS such that we can do index-based iteration over incidence array
    //              via custom iterator (so that we do not need to collect the IDs to contract upfront).
    std::vector<HypernodeID> hns_to_contract;
    while (pins_begin != pins_end) {
      hns_to_contract.push_back(*pins_begin);
      ++pins_begin;
    }
    for (auto hn_to_contract : hns_to_contract) {
      DBG(true, "Contracting (" << representative << "," << hn_to_contract << ") from HE " << he);
      _contraction_mementos.push_back(_hg.contract(representative, hn_to_contract));
      ++_history.top().mementos_size;
    }
    return representative;
  }

  void performUncontraction(const HyperedgeCoarseningMemento& memento,
                            std::vector<HypernodeID>& refinement_nodes,
                            size_t& num_refinement_nodes) {
    num_refinement_nodes = 0;
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

  Rater _rater;
  PriorityQueue<HyperedgeID, RatingType, MetaKeyDouble> _pq;
  std::vector<ContractionMemento> _contraction_mementos;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HYPEREDGECOARSENER_H_
