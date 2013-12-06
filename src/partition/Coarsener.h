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
      _removed_hyperedges(),
      _removed_parallel_hyperedges(),
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

  void restoreSingleNodeHyperedges(CoarseningMemento& memento) {
    // for (size_t i = memento.one_pin_hes_begin;
    //      i < memento.one_pin_hes_begin + memento.one_pin_hes_size; ++i) {
    //   ASSERT(i < _removed_hyperedges.size(), "Index out of bounds");
    //   PRINT("*** restore single-node HE " << _removed_hyperedges[i]);
    //   _hypergraph.restoreSingleNodeHyperedge(_removed_hyperedges[i]);
    // }
  }

  void restoreParallelHyperedges(CoarseningMemento& memento) {
    for (size_t i = memento.parallel_hes_begin;
         i < memento.parallel_hes_begin + memento.parallel_hes_size; ++i) {
      ASSERT(i < _removed_parallel_hyperedges.size(), "Index out of bounds");
      PRINT("*** restore HE " << _removed_parallel_hyperedges[i].removed_id << " which is parallel to " << _removed_parallel_hyperedges[i].representative_id);
      _hypergraph.restoreParallelHyperedge(_removed_parallel_hyperedges[i].representative_id,
                                           _removed_parallel_hyperedges[i].removed_id);
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
    // ASSERT(_history.top().contraction_memento.u == u,
    //        "Current coarsening memento does not belong to hypernode" << u);
    // _history.top().one_pin_hes_begin = _removed_hyperedges.size();
    // IncidenceIterator begin, end;
    // std::tie(begin, end) = _hypergraph.incidentEdges(u);
    // for (IncidenceIterator he_it = begin; he_it != end; ++he_it) {
    //   if (_hypergraph.edgeSize(*he_it) == 1) {
    //     _removed_hyperedges.push_back(*he_it);
    //     ++_history.top().one_pin_hes_size;
    //     PRINT("*** removing single-node HE " << *he_it);
    //     _hypergraph.printEdgeState(*he_it);
    //     _hypergraph.removeEdge(*he_it);
    //     --he_it;
    //     --end;
    //   }
    // }
  }

  void removeParallelHyperedges(HypernodeID u) {
    std::vector<Fingerprint> fingerprints;
    boost::dynamic_bitset<uint64_t> contained_hypernodes(_hypergraph.initialNumNodes());
    _history.top().parallel_hes_begin = _removed_parallel_hyperedges.size();
    createFingerprints(u, fingerprints);
    
    std::sort(fingerprints.begin(), fingerprints.end(),
              [](const Fingerprint& a, const Fingerprint& b) {return a.hash < b.hash;});

    for (size_t i = 0; i < fingerprints.size(); ++i) {

      // optimization: do this only if there actually is an equal hash next to the current entry
      contained_hypernodes.reset();
      forall_pins(pin, fingerprints[i].id, _hypergraph) {
        contained_hypernodes[*pin] = 1;
      } endfor

      size_t j = 0;
      for (j = i + 1; j < fingerprints.size() &&
                      fingerprints[i].hash == fingerprints[j].hash; ++j) {
        if (fingerprints[i].size == fingerprints[j].size) {
          // good chance that i and j are parallel so we have to check
          bool is_parallel = true;
          forall_pins(pin, fingerprints[j].id, _hypergraph) {
            if (!contained_hypernodes[*pin]) {
              is_parallel = false;
              break;
            }
          } endfor
          if (is_parallel) {
            _hypergraph.setEdgeWeight(fingerprints[i].id, _hypergraph.edgeWeight(fingerprints[i].id)
                                      + _hypergraph.edgeWeight(fingerprints[j].id));
            _hypergraph.removeEdge(fingerprints[j].id);
            _removed_parallel_hyperedges.emplace_back(fingerprints[i].id, fingerprints[j].id);
            PRINT("*** removed HE " << fingerprints[j].id << " which was parallel to " << fingerprints[i].id);
            ++_history.top().parallel_hes_size;
          }
        }
      }
      i = j;
    }
  }

  void createFingerprints(HypernodeID u, std::vector<Fingerprint>& fingerprints) {
    forall_incident_hyperedges(he, u, _hypergraph) {
      HyperedgeID hash = /* seed */ 42;
      forall_pins(pin, *he, _hypergraph) {
        hash ^= *pin;
      } endfor
      fingerprints.emplace_back(*he, hash, _hypergraph.edgeSize(*he));
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
  std::vector<HyperedgeID> _removed_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  PriorityQueue<HypernodeID, RatingType> _pq;
};

} // namespace partition

#endif  // PARTITION_COARSENER_H_
