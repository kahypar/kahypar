#ifndef PARTITION_COARSENER_H_
#define PARTITION_COARSENER_H_

#include <stack>

#include <boost/dynamic_bitset.hpp>

#include "RatingTieBreakingPolicies.h"
#include "../lib/datastructure/Hypergraph.h"
#include "../lib/datastructure/PriorityQueue.h"

using datastructure::PriorityQueue;

namespace partition {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++" // See Modern C++ Design
template <class Hypergraph, typename RatingType_, template <class> class _TieBreakingPolicy>
class Coarsener : public _TieBreakingPolicy<typename Hypergraph::HypernodeID>  {
 public:
  typedef RatingType_ RatingType;
  
 private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::ContractionMemento Memento;
  typedef typename Hypergraph::ConstIncidenceIterator ConstIncidenceIterator;
  typedef typename Hypergraph::ConstHypernodeIterator ConstHypernodeIterator;
  typedef _TieBreakingPolicy<HypernodeID> TieBreakingPolicy;
  
  struct HeavyEdgeRating {
    HypernodeID target;
    RatingType value;
    bool valid;
    HeavyEdgeRating(HypernodeID trgt, RatingType val, bool is_valid) :
        target(trgt),
        value(val),
        valid(is_valid) {}
    HeavyEdgeRating() :
        target(std::numeric_limits<HypernodeID>::max()),
        value(std::numeric_limits<RatingType>::min()),
        valid(false) {}
  };
  
 public:  
  Coarsener(Hypergraph& hypergraph, int coarsening_limit, int threshold_node_weight) :
      _hypergraph(hypergraph),
      _coarsening_limit(coarsening_limit),
      _threshold_node_weight(threshold_node_weight),
      _coarsening_history(),
      _tmp_edge_ratings(_hypergraph.initialNumNodes(), static_cast<RatingType>(0)),
      _visited_hypernodes(_hypergraph.initialNumNodes()),
      _used_entries(),
      _equally_rated_nodes(),
      _prio_queue(_hypergraph.initialNumNodes(), _hypergraph.initialNumNodes()) {}
  
  void coarsen() {
    //    _coarsening_history.push(_hypergraph.contract(0,2));
    forall_hypernodes(hn, _hypergraph) {
      HeavyEdgeRating rating = rate(*hn);
      PRINT("HN " << *hn << ": (" << *hn << "," << rating.target << ")=" << rating.value);
    } endfor
  }

  void uncoarsen() {
    while(!_coarsening_history.empty()) {
      _hypergraph.uncontract(_coarsening_history.top());
      _coarsening_history.pop();
    }
  }

  HeavyEdgeRating rate(HypernodeID u) {
    ASSERT(_used_entries.empty(), "Stack is not empty");
    ASSERT(_visited_hypernodes.none(), "Bitset not empty");
    forall_incident_hyperedges(he,  u, _hypergraph) {
      forall_pins(v, *he, _hypergraph) {
        if (*v != u && belowThresholdNodeWeight(*v, u) ) {
          _tmp_edge_ratings[*v] += static_cast<RatingType>(_hypergraph.edgeWeight(*he))
                                   / (_hypergraph.edgeSize(*he) - 1);
          if (!_visited_hypernodes[*v]) {
            _visited_hypernodes[*v] = 1;
            _used_entries.push(*v);
          } 
        }
      } endfor
    } endfor
          
    _equally_rated_nodes.clear();
    RatingType tmp = 0.0;
    RatingType max_rating = std::numeric_limits<RatingType>::min();
    while (!_used_entries.empty()) {
      tmp = _tmp_edge_ratings[_used_entries.top()] /
            (_hypergraph.nodeWeight(u) * _hypergraph.nodeWeight(_used_entries.top()));
      // PRINT("r(" << u << "," << _used_entries.top() << ")=" << tmp); 
      if (max_rating < tmp) {
        max_rating = tmp;
        _equally_rated_nodes.clear();
        _equally_rated_nodes.push_back(_used_entries.top());
      } else if (max_rating == tmp) {
        _equally_rated_nodes.push_back(_used_entries.top());
      }
      _tmp_edge_ratings[_used_entries.top()] = 0.0;
      _visited_hypernodes[_used_entries.top()] = 0;
      _used_entries.pop();
    }
    HeavyEdgeRating ret;
    if (!_equally_rated_nodes.empty()) {
      ret.value = max_rating;
      ret.target = TieBreakingPolicy::select(_equally_rated_nodes);
      ret.valid = true;
    }
    return ret;
  }
  
 private:
  bool belowThresholdNodeWeight(HypernodeID u, HypernodeID v) {
    return _hypergraph.nodeWeight(v) + _hypergraph.nodeWeight(u)
        <= _threshold_node_weight;
  }
  
  Hypergraph& _hypergraph;
  const int _coarsening_limit;
  const int _threshold_node_weight;
  std::stack<Memento> _coarsening_history;
  std::vector<RatingType> _tmp_edge_ratings;
  boost::dynamic_bitset<uint64_t> _visited_hypernodes;
  std::stack<HypernodeID> _used_entries;
  std::vector<HypernodeID> _equally_rated_nodes;
  PriorityQueue<HypernodeID, RatingType> _prio_queue;
};
#pragma GCC diagnostic pop

} // namespace partition

#endif  // PARTITION_COARSENER_H_
