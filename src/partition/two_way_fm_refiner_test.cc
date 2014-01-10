#include "gmock/gmock.h"

#include "../lib/datastructure/Hypergraph.h"
#include "TwoWayFMRefiner.h"
#include "Metrics.h"

namespace partition {
using ::testing::Test;
using ::testing::Eq;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;

class ATwoWayFMRefiner : public Test {
 public:
  ATwoWayFMRefiner() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      refiner(hypergraph) {
    hypergraph.changeNodePartition(1,0,1);
    hypergraph.changeNodePartition(2,0,1);
    hypergraph.changeNodePartition(5,0,1);
    hypergraph.changeNodePartition(6,0,1);
  }

  HypergraphType hypergraph;
  TwoWayFMRefiner<HypergraphType> refiner;
};

TEST_F(ATwoWayFMRefiner, IdentifiesBorderHypernodes) {
  ASSERT_THAT(refiner.isBorderNode(0), Eq(true));
  ASSERT_THAT(refiner.isBorderNode(1), Eq(true));
  ASSERT_THAT(refiner.isBorderNode(5), Eq(false));
}

TEST_F(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement) {
  ASSERT_THAT(refiner.computeGain(6), Eq(0));
  ASSERT_THAT(refiner.computeGain(1), Eq(1));
  ASSERT_THAT(refiner.computeGain(5), Eq(-1));
}

TEST_F(ATwoWayFMRefiner, ActivatesBorderNodes) {
  refiner.activate(1);
  ASSERT_THAT(refiner._pq[1]->max(), Eq(1));
  ASSERT_THAT(refiner._pq[1]->maxKey(), Eq(1));
}

TEST_F(ATwoWayFMRefiner, RefinesAroundUncontractedNodes) {
  refiner.refine(1,3, metrics::hyperedgeCut(hypergraph));
}

TEST_F(ATwoWayFMRefiner, DoesNotInvalidateBalanceConstraint) {
  ASSERT_THAT(true, Eq(false));
}

TEST_F(ATwoWayFMRefiner, UpdatesPartitionWeight) {
  ASSERT_THAT(true, Eq(false));
}

TEST_F(ATwoWayFMRefiner, UpdatesUnmarkedNeighbors) {
  ASSERT_THAT(true, Eq(false));
}

TEST_F(ATwoWayFMRefiner, RollsBackToTheLowestCutStateReached) {
  ASSERT_THAT(true, Eq(false));
}

TEST_F(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack) {
  ASSERT_THAT(true, Eq(false));
}

TEST_F(ATwoWayFMRefiner, RollsBackAllNodeMovementsIfCutCouldNotBeImproved) {
  ASSERT_THAT(true, Eq(false));
}

} // namespace partition
