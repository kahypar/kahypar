#include "gmock/gmock.h"

#include "Coarsener.h"
#include "../lib/datastructure/Hypergraph.h"

using ::testing::Test;
using ::testing::Eq;
using ::testing::DoubleEq;

using defs::hMetisHyperEdgeIndexVector;
using defs::hMetisHyperEdgeVector;

typedef hgr::HypergraphType HypergraphType;
typedef Coarsener<defs::RatingType> CoarsenerType;

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
  CoarsenerType coarsener(hypergraph, coarsening_limit, threshold_node_weight);
}

TEST_F(ACoarsener, UsesACoarseningHistoryToRememberAndUndoContractions) {
  CoarsenerType coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  coarsener.coarsen();
  coarsener.uncoarsen();
}

TEST_F(ACoarsener, CalculatesHeavyEdgeRating) {
  CoarsenerType coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  ASSERT_THAT(coarsener.rate(0).value, Eq(1));
  ASSERT_THAT(coarsener.rate(0).target, Eq(2));
  ASSERT_THAT(coarsener.rate(3).value, DoubleEq(5.0/6));
  ASSERT_THAT(coarsener.rate(3).target, Eq(4));
}
