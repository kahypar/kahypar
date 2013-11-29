#include "gmock/gmock.h"

#include "Coarsener.h"
#include "../lib/datastructure/Hypergraph.h"

using ::testing::Test;

typedef hgr::HypergraphType HypergraphType;

class ACoarsener : public Test {
 public:
  ACoarsener() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}, nullptr, nullptr),
      coarsening_limit(2),
      threshold_node_weight(5) {}
  
  HypergraphType hypergraph;
  int coarsening_limit;
  int threshold_node_weight;
};

TEST_F(ACoarsener, TakesAHypergraphAContractionLimitAndAThresholdForNodeWeightAsInput) {
  Coarsener coarsener(hypergraph, coarsening_limit, threshold_node_weight);
}

TEST_F(ACoarsener, UsesACoarseningHistoryToRememberAndUndoContractions) {
  Coarsener coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  coarsener.coarsen();
  coarsener.uncoarsen();
}
