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
class HeavyEdgeCoarsenerBase : private CoarsenerBase<CoarseningMemento>{
  protected:
  typedef typename Rater::Rating Rating;
  typedef typename Rater::RatingType RatingType;

  using CoarsenerBase::_hg;
  using CoarsenerBase::_history;
  using CoarsenerBase::removeSingleNodeHyperedges;
  using CoarsenerBase::removeParallelHyperedges;
  using CoarsenerBase::_removed_parallel_hyperedges;
  using CoarsenerBase::_removed_single_node_hyperedges;

  public:
  HeavyEdgeCoarsenerBase(HypergraphType& hypergraph, const Configuration& config) :
    CoarsenerBase<CoarseningMemento>(hypergraph, config),
    _rater(_hg, _config),
    _pq(_hg.initialNumNodes(), _hg.initialNumNodes())
  { }

  virtual ~HeavyEdgeCoarsenerBase() { }

  protected:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void performContraction(HypernodeID rep_node, HypernodeID contracted_node) {
    _history.emplace(_hg.contract(rep_node, contracted_node));
  }

  void uncoarsen(IRefiner& refiner) {
    double current_imbalance = metrics::imbalance(_hg);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);

#ifdef USE_BUCKET_PQ
    HyperedgeWeight max_single_he_induced_weight = 0;
    for (auto iter = _weights_table.begin(); iter != _weights_table.end(); ++iter) {
      if (iter->second > max_single_he_induced_weight) {
        max_single_he_induced_weight = iter->second;
      }
    }
    HyperedgeWeight max_degree = 0;
    HypernodeID max_node = 0;
    forall_hypernodes(hn, _hg) {
      ASSERT(_hg.partitionIndex(*hn) != INVALID_PARTITION,
             "TwoWayFmRefiner cannot work with HNs in invalid partition");
      HyperedgeWeight curr_degree = 0;
      forall_incident_hyperedges(he, *hn, _hg) {
        curr_degree += _hg.edgeWeight(*he);
      } endfor
      if (curr_degree > max_degree) {
        max_degree = curr_degree;
        max_node = *hn;
      }
    } endfor

      DBG(true, "max_single_he_induced_weight=" << max_single_he_induced_weight);
    DBG(true, "max_degree=" << max_degree << ", HN=" << max_node);
    refiner.initialize(max_degree + max_single_he_induced_weight);
#else
    refiner.initialize(0);
#endif

    double old_imbalance = current_imbalance;
    HyperedgeWeight old_cut = current_cut;
    GPERF_START_PROFILER("/home/schlag/repo/schlag_git/profile/src/application/test.prof");
    while (!_history.empty()) {
      restoreParallelHyperedges(_history.top());
      restoreSingleNodeHyperedges(_history.top());

      DBG(dbg_coarsening_uncoarsen, "Uncontracting: (" << _history.top().contraction_memento.u << ","
          << _history.top().contraction_memento.v << ")");

      _hg.uncontract(_history.top().contraction_memento);

      int iteration = 0;
      do {
        old_imbalance = current_imbalance;
        old_cut = current_cut;
        refiner.refine(_history.top().contraction_memento.u, _history.top().contraction_memento.v,
                       current_cut, _config.partitioning.epsilon, current_imbalance);

        ASSERT(current_cut <= old_cut, "Cut increased during uncontraction");
        DBG(dbg_coarsening_uncoarsen, "Iteration " << iteration << ": " << old_cut << "-->"
            << current_cut);
        ++iteration;
      } while ((iteration < refiner.numRepetitions()) &&
               (improvedCutWithinBalance(old_cut, current_cut, current_imbalance) ||
                improvedOldImbalanceTowardsValidSolution(old_imbalance, current_imbalance)));
      _history.pop();
    }
    ASSERT(current_imbalance <= _config.partitioning.epsilon,
           "balance_constraint is violated after uncontraction:" << current_imbalance
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
