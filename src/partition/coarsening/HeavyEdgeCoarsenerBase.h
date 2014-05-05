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
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/TwoWayFMRefiner.h"

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
static const bool dbg_coarsening_coarsen = false;
static const bool dbg_coarsening_uncoarsen = false;
static const bool dbg_coarsening_uncoarsen_improvement = false;
static const bool dbg_coarsening_single_node_he_removal = false;
static const bool dbg_coarsening_parallel_he_removal = false;

template <class Rater>
class HeavyEdgeCoarsenerBase : public ICoarsener {
  protected:
  typedef typename HypergraphType::ContractionMemento Memento;
  typedef typename Rater::Rating HeavyEdgeRating;
  typedef typename Rater::RatingType RatingType;

  struct CoarseningMemento {
    int one_pin_hes_begin;      // start of removed single pin hyperedges
    int one_pin_hes_size;       // # removed single pin hyperedges
    int parallel_hes_begin;     // start of removed parallel hyperedges
    int parallel_hes_size;      // # removed parallel hyperedges
    Memento contraction_memento;
    explicit CoarseningMemento(Memento contraction_memento_) :
      one_pin_hes_begin(0),
      one_pin_hes_size(0),
      parallel_hes_begin(0),
      parallel_hes_size(0),
      contraction_memento(contraction_memento_) { }
  };

  struct Fingerprint {
    HyperedgeID id;
    HyperedgeID hash;
    HypernodeID size;
    Fingerprint(HyperedgeID id_, HyperedgeID hash_, HypernodeID size_) :
      id(id_),
      hash(hash_),
      size(size_) { }
  };

  struct ParallelHE {
    HyperedgeID representative_id;
    HyperedgeID removed_id;
    ParallelHE(HyperedgeID representative_id_, HyperedgeID removed_id_) :
      representative_id(representative_id_),
      removed_id(removed_id_) { }
  };

  public:
  HeavyEdgeCoarsenerBase(HypergraphType& hypergraph, const Configuration& config) :
    _hg(hypergraph),
    _config(config),
    _rater(_hg, _config),
    _history(),
    _removed_single_node_hyperedges(),
    _removed_parallel_hyperedges(),
    _fingerprints(),
    _contained_hypernodes(_hg.initialNumNodes()),
    _pq(_hg.initialNumNodes(), _hg.initialNumNodes()) { }

  virtual ~HeavyEdgeCoarsenerBase() { }

  void uncoarsen(IRefiner& refiner) {
    double current_imbalance = metrics::imbalance(_hg);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);

    refiner.initialize();

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

  protected:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void performContraction(HypernodeID rep_node, HypernodeID contracted_node) {
    _history.emplace(_hg.contract(rep_node, contracted_node));
  }

  bool improvedCutWithinBalance(HyperedgeWeight old_cut, HyperedgeWeight current_cut,
                                double current_imbalance) {
    DBG(dbg_coarsening_uncoarsen_improvement && (current_cut < old_cut) &&
        (current_imbalance < _config.partitioning.epsilon),
        "improved cut: " << old_cut << "-->" << current_cut);
    return (current_cut < old_cut) && (current_imbalance < _config.partitioning.epsilon);
  }

  bool improvedOldImbalanceTowardsValidSolution(double old_imbalance, double current_imbalance) {
    DBG(dbg_coarsening_uncoarsen_improvement && (old_imbalance > _config.partitioning.epsilon) &&
        (current_imbalance < old_imbalance), "improved imbalance: " << old_imbalance << "-->"
        << current_imbalance);
    return (old_imbalance > _config.partitioning.epsilon) && (current_imbalance < old_imbalance);
  }

  void restoreSingleNodeHyperedges(const CoarseningMemento& memento) {
    for (int i = memento.one_pin_hes_begin + memento.one_pin_hes_size - 1;
         i >= memento.one_pin_hes_begin; --i) {
      ASSERT(i < _removed_single_node_hyperedges.size(), "Index out of bounds");
      DBG(dbg_coarsening_single_node_he_removal, "restore single-node HE "
          << _removed_single_node_hyperedges[i]);
      _hg.restoreEdge(_removed_single_node_hyperedges[i]);
      _removed_single_node_hyperedges.pop_back();
    }
  }

  void restoreParallelHyperedges(const CoarseningMemento& memento) {
    for (int i = memento.parallel_hes_begin + memento.parallel_hes_size - 1;
         i >= memento.parallel_hes_begin; --i) {
      ASSERT(i < _removed_parallel_hyperedges.size(), "Index out of bounds");
      DBG(dbg_coarsening_parallel_he_removal, "restore HE "
          << _removed_parallel_hyperedges[i].removed_id << " which is parallel to "
          << _removed_parallel_hyperedges[i].representative_id);
      _hg.restoreEdge(_removed_parallel_hyperedges[i].removed_id);
      _hg.setEdgeWeight(_removed_parallel_hyperedges[i].representative_id,
                        _hg.edgeWeight(_removed_parallel_hyperedges[i].representative_id) -
                        _hg.edgeWeight(_removed_parallel_hyperedges[i].removed_id));
      _removed_parallel_hyperedges.pop_back();
    }
  }

  void removeSingleNodeHyperedges(HypernodeID u) {
    ASSERT(_history.top().contraction_memento.u == u,
           "Current coarsening memento does not belong to hypernode" << u);
    _history.top().one_pin_hes_begin = _removed_single_node_hyperedges.size();
    IncidenceIterator begin, end;
    std::tie(begin, end) = _hg.incidentEdges(u);
    for (IncidenceIterator he_it = begin; he_it != end; ++he_it) {
      if (_hg.edgeSize(*he_it) == 1) {
        _removed_single_node_hyperedges.push_back(*he_it);
        ++_history.top().one_pin_hes_size;
        DBG(dbg_coarsening_single_node_he_removal, "removing single-node HE " << *he_it);
        _hg.removeEdge(*he_it, false);
        --he_it;
        --end;
      }
    }
  }

  void removeParallelHyperedges(HypernodeID u) {
    ASSERT(_history.top().contraction_memento.u == u,
           "Current coarsening memento does not belong to hypernode" << u);
    _history.top().parallel_hes_begin = _removed_parallel_hyperedges.size();

    createFingerprints(u);
    std::sort(_fingerprints.begin(), _fingerprints.end(),
              [](const Fingerprint& a, const Fingerprint& b) { return a.hash < b.hash; });

    for (size_t i = 0; i < _fingerprints.size(); ++i) {
      size_t j = i + 1;
      if (j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash) {
        fillProbeBitset(_fingerprints[i].id);
        for ( ; j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash; ++j) {
          if (_fingerprints[i].size == _fingerprints[j].size &&
              isParallelHyperedge(_fingerprints[j].id)) {
            removeParallelHyperedge(_fingerprints[i].id, _fingerprints[j].id);
          }
        }
        i = j;
      }
    }
  }

  bool isParallelHyperedge(HyperedgeID he) const {
    bool is_parallel = true;
    forall_pins(pin, he, _hg) {
      if (!_contained_hypernodes[*pin]) {
        is_parallel = false;
        break;
      }
    } endfor
    return is_parallel;
  }

  void fillProbeBitset(HyperedgeID he) {
    _contained_hypernodes.reset();
    forall_pins(pin, he, _hg) {
      _contained_hypernodes[*pin] = 1;
    } endfor
  }

  void removeParallelHyperedge(HyperedgeID representative, HyperedgeID to_remove) {
    _hg.setEdgeWeight(representative,
                      _hg.edgeWeight(representative)
                      + _hg.edgeWeight(to_remove));
    _hg.removeEdge(to_remove, false);
    _removed_parallel_hyperedges.emplace_back(representative, to_remove);
    DBG(dbg_coarsening_parallel_he_removal, "removed HE " << to_remove << " which was parallel to "
        << representative);
    ++_history.top().parallel_hes_size;
  }

  void createFingerprints(HypernodeID u) {
    _fingerprints.clear();
    forall_incident_hyperedges(he, u, _hg) {
      HyperedgeID hash = /* seed */ 42;
      forall_pins(pin, *he, _hg) {
        hash ^= *pin;
      } endfor
      _fingerprints.emplace_back(*he, hash, _hg.edgeSize(*he));
    } endfor
  }

  template <typename Map>
  void rateAllHypernodes(std::vector<HypernodeID>& target, Map& sources) {
    std::vector<HypernodeID> permutation;
    createHypernodePermutation(permutation);
    HeavyEdgeRating rating;
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

  HypergraphType& _hg;
  const Configuration& _config;
  Rater _rater;
  std::stack<CoarseningMemento> _history;
  std::vector<HyperedgeID> _removed_single_node_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  std::vector<Fingerprint> _fingerprints;
  boost::dynamic_bitset<uint64_t> _contained_hypernodes;
  PriorityQueue<HypernodeID, RatingType, MetaKeyDouble> _pq;
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HEAVYEDGECOARSENERBASE_H_
