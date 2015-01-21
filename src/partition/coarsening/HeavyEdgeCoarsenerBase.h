/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_
#define SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_

#include <algorithm>
#include <stack>
#include <unordered_map>
#include <vector>

#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/IRefiner.h"

using external::NoDataBinaryMaxHeap;
using datastructure::PriorityQueue;
using datastructure::MetaKeyDouble;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using defs::IncidenceIterator;
using defs::HypernodeIterator;

namespace partition {
struct CoarseningMemento {
  typedef typename Hypergraph::ContractionMemento Memento;

  int one_pin_hes_begin;        // start of removed single pin hyperedges
  int one_pin_hes_size;         // # removed single pin hyperedges
  int parallel_hes_begin;       // start of removed parallel hyperedges
  int parallel_hes_size;        // # removed parallel hyperedges
  Memento contraction_memento;
  explicit CoarseningMemento(Memento contraction_memento_) :
    one_pin_hes_begin(0),
    one_pin_hes_size(0),
    parallel_hes_begin(0),
    parallel_hes_size(0),
    contraction_memento(contraction_memento_) { }
};

template <class Rater = Mandatory,
          class PrioQueue = PriorityQueue<
            NoDataBinaryMaxHeap<HypernodeID,
                                typename Rater::RatingType,
                                MetaKeyDouble> > >
class HeavyEdgeCoarsenerBase : public CoarsenerBase<CoarseningMemento>{
  protected:
  typedef typename Rater::Rating Rating;
  typedef typename Rater::RatingType RatingType;
  typedef CoarsenerBase<CoarseningMemento> Base;

  using Base::_hg;
  using Base::_config;
  using Base::_history;
  using Base::_stats;
#ifdef USE_BUCKET_PQ
  using Base::_weights_table;
#endif
  using Base::restoreSingleNodeHyperedges;
  using Base::restoreParallelHyperedges;
  using Base::performLocalSearch;
  using Base::initializeRefiner;

  public:
  HeavyEdgeCoarsenerBase(Hypergraph& hypergraph, const Configuration& config) :
    Base(hypergraph, config),
    _rater(_hg, _config),
    _pq(_hg.initialNumNodes())
  { }

  virtual ~HeavyEdgeCoarsenerBase() { }

  protected:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void performContraction(const HypernodeID rep_node, const HypernodeID contracted_node) {
    _history.emplace(_hg.contract(rep_node, contracted_node));
  }

  bool doUncoarsen(IRefiner& refiner) {
    double current_imbalance = metrics::imbalance(_hg);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
    const HyperedgeWeight initial_cut = current_cut;

    _stats.add("initialCut", _config.partition.current_v_cycle, initial_cut);
    _stats.add("initialImbalance", _config.partition.current_v_cycle, current_imbalance);
    DBG(true, "initial cut =" << current_cut);
    DBG(true, "initial imbalance=" << current_imbalance);

    initializeRefiner(refiner);
    std::vector<HypernodeID> refinement_nodes(2, 0);

    while (!_history.empty()) {
      restoreParallelHyperedges();
      restoreSingleNodeHyperedges();

      DBG(dbg_coarsening_uncoarsen, "Uncontracting: (" << _history.top().contraction_memento.u << ","
          << _history.top().contraction_memento.v << ")");
      _hg.uncontract(_history.top().contraction_memento);
      refinement_nodes[0] = _history.top().contraction_memento.u;
      refinement_nodes[1] = _history.top().contraction_memento.v;
      performLocalSearch(refiner, refinement_nodes, 2, current_imbalance, current_cut);
      _history.pop();
    }
    return current_cut < initial_cut;
    // ASSERT(current_imbalance <= _config.partition.epsilon,
    //        "balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg)
    //        << " > " << _config.partition.epsilon);
  }

  template <typename Map>
  void rateAllHypernodes(std::vector<HypernodeID>& target, Map& sources) {
    std::vector<HypernodeID> permutation;
    createHypernodePermutation(permutation);
    for (size_t i = 0; i < permutation.size(); ++i) {
      const Rating rating = _rater.rate(permutation[i]);
      if (rating.valid) {
        _pq.insert(permutation[i], rating.value);
        target[permutation[i]] = rating.target;
        sources.insert({ rating.target, permutation[i] });
      }
    }
  }

  void createHypernodePermutation(std::vector<HypernodeID>& permutation) {
    permutation.reserve(_hg.initialNumNodes());
    for (HypernodeID hn : _hg.nodes()) {
      permutation.push_back(hn);
    }
    Randomize::shuffleVector(permutation, permutation.size());
  }

  Rater _rater;
  PrioQueue _pq;

  private:
  DISALLOW_COPY_AND_ASSIGN(HeavyEdgeCoarsenerBase);
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_
