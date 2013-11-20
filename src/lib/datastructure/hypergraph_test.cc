#include <iostream>

#include "../definitions.h"
#include "hypergraph.h"

int main(int, char**) {

  hMetisHyperEdgeIndexVector idx {0,2,6,9,/*sentinel*/12};
  hMetisHyperEdgeVector edges {0,2,0,1,3,4,3,4,6,2,5,6};
  
  hgr::Hypergraph<HyperNodeID,HyperEdgeID,HyperNodeWeight,HyperEdgeWeight> hypergraph(7,4,idx,edges);
  hypergraph.DEBUG_print();
  hypergraph.Contract(0,2);
  // hypergraph.RemoveHyperNode(0);
  //  hypergraph.RemoveHyperEdge(2);
  // hypergraph.Disconnect(6,3);
  hypergraph.DEBUG_print();
  
  PRINT(hypergraph.number_of_hyperedges());
  HyperEdgeWeight w = hypergraph.hyperedge_weight(0);
  PRINT(w);
  return 0;
}
