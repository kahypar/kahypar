#ifndef PARTITION_COARSENER_H_
#define PARTITION_COARSENER_H_

#include "../lib/datastructure/Hypergraph.h"

class Coarsener {
  typedef hgr::HypergraphType HypergraphType;
  
public:
  Coarsener(const HypergraphType& hypergraph) :
      _hypergraph(hypergraph) {}
  
private:
  const HypergraphType& _hypergraph;
};

#endif  // PARTITION_COARSENER_H_
