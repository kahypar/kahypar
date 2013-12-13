#ifndef PARTITION_COARSENER_H_
#define PARTITION_COARSENER_H_

#include <stack>

#include <boost/dynamic_bitset.hpp>

#include "Rater.h"
#include "../lib/datastructure/Hypergraph.h"
#include "../lib/datastructure/PriorityQueue.h"

namespace partition {
using datastructure::PriorityQueue;

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
      _hg(hypergraph),
      _rater(_hg, threshold_node_weight),
      _history(),
      _removed_single_node_hyperedges(),
      _removed_parallel_hyperedges(),
      _fingerprints(),
      _contained_hypernodes(_hg.initialNumNodes()),
      _pq(_hg.initialNumNodes(), _hg.initialNumNodes()) {}
  
  void coarsen(int limit) {
    ASSERT(_pq.empty(), "coarsen() can only be called once");
    HeavyEdgeRating rating;
    std::vector<HypernodeID> contraction_targets(_hg.initialNumNodes());
    rateAllHypernodes(contraction_targets);

    HypernodeID rep_node;
    boost::dynamic_bitset<uint64_t> rerated_hypernodes(_hg.initialNumNodes());
    boost::dynamic_bitset<uint64_t> inactive_hypernodes(_hg.initialNumNodes());
    while (!_pq.empty() && _hg.numNodes() > limit) {
      rep_node = _pq.max();
      PRINT("Contracting: (" << rep_node << ","
            << contraction_targets[rep_node] << ") prio: " << _pq.maxKey());

      performContraction(rep_node, contraction_targets);
      removeSingleNodeHyperedges(rep_node);
      removeParallelHyperedges(rep_node);

      rating = _rater.rate(rep_node);
      rerated_hypernodes[rep_node] = 1;
      updatePQandContractionTargets(rep_node, rating, contraction_targets, inactive_hypernodes);

      reRateAffectedHypernodes(rep_node, contraction_targets, rerated_hypernodes, inactive_hypernodes);
    }
   
  }

  void uncoarsen() {
    while(!_history.empty()) {
      restoreParallelHyperedges(_history.top());
      restoreSingleNodeHyperedges(_history.top());
      _hg.uncontract(_history.top().contraction_memento);
      _history.pop();
    }
  }

 private:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);
  
  void performContraction(HypernodeID rep_node, std::vector<HypernodeID>& contraction_targets) {
    _history.emplace(_hg.contract(rep_node, contraction_targets[rep_node]));
    _pq.remove(contraction_targets[rep_node]);
  }

  void restoreSingleNodeHyperedges(const CoarseningMemento& memento) {
    for (size_t i = memento.one_pin_hes_begin;
         i < memento.one_pin_hes_begin + memento.one_pin_hes_size; ++i) {
      ASSERT(i < _removed_single_node_hyperedges.size(), "Index out of bounds");
      // PRINT("*** restore single-node HE " << _removed_single_node_hyperedges[i]);
      _hg.restoreEdge(_removed_single_node_hyperedges[i]);
    }
  }

  void restoreParallelHyperedges(const CoarseningMemento& memento) {
    for (size_t i = memento.parallel_hes_begin;
         i < memento.parallel_hes_begin + memento.parallel_hes_size; ++i) {
      ASSERT(i < _removed_parallel_hyperedges.size(), "Index out of bounds");
      // PRINT("*** restore HE " << _removed_parallel_hyperedges[i].removed_id
      //       << " which is parallel to " << _removed_parallel_hyperedges[i].representative_id);
      _hg.restoreEdge(_removed_parallel_hyperedges[i].removed_id);
      _hg.setEdgeWeight(_removed_parallel_hyperedges[i].representative_id,
                                _hg.edgeWeight(_removed_parallel_hyperedges[i].representative_id) -
                                _hg.edgeWeight(_removed_parallel_hyperedges[i].removed_id));
    }
  }

  void rateAllHypernodes(std::vector<HypernodeID>& contraction_targets) {
    HeavyEdgeRating rating;
    forall_hypernodes(hn, _hg) {
      rating = _rater.rate(*hn);
      if (rating.valid) {
        _pq.insert(*hn, rating.value);
        contraction_targets[*hn] = rating.target;
      }
    } endfor    
  }

  void reRateAffectedHypernodes(HypernodeID rep_node,
                                std::vector<HypernodeID>& contraction_targets,
                                boost::dynamic_bitset<uint64_t>& rerated_hypernodes,
                                boost::dynamic_bitset<uint64_t>& inactive_hypernodes) {
    // ToDo: This can be done more fine grained if we know which HEs are affected: see p. 31
    HeavyEdgeRating rating;
    forall_incident_hyperedges(he, rep_node, _hg) {
      forall_pins(pin, *he, _hg) {
        if (!rerated_hypernodes[*pin] && !inactive_hypernodes[*pin]) {
          PRINT("rating pin " << *pin);
          rating = _rater.rate(*pin);
          rerated_hypernodes[*pin] = 1;
          updatePQandContractionTargets(*pin, rating, contraction_targets, inactive_hypernodes);
        }
      } endfor
    } endfor
    rerated_hypernodes.reset();
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

  void updatePQandContractionTargets(HypernodeID hn, const HeavyEdgeRating& rating,
                                     std::vector<HypernodeID>& contraction_targets,
                                     boost::dynamic_bitset<uint64_t>& inactive_hypernodes) {
    if (rating.valid) {
      _pq.update(hn, rating.value);
      contraction_targets[hn] = rating.target;
    } else if (_pq.contains(hn)) {
      _pq.remove(hn);
      inactive_hypernodes[hn] = 1;
    }
  }
  
  Hypergraph& _hg;
  Rater _rater;
  std::stack<CoarseningMemento> _history;
  std::vector<HyperedgeID> _removed_single_node_hyperedges;
  std::vector<ParallelHE> _removed_parallel_hyperedges;
  std::vector<Fingerprint> _fingerprints;
  boost::dynamic_bitset<uint64_t> _contained_hypernodes;
  PriorityQueue<HypernodeID, RatingType> _pq;
};

} // namespace partition

#endif  // PARTITION_COARSENER_H_
