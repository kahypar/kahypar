#ifndef PARTITION_COARSENER_H_
#define PARTITION_COARSENER_H_

#include <stack>

#include <boost/dynamic_bitset.hpp>

#include "../lib/datastructure/Hypergraph.h"

template <typename RatingType_>
class Coarsener {
public:
    typedef RatingType_ RatingType;
    
private:
  typedef hgr::HypergraphType HypergraphType;
  typedef hgr::HypergraphType::HypernodeID HypernodeID;
  typedef hgr::HypergraphType::ContractionMemento Memento;
  typedef hgr::HypergraphType::ConstIncidenceIterator ConstIncidenceIterator;
  typedef hgr::HypergraphType::ConstHypernodeIterator ConstHypernodeIterator;
  
  struct HeavyEdgeRating {
    HypernodeID target;
    RatingType value;
  };
  
public:  
  Coarsener(HypergraphType& hypergraph, int coarsening_limit, int threshold_node_weight) :
      _hypergraph(hypergraph),
      _coarsening_limit(coarsening_limit),
      _threshold_node_weight(threshold_node_weight),
      _coarsening_history(),
      _tmp_edge_ratings(_hypergraph.initialNumHypernodes(), static_cast<RatingType>(0)),
      _visited_hypernodes(_hypergraph.initialNumHypernodes()),
      _used_entries() {}

  void coarsen() {
    //    _coarsening_history.push(_hypergraph.contract(0,2));
    forall_hypernodes(hn, _hypergraph) {
      PRINT("HN " << *hn << ": (" << *hn << "," << rate(*hn).target << ")="
            << rate(*hn).value);
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
        if (*v != u) {
          _tmp_edge_ratings[*v] += static_cast<RatingType>(_hypergraph.hyperedgeWeight(*he))
                         / (_hypergraph.hyperedgeSize(*he) - 1);
          if (!_visited_hypernodes[*v]) {
            _visited_hypernodes[*v] = 1;
            _used_entries.push(*v);
          } 
        }
      } endfor
    } endfor
          
    HeavyEdgeRating rating{0, static_cast<RatingType>(0)};
    RatingType tmp = 0.0;
    while (!_used_entries.empty()) {
      tmp = _tmp_edge_ratings[_used_entries.top()] /
            (_hypergraph.hypernodeWeight(u) * _hypergraph.hypernodeWeight(_used_entries.top()));
      //PRINT("r(" << u << "," << _used_entries.top() << ")=" << tmp); 
      if (rating.value < tmp) {
        rating.value = tmp;
        rating.target = _used_entries.top();
      }
      _tmp_edge_ratings[_used_entries.top()] = 0.0;
      _visited_hypernodes[_used_entries.top()] = 0;
      _used_entries.pop();
      // ToDo: collect equals for tiebraking!
    }
    return rating;
  }
  
private:
  HypergraphType& _hypergraph;
  const int _coarsening_limit;
  const int _threshold_node_weight;
  std::stack<Memento> _coarsening_history;
  std::vector<RatingType> _tmp_edge_ratings;
  boost::dynamic_bitset<uint64_t> _visited_hypernodes;
  std::stack<HypernodeID> _used_entries;
};

#endif  // PARTITION_COARSENER_H_







