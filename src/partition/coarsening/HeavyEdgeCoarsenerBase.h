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
#include "partition/coarsening/Rater.h"
#include "partition/refinement/TwoWayFMRefiner.h"

using datastructure::PriorityQueue;
using datastructure::MetaKeyDouble;

namespace partition {
template <class Hypergraph, class Rater>
class HeavyEdgeCoarsenerBase {
  protected:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeID HyperedgeID;
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;
  typedef typename Hypergraph::ContractionMemento Memento;
  typedef typename Hypergraph::IncidenceIterator IncidenceIterator;
  typedef typename Hypergraph::HypernodeIterator HypernodeIterator;
  typedef typename Rater::Rating HeavyEdgeRating;
  typedef typename Rater::RatingType RatingType;
  typedef std::unordered_multimap<HypernodeID, HypernodeID> TargetToSourcesMap;

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
  HeavyEdgeCoarsenerBase(Hypergraph& hypergraph, const Configuration<Hypergraph>& config) :
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

  void uncoarsen(IRefiner<Hypergraph>& refiner) {
    double current_imbalance = metrics::imbalance(_hg);
    HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);

    while (!_history.empty()) {
#ifndef NDEBUG
      HyperedgeWeight old_cut = current_cut;
#endif

      restoreParallelHyperedges(_history.top());
      restoreSingleNodeHyperedges(_history.top());

      // PRINT("Uncontracting: (" << _history.top().contraction_memento.u << ","
      //       << _history.top().contraction_memento.v << ")");

      _hg.uncontract(_history.top().contraction_memento);
      refiner.refine(_history.top().contraction_memento.u, _history.top().contraction_memento.v,
                     current_cut, _config.partitioning.epsilon, current_imbalance);
      _history.pop();

      ASSERT(current_cut <= old_cut, "Cut increased during uncontraction");
    }
    ASSERT(current_imbalance <= _config.partitioning.epsilon,
           "balance_constraint is violated after uncontraction:" << current_imbalance
                                                                 << " > " << _config.partitioning.epsilon);
  }

  protected:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);

  void performContraction(HypernodeID rep_node, HypernodeID contracted_node) {
    _history.emplace(_hg.contract(rep_node, contracted_node));
  }

  void restoreSingleNodeHyperedges(const CoarseningMemento& memento) {
    for (int i = memento.one_pin_hes_begin + memento.one_pin_hes_size - 1;
         i >= memento.one_pin_hes_begin; --i) {
      ASSERT(i < _removed_single_node_hyperedges.size(), "Index out of bounds");
      // PRINT("*** restore single-node HE " << _removed_single_node_hyperedges[i]);
      _hg.restoreEdge(_removed_single_node_hyperedges[i]);
    }
  }

  void restoreParallelHyperedges(const CoarseningMemento& memento) {
    for (int i = memento.parallel_hes_begin + memento.parallel_hes_size - 1;
         i >= memento.parallel_hes_begin; --i) {
      ASSERT(i < _removed_parallel_hyperedges.size(), "Index out of bounds");
      // PRINT("*** restore HE " << _removed_parallel_hyperedges[i].removed_id
      //       << " which is parallel to " << _removed_parallel_hyperedges[i].representative_id);
      _hg.restoreEdge(_removed_parallel_hyperedges[i].removed_id);
      _hg.setEdgeWeight(_removed_parallel_hyperedges[i].representative_id,
                        _hg.edgeWeight(_removed_parallel_hyperedges[i].representative_id) -
                        _hg.edgeWeight(_removed_parallel_hyperedges[i].removed_id));
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
        // PRINT("*** removing single-node HE " << *he_it);
        // _hg.printEdgeState(*he_it);
        _hg.removeEdge(*he_it);
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
              [] (const Fingerprint &a, const Fingerprint &b) { return a.hash < b.hash; });

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
    _hg.removeEdge(to_remove);
    _removed_parallel_hyperedges.emplace_back(representative, to_remove);
    // PRINT("*** removed HE " << to_remove << " which was parallel to " << representative);
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

  Hypergraph& _hg;
  const Configuration<Hypergraph>& _config;
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
