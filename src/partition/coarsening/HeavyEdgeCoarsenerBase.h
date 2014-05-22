/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_
#define SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_

#include <algorithm>
#include <stack>
#include <unordered_map>
#include <vector>

#include "lib/datastructure/Hypergraph.h"
#include "lib/datastructure/PriorityQueue.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/coarsening/CoarsenerBase.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/IRefiner.h"

using datastructure::PriorityQueue;
using datastructure::MetaKeyDouble;
using datastructure::HypergraphType;
using datastructure::HypernodeID;
using datastructure::HyperedgeID;
using datastructure::HypernodeWeight;
using datastructure::HyperedgeWeight;
using datastructure::IncidenceIterator;
using datastructure::HypernodeIterator;

namespace partition {
struct CoarseningMemento {
  typedef typename HypergraphType::ContractionMemento Memento;

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

template <class Rater>
class HeavyEdgeCoarsenerBase : public CoarsenerBase<HeavyEdgeCoarsenerBase<Rater>,
                                                    CoarseningMemento>{
  protected:
  typedef typename Rater::Rating Rating;
  typedef typename Rater::RatingType RatingType;
  typedef CoarsenerBase<HeavyEdgeCoarsenerBase<Rater>, CoarseningMemento> Base;

  using Base::_hg;
  using Base::_config;
  using Base::_history;
#ifdef USE_BUCKET_PQ
  using Base::_weights_table;
#endif
  using Base::removeSingleNodeHyperedges;
  using Base::removeParallelHyperedges;
  using Base::restoreParallelHyperedges;
  using Base::restoreSingleNodeHyperedges;
  using Base::performLocalSearch;
  using Base::initializeRefiner;

  public:
  HeavyEdgeCoarsenerBase(HypergraphType& hypergraph, const Configuration& config) :
    Base(hypergraph, config),
    _rater(_hg, _config),
    _pq(_hg.initialNumNodes(), _hg.initialNumNodes())
  { }

  virtual ~HeavyEdgeCoarsenerBase() { }

  // Nothing to do in case of a hypernode-based coarsener
  // CRTP for hyperedge-based coarseners that need to delete single-node HEs and
  // parallel HEs from their PQ
  void removeHyperedgeFromPQ(HyperedgeID) { }

  protected:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void performContraction(HypernodeID rep_node, HypernodeID contracted_node) {
    _history.emplace(_hg.contract(rep_node, contracted_node));
  }

  void uncoarsen(IRefiner& refiner) {
    initializeRefiner(refiner);

    GPERF_START_PROFILER("/home/schlag/repo/schlag_git/profile/src/application/test.prof");
    while (!_history.empty()) {
      restoreParallelHyperedges(_history.top());
      restoreSingleNodeHyperedges(_history.top());

      DBG(dbg_coarsening_uncoarsen, "Uncontracting: (" << _history.top().contraction_memento.u << ","
          << _history.top().contraction_memento.v << ")");
      _hg.uncontract(_history.top().contraction_memento);

      performLocalSearch(refiner);
      _history.pop();
    }
    ASSERT(metrics::imbalance(_hg) <= _config.partitioning.epsilon,
           "balance_constraint is violated after uncontraction:" << metrics::imbalance(_hg)
           << " > " << _config.partitioning.epsilon);
    GPERF_STOP_PROFILER();
  }

  template <typename Map>
  void rateAllHypernodes(std::vector<HypernodeID>& target, Map& sources) {
    std::vector<HypernodeID> permutation;
    createHypernodePermutation(permutation);
    Rating rating;
    for (int i = 0; i < permutation.size(); ++i) {
      rating = _rater.rate(permutation[i]);
      if (rating.valid) {
        _pq.insert(permutation[i], rating.value);
        target[permutation[i]] = rating.target;
        sources.insert({ rating.target, permutation[i] });
      }
    }
  }

  void createHypernodePermutation(std::vector<HypernodeID>& permutation) {
    permutation.reserve(_hg.initialNumNodes());
    for (int i = 0; i < _hg.initialNumNodes(); ++i) {
      permutation.push_back(i);
    }
    Randomize::shuffleVector(permutation);
  }

  Rater _rater;
  PriorityQueue<HypernodeID, RatingType, MetaKeyDouble> _pq;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_
