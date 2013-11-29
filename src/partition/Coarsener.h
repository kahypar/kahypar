#ifndef PARTITION_COARSENER_H_
#define PARTITION_COARSENER_H_

#include <stack>

#include "../lib/datastructure/Hypergraph.h"

class Coarsener {
  typedef hgr::HypergraphType HypergraphType;
  typedef hgr::HypergraphType::ContractionMemento Memento;
  
public:
  Coarsener(HypergraphType& hypergraph, int coarsening_limit, int threshold_node_weight) :
      _hypergraph(hypergraph),
      _coarsening_limit(coarsening_limit),
      _threshold_node_weight(threshold_node_weight),
      _coarsening_history() {}

  void coarsen() {
    _coarsening_history.push(_hypergraph.contract(0,2));
  }

  void uncoarsen() {
    while(!_coarsening_history.empty()) {
      _hypergraph.uncontract(_coarsening_history.top());
      _coarsening_history.pop();
    }
  }
  
private:
  HypergraphType& _hypergraph;
  const int _coarsening_limit;
  const int _threshold_node_weight;
  std::stack<Memento> _coarsening_history;
};

#endif  // PARTITION_COARSENER_H_
