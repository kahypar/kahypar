#ifndef PARTITION_COARSENER_H_
#define PARTITION_COARSENER_H_

#include <stack>

#include <boost/dynamic_bitset.hpp>

#include "Rater.h"
#include "../lib/datastructure/Hypergraph.h"
#include "../lib/datastructure/PriorityQueue.h"

using datastructure::PriorityQueue;

namespace partition {

template <class Hypergraph, class Rater>
class Coarsener{
 private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeID HyperedgeID;
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::ContractionMemento Memento;
  typedef typename Hypergraph::IncidenceIterator IncidenceIterator;
  typedef typename Hypergraph::HypernodeIterator HypernodeIterator;
  typedef typename Rater::Rating HeavyEdgeRating;
  typedef typename Rater::RatingType RatingType;
  
  struct CoarseningMemento {
    size_t one_pin_hes_begin;   // start of removed single pin hyperedges
    size_t one_pin_hes_size;    // # removed single pin hyperedges
    size_t parallel_hes_begin;  // start of removed parallel hyperedges
    size_t parallel_hes_size;   // # removed parallel hyperedges
    Memento contraction_memento;
    CoarseningMemento(Memento contraction_memento_) :
        one_pin_hes_begin(0),
        one_pin_hes_size(0),
        parallel_hes_begin(0),
        parallel_hes_size(0),
        contraction_memento(contraction_memento_) {}
  };

  struct Fingerprint {
    HyperedgeID id;
    HyperedgeID hash;
    HypernodeID size;
    Fingerprint(HyperedgeID id_, HyperedgeID hash_, HypernodeID size_) :
        id(id_),
        hash(hash_),
        size(size_) {}
  };

  struct ParallelHE {
    HyperedgeID representative_id;
    HyperedgeID removed_id;
    ParallelHE(HyperedgeID representative_id_, HyperedgeID removed_id_) :
        representative_id(representative_id_),
        removed_id(removed_id_) {}
  };
  
 public:  
  Coarsener(Hypergraph& hypergraph,  HypernodeWeight threshold_node_weight) :
      _hypergraph(hypergraph),
      _rater(_hypergraph, threshold_node_weight),
      _history(),
      removed_single_node_hyperedges(),
      _removed_parallel_hyperedges(),
      _fingerprints(),
      _contained_hypernodes(_hypergraph.initialNumNodes()),
      _pq(_hypergraph.initialNumNodes(), _hypergraph.initialNumNodes()) {}
  
  void coarsen(int limit) {
    ASSERT(_pq.empty(), "coarsen() can only be called once");
    HeavyEdgeRating rating;
    std::vector<HypernodeID> contraction_targets(_hypergraph.initialNumNodes());
    rateAllHypernodes(contraction_targets);

    HypernodeID rep_node;
    boost::dynamic_bitset<uint64_t> rerated_hypernodes(_hypergraph.initialNumNodes());
    while (!_pq.empty() && _hypergraph.numNodes() > limit) {
      rep_node = _pq.max();
      //PRINT("Contracting: (" << rep_node << ","
      //      << contraction_targets[rep_node] << ") prio: " << _pq.maxKey());

      performContraction(rep_node, contraction_targets);
      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);
      
      rating = _rater.rate(rep_node);
      rerated_hypernodes[rep_node] = 1;
      updatePQandContractionTargets(rep_node, rating, contraction_targets);

      reRateAffectedHypernodes(rep_node, contraction_targets, rerated_hypernodes);
    }
   
  }

  void uncoarsen() {
    while(!_history.empty()) {
      restoreParallelHyperedges(_history.top());
      restoreSingleNodeHyperedges(_history.top());
      _hypergraph.uncontract(_history.top().contraction_memento);
      _history.pop();
    }
  }

 private:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);
  
  void performContraction(HypernodeID rep_node, std::vector<HypernodeID>& contraction_targets) {
    _history.emplace(_hypergraph.contract(rep_node, contraction_targets[rep_node]));
    _pq.remove(contraction_targets[rep_node]);
  }

  void restoreSingleNodeHyperedges(const CoarseningMemento& memento) {
    for (size_t i = memento.one_pin_hes_begin;
         i < memento.one_pin_hes_begin + memento.one_pin_hes_size; ++i) {
      ASSERT(i < removed_single_node_hyperedges.size(), "Index out of bounds");
      // PRINT("*** restore single-node HE " << removed_single_node_hyperedges[i]);
      _hypergraph.restoreEdge(removed_single_node_hyperedges[i]);
    }
  }

  void restoreParallelHyperedges(const CoarseningMemento& memento) {
    for (size_t i = memento.parallel_hes_begin;
         i < memento.parallel_hes_begin + memento.parallel_hes_size; ++i) {
      ASSERT(i < _removed_parallel_hyperedges.size(), "Index out of bounds");
      // PRINT("*** restore HE " << _removed_parallel_hyperedges[i].removed_id
      //       << " which is parallel to " << _removed_parallel_hyperedges[i].representative_id);
      _hypergraph.restoreEdge(_removed_parallel_hyperedges[i].removed_id);
      _hypergraph.setEdgeWeight(_removed_parallel_hyperedges[i].representative_id,
                                _hypergraph.edgeWeight(_removed_parallel_hyperedges[i].representative_id) -
                                _hypergraph.edgeWeight(_removed_parallel_hyperedges[i].removed_id));
    }
  }

  void rateAllHypernodes(std::vector<HypernodeID>& contraction_targets) {
    HeavyEdgeRating rating;
    forall_hypernodes(hn, _hypergraph) {
      rating = _rater.rate(*hn);
      if (rating.valid) {
        _pq.insert(*hn, rating.value);
        contraction_targets[*hn] = rating.target;
      }
    } endfor    
  }

  void reRateAffectedHypernodes(HypernodeID rep_node,
                                std::vector<HypernodeID>& contraction_targets,
                                boost::dynamic_bitset<uint64_t>& rerated_hypernodes) {
    // ToDo: This can be done more fine grained if we know which HEs are affected: see p. 31
    HeavyEdgeRating rating;
    forall_incident_hyperedges(he, rep_node, _hypergraph) {
      forall_pins(pin, *he, _hypergraph) {
        if (!rerated_hypernodes[*pin]) {
          rating = _rater.rate(*pin);
          rerated_hypernodes[*pin] = 1;
          updatePQandContractionTargets(*pin, rating, contraction_targets);
        }
      } endfor
    } endfor
    rerated_hypernodes.reset();
  }

  void removeSingleNodeHyperedges(HypernodeID u) {
    ASSERT(_history.top().contraction_memento.u == u,
           "Current coarsening memento does not belong to hypernode" << u);
    _history.top().one_pin_hes_begin = removed_single_node_hyperedges.size();
    IncidenceIterator begin, end;
    std::tie(begin, end) = _hypergraph.incidentEdges(u);
    for (IncidenceIterator he_it = begin; he_it != end; ++he_it) {
      if (_hypergraph.edgeSize(*he_it) == 1) {
        removed_single_node_hyperedges.push_back(*he_it);
        ++_history.top().one_pin_hes_size;
        // PRINT("*** removing single-node HE " << *he_it);
        // _hypergraph.printEdgeState(*he_it);
        _hypergraph.removeEdge(*he_it);
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
              [](const Fingerprint& a, const Fingerprint& b) {return a.hash < b.hash;});

    for (size_t i = 0; i < _fingerprints.size(); ++i) {
      size_t j = i + 1;
      if (j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash) {
        fillProbeBitset(_fingerprints[i].id);
        for (; j < _fingerprints.size() && _fingerprints[i].hash == _fingerprints[j].hash; ++j) {
          if (_fingerprints[i].size == _fingerprints[j].size
              && isParallelHyperedge(_fingerprints[j].id)) {
              removeParallelHyperedge(_fingerprints[i].id, _fingerprints[j].id);
          }
        }
        i = j;
      }
    }
  }

  bool isParallelHyperedge(HyperedgeID he) const {
    bool is_parallel = true;
    forall_pins(pin, he, _hypergraph) {
      if (!_contained_hypernodes[*pin]) {
        is_parallel = false;
        break;
      }
    } endfor
    return is_parallel;
  }

  void fillProbeBitset(HyperedgeID he) {
    _contained_hypernodes.reset();
    forall_pins(pin, he, _hypergraph) {
      _contained_hypernodes[*pin] = 1;
    } endfor
  }

  void removeParallelHyperedge(HyperedgeID representative, HyperedgeID to_remove) {
    _hypergraph.setEdgeWeight(representative,
                              _hypergraph.edgeWeight(representative)
                              + _hypergraph.edgeWeight(to_remove));
    _hypergraph.removeEdge(to_remove);
    _removed_parallel_hyperedges.emplace_back(representative, to_remove);
    // PRINT("*** removed HE " << to_remove << " which was parallel to " << representative);
    ++_history.top().parallel_hes_size;
  }

  void createFingerprints(HypernodeID u) {
    _fingerprints.clear();
    forall_incident_hyperedges(he, u, _hypergraph) {
      HyperedgeID hash = /* seed */ 42;
      forall_pins(pin, *he, _hypergraph) {
        hash ^= *pin;
      } endfor
      _fingerprints.emplace_back(*he, hash, _hypergraph.edgeSize(*he));
    } endfor
  }

  void updatePQandContractionTargets(HypernodeID hn, const HeavyEdgeRating& rating,
                                     std::vector<HypernodeID>& contraction_targets) {
    if (rating.valid) {
      _pq.update(hn, rating.value);
      contraction_targets[hn] = rating.target;
    } else {
      _pq.remove(hn);
    }
  }
  
  Hypergraph& _hypergraph;
  Rater _rater;
  std::stack<CoarseningMemento> _history;
  std::vector<HyperedgeID> removed_single_node_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  std::vector<Fingerprint> _fingerprints;
  boost::dynamic_bitset<uint64_t> _contained_hypernodes;
  PriorityQueue<HypernodeID, RatingType> _pq;
};

} // namespace partition

#endif  // PARTITION_COARSENER_H_
