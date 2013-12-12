#include "gmock/gmock.h"

#include "Coarsener.h"
#include "../lib/datastructure/Hypergraph.h"

namespace partition {
using ::testing::AnyOf;
using ::testing::DoubleEq;
using ::testing::Eq;
using ::testing::Le;
using ::testing::Test;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;
using datastructure::HypernodeID;
using datastructure::HypernodeWeight;

typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef Coarsener<HypergraphType, FirstWinsRater> CoarsenerType;

class ACoarsener : public Test {
 public:
  ACoarsener() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      threshold_node_weight(5),
      coarsener(hypergraph, threshold_node_weight) {}
  
  HypergraphType hypergraph;
  HypernodeWeight threshold_node_weight;
  CoarsenerType coarsener;
};

class ACoarsenerWithThresholdWeight3 : public Test {
 public:
  ACoarsenerWithThresholdWeight3() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      threshold_node_weight(3),
      coarsener(hypergraph, threshold_node_weight) {}
  
  HypergraphType hypergraph;
  HypernodeWeight threshold_node_weight;
  CoarsenerType coarsener;
};

TEST_F(ACoarsener, RemovesHyperedgesOfSizeOneDuringCoarsening) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(false));
}

TEST_F(ACoarsener, DecreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne) {
  coarsener.coarsen(6);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(false));

  ASSERT_THAT(hypergraph.numPins(), Eq(10));
}

TEST_F(ACoarsener, ReAddsHyperedgesOfSizeOneDuringUncoarsening) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(false));
  coarsener.uncoarsen();
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(true));
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(true));
  ASSERT_THAT(hypergraph.edgeSize(1), Eq(4));
  ASSERT_THAT(hypergraph.edgeSize(3), Eq(3));
}

TEST_F(ACoarsener, SelectsNodePairToContractBasedOnHighestRating) {
  coarsener.coarsen(6);
  ASSERT_THAT(hypergraph.nodeIsEnabled(2), Eq(false));
  ASSERT_THAT(coarsener._history.top().contraction_memento.u, Eq(0));
  ASSERT_THAT(coarsener._history.top().contraction_memento.v, Eq(2));
}

TEST_F(ACoarsener, RemovesParallelHyperedgesDuringCoarsening) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(3), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(1), Eq(true));
}

TEST_F(ACoarsener, UpdatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeWeight(1), Eq(2));
}
TEST_F(ACoarsener, DecreasesNumberOfHyperedgesOnParallelHyperedgeRemoval) {
 coarsener.coarsen(2);
 ASSERT_THAT(hypergraph.numEdges(), Eq(1));
}

TEST_F(ACoarsener, DecreasesNumberOfPinsOnParallelHyperedgeRemoval) {
 coarsener.coarsen(2);
 ASSERT_THAT(hypergraph.numPins(), Eq(2));
}

TEST_F(ACoarsener, RestoresParallelHyperedgesDuringUncoarsening) {
  coarsener.coarsen(2);
  coarsener.uncoarsen();
  ASSERT_THAT(hypergraph.edgeSize(1), Eq(4));
  ASSERT_THAT(hypergraph.edgeSize(3), Eq(3));
  ASSERT_THAT(hypergraph.edgeWeight(1), Eq(1));
  ASSERT_THAT(hypergraph.edgeWeight(3), Eq(1));
}

TEST_F(ACoarsenerWithThresholdWeight3, DoesNotCoarsenUntilCoarseningLimit) {
  coarsener.coarsen(2);
  forall_hypernodes(hn, hypergraph) {
    ASSERT_THAT(hypergraph.nodeWeight(*hn), Le(3));
  } endfor
  ASSERT_THAT(hypergraph.numNodes(), Eq(3));
}

} // namespace partition
