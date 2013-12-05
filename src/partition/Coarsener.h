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
    size_t one_pin_hes_begin; // start of removed hyperedges
    size_t one_pin_hes_size; // # removed hyperedges
    Memento contraction_memento;
    CoarseningMemento(size_t one_pin_hes_begin_, size_t one_pin_hes_size_, Memento contraction_memento_) :
        one_pin_hes_begin(one_pin_hes_begin_),
        one_pin_hes_size(one_pin_hes_size_),
        contraction_memento(contraction_memento_) {}
  };
  
 public:  
  Coarsener(Hypergraph& hypergraph,  HypernodeWeight threshold_node_weight) :
      _hypergraph(hypergraph),
      _rater(_hypergraph, threshold_node_weight),
      _history(),
      _removed_hyperedges(),
      _pq(_hypergraph.initialNumNodes(), _hypergraph.initialNumNodes()) {}
  
  void coarsen(int limit) {
    HeavyEdgeRating rating;
    std::vector<HypernodeID> contraction_targets(_hypergraph.initialNumNodes());
    rateAllHypernodes(contraction_targets);

    HypernodeID rep_node;
    boost::dynamic_bitset<uint64_t> rerated_hypernodes(_hypergraph.initialNumNodes());
    while (!_pq.empty() && _hypergraph.numNodes() > limit) {
      rep_node = _pq.max();
      PRINT("Contracting: (" << rep_node << ","
            << contraction_targets[rep_node] << ") prio: " << _pq.maxKey());

      performContraction(rep_node, contraction_targets);
      deleteSingleNodeHyperedges(rep_node);

      // ToDos:
      // 1.) remove parallel hyperedges
      
      rating = _rater.rate(rep_node);
      rerated_hypernodes[rep_node] = 1;
      updatePQandContractionTargets(rep_node, rating, contraction_targets);

      reRateAffectedHypernodes(rep_node, contraction_targets, rerated_hypernodes);
    }
   
  }

  void uncoarsen() {
    while(!_history.empty()) {
      reEnabledSingleNodeHyperedges(_history.top());
      _hypergraph.uncontract(_history.top().contraction_memento);
      _history.pop();
    }
  }
  
 private:
  FRIEND_TEST(ACoarsener, SelectsNodePairToContractBasedOnHighestRating);
  
  void performContraction(HypernodeID rep_node, std::vector<HypernodeID>& contraction_targets) {
    _history.emplace(0, 0,
            _hypergraph.contract(rep_node, contraction_targets[rep_node]));
    _pq.remove(contraction_targets[rep_node]);
  }

  void reEnabledSingleNodeHyperedges(CoarseningMemento& memento) {
    for (size_t i = memento.one_pin_hes_begin;
         i < memento.one_pin_hes_begin + memento.one_pin_hes_size; ++i) {
      ASSERT(i < _removed_hyperedges.size(), "Index out of bounds");
      _hypergraph.enableEdge(_removed_hyperedges[i]);
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

  void deleteSingleNodeHyperedges(HypernodeID u) {
    ASSERT(_history.top().contraction_memento.u == u,
           "Current coarsening memento does not belong to hypernode" << u);
    _history.top().one_pin_hes_begin = _removed_hyperedges.size();
    IncidenceIterator begin, end;
    std::tie(begin, end) = _hypergraph.incidentEdges(u);
    for (IncidenceIterator he_it = begin; he_it != end; ++he_it) {
      if (_hypergraph.edgeSize(*he_it) == 1) {
        _removed_hyperedges.push_back(*he_it);
        ++_history.top().one_pin_hes_size;
        _hypergraph.removeEdge(*he_it);
        --he_it;
        --end;
      }
    }
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
  PriorityQueue<HypernodeID, RatingType> _pq;
};

} // namespace partition

#endif  // PARTITION_COARSENER_H_
