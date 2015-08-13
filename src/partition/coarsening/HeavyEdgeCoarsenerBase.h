/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_
#define SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_

#include <algorithm>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>

#include "lib/core/Mandatory.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/IRefiner.h"

using datastructure::NoDataBinaryMaxHeap;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;

namespace partition {
struct CoarseningMemento {
  int one_pin_hes_begin;        // start of removed single pin hyperedges
  int one_pin_hes_size;         // # removed single pin hyperedges
  int parallel_hes_begin;       // start of removed parallel hyperedges
  int parallel_hes_size;        // # removed parallel hyperedges
  Hypergraph::ContractionMemento contraction_memento;
  explicit CoarseningMemento(Hypergraph::ContractionMemento&& contraction_memento_) noexcept :
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    contraction_memento(std::move(contraction_memento_)) { }
};

template <class Rater = Mandatory,
          class PrioQueue = NoDataBinaryMaxHeap<HypernodeID,
                                                typename Rater::RatingType> >
class HeavyEdgeCoarsenerBase : public CoarsenerBase<CoarseningMemento>{
 protected:
  using Base = CoarsenerBase<CoarseningMemento>;
  using Base::_hg;
  using Base::_config;
  using Base::_history;
  using Base::_max_hn_weights;
  using Base::CurrentMaxNodeWeight;
  using Base::restoreSingleNodeHyperedges;
  using Base::restoreParallelHyperedges;
  using Base::performLocalSearch;
  using Base::initializeRefiner;
  using Rating = typename Rater::Rating;
  using RatingType = typename Rater::RatingType;

 public:
  HeavyEdgeCoarsenerBase(Hypergraph& hypergraph, const Configuration& config,
                         const HypernodeWeight weight_of_heaviest_node) noexcept :
    Base(hypergraph, config, weight_of_heaviest_node),
    _rater(_hg, _config),
    _pq(_hg.initialNumNodes()) { }

  ~HeavyEdgeCoarsenerBase() { }

  HeavyEdgeCoarsenerBase(const HeavyEdgeCoarsenerBase&) = delete;
  HeavyEdgeCoarsenerBase& operator= (const HeavyEdgeCoarsenerBase&) = delete;

  HeavyEdgeCoarsenerBase(HeavyEdgeCoarsenerBase&&) = delete;
  HeavyEdgeCoarsenerBase& operator= (HeavyEdgeCoarsenerBase&&) = delete;

 protected:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void performContraction(const HypernodeID rep_node, const HypernodeID contracted_node) noexcept {
    _history.emplace_back(_hg.contract(rep_node, contracted_node));
    if (_hg.nodeWeight(rep_node) > _max_hn_weights.back().max_weight) {
      _max_hn_weights.emplace_back(_hg.numNodes(), _hg.nodeWeight(rep_node));
    }
  }

  bool doUncoarsen(IRefiner& refiner) noexcept {
    double current_imbalance = metrics::imbalance(_hg, _config);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
    const HyperedgeWeight initial_cut = current_cut;

    Stats::instance().add(_config, "initialCut", initial_cut);
    Stats::instance().add(_config, "initialImbalance", current_imbalance);
    LOG("initial cut =" << current_cut);
    LOG("initial imbalance=" << current_imbalance);
    LOG("target  weights (RB): w(0)=" << _config.partition.max_part_weights[0]
        << " w(1)=" << _config.partition.max_part_weights[1]);
    LOG("initial weights (RB): w(0)=" << _hg.partWeight(0) << " w(1)=" << _hg.partWeight(1));

    initializeRefiner(refiner);
    std::vector<HypernodeID> refinement_nodes(2, 0);

    while (!_history.empty()) {
      restoreParallelHyperedges();
      restoreSingleNodeHyperedges();

      DBG(dbg_coarsening_uncoarsen, "Uncontracting: (" << _history.back().contraction_memento.u << ","
          << _history.back().contraction_memento.v << ")");
      const std::pair<HyperedgeWeight, HyperedgeWeight> changes = _hg.uncontract(_history.back().contraction_memento);
      refinement_nodes[0] = _history.back().contraction_memento.u;
      refinement_nodes[1] = _history.back().contraction_memento.v;

      if (_hg.numNodes() > _max_hn_weights.back().num_nodes) {
        _max_hn_weights.pop_back();
      }
      ASSERT([&]() {
          for (const HypernodeID hn : _hg.nodes()) {
            if (_hg.nodeWeight(hn) == _max_hn_weights.back().max_weight) {
              return true;
            }
          }
          return false;
        } (), "No HN of weight " << _max_hn_weights.back().max_weight << " found");

      performLocalSearch(refiner, refinement_nodes, 2, current_imbalance, current_cut, changes);
      _history.pop_back();
    }

    // This currently cannot be guaranteed for RB-partitioning and k != 2^x, since it might be
    // possible that 2FM cannot re-adjust the part weights to be less than Lmax0 and Lmax1.
    // In order to guarantee this, 2FM would have to force rebalancing by sacrificing cut-edges.
    // ASSERT(current_imbalance <= _config.partition.epsilon,
    //        "balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg, _config)
    //        << " > " << _config.partition.epsilon);
    Stats::instance().add(_config, "finalCut", current_cut);
    Stats::instance().add(_config, "finaleImbalance", current_imbalance);
    LOG("final cut: " << current_cut);
    LOG("final imbalance: " << current_imbalance);
    LOG("final weights (RB):   w(0)=" << _hg.partWeight(0) << " w(1)=" << _hg.partWeight(1));
    return current_cut < initial_cut;
  }

  template <typename Map>
  void rateAllHypernodes(std::vector<HypernodeID>& target, Map& sources) noexcept {
    std::vector<HypernodeID> permutation;
    createHypernodePermutation(permutation);
    for (size_t i = 0; i < permutation.size(); ++i) {
      const Rating rating = _rater.rate(permutation[i]);
      if (rating.valid) {
        _pq.push(permutation[i], rating.value);
        target[permutation[i]] = rating.target;
        sources.insert({ rating.target, permutation[i] });
      }
    }
  }

  void createHypernodePermutation(std::vector<HypernodeID>& permutation) noexcept {
    permutation.reserve(_hg.initialNumNodes());
    for (HypernodeID hn : _hg.nodes()) {
      permutation.push_back(hn);
    }
    Randomize::shuffleVector(permutation, permutation.size());
  }

  Rater _rater;
  PrioQueue _pq;
};
}  // namespace partition

#endif  // SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_
