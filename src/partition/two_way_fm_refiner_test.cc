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
using datastructure::HyperedgeWeight;

class ATwoWayFMRefiner : public Test {
 public:
  ATwoWayFMRefiner() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      refiner(nullptr) {
    hypergraph.changeNodePartition(1,0,1);
    hypergraph.changeNodePartition(2,0,1);
    hypergraph.changeNodePartition(5,0,1);
    hypergraph.changeNodePartition(6,0,1);

    refiner = new TwoWayFMRefiner<HypergraphType>(hypergraph);
    
  }
  
  HypergraphType hypergraph;
  TwoWayFMRefiner<HypergraphType>* refiner;
  DISALLOW_COPY_AND_ASSIGN(ATwoWayFMRefiner);
};

TEST_F(ATwoWayFMRefiner, IdentifiesBorderHypernodes) {
  ASSERT_THAT(refiner->isBorderNode(0), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(1), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(5), Eq(false));
}

TEST_F(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement) {
  ASSERT_THAT(refiner->computeGain(6), Eq(0));
  ASSERT_THAT(refiner->computeGain(1), Eq(1));
  ASSERT_THAT(refiner->computeGain(5), Eq(-1));
}

TEST_F(ATwoWayFMRefiner, ActivatesBorderNodes) {
  refiner->activate(1);
  ASSERT_THAT(refiner->_pq[1]->max(), Eq(1));
  ASSERT_THAT(refiner->_pq[1]->maxKey(), Eq(1));
}

TEST_F(ATwoWayFMRefiner, CalculatesNodeCountsInBothPartitions) {
  ASSERT_THAT(refiner->_partition_size[0], Eq(3));
  ASSERT_THAT(refiner->_partition_size[1], Eq(4));
}


TEST_F(ATwoWayFMRefiner, UpdatesNodeCountsOnNodeMovements) {
  ASSERT_THAT(refiner->_partition_size[0], Eq(3));
  ASSERT_THAT(refiner->_partition_size[1], Eq(4));
  
  refiner->moveHypernode(1,1,0);

  ASSERT_THAT(refiner->_partition_size[0], Eq(4));
  ASSERT_THAT(refiner->_partition_size[1], Eq(3));
}

// TEST_F(ATwoWayFMRefiner, DoesNotInvalidateBalanceConstraint) {
//   ASSERT_THAT(true, Eq(false));
// }

// TEST_F(ATwoWayFMRefiner, UpdatesPartitionWeight) {
//   ASSERT_THAT(true, Eq(false));
// }

// TEST_F(ATwoWayFMRefiner, UpdatesUnmarkedNeighbors) {
//   ASSERT_THAT(true, Eq(false));
// }

// TEST_F(ATwoWayFMRefiner, RollsBackToTheLowestCutStateReached) {
//   ASSERT_THAT(true, Eq(false));
// }

// TEST_F(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack) {
//   ASSERT_THAT(true, Eq(false));
// }

// TEST_F(ATwoWayFMRefiner, UsesRandomTieBreakingOnEqualCutAcceptance) {
//   ASSERT_THAT(true, Eq(false));
// }

// TEST_F(ATwoWayFMRefiner, ManagesWeightsOfPartitionsExplicitely) {
//   // remember to adjust partition weights after changing the partition of a node
//   ASSERT_THAT(true, Eq(false));
// }

TEST_F(ATwoWayFMRefiner, RollsBackAllNodeMovementsIfCutCouldNotBeImproved) {
  HyperedgeWeight old_cut = metrics::hyperedgeCut(hypergraph);
  HyperedgeWeight new_cut = refiner->refine(1, 6, old_cut);

  ASSERT_THAT(new_cut, Eq(metrics::hyperedgeCut(hypergraph)));
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(5), Eq(1));
}

// Ugly: We could seriously need Mocks here!
TEST(AGainUpdateMethod, RespectsPositiveGainUpdateSpecialCaseForHyperedgesOfSize2) {
  HypergraphType hypergraph(2,1, HyperedgeIndexVector {0,2}, HyperedgeVector {0,1});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);
  ASSERT_THAT(hypergraph.partitionIndex(0), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(0));

  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._pq[0]->insert(0, refiner.computeGain(0));
  refiner._pq[0]->insert(1, refiner.computeGain(1));
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));

  hypergraph.changeNodePartition(1,0,1);
  refiner._marked[1] = true;

  refiner.updateNeighbours(1,0,1);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(1));
  // updateNeighbours does not delete the current max_gain node, neither does it update its gain
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
}

TEST(AGainUpdateMethod, RespectsNegativeGainUpdateSpecialCaseForHyperedgesOfSize2) {
  HypergraphType hypergraph(3,2, HyperedgeIndexVector {0,2,4}, HyperedgeVector {0,1,0,2});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);
  hypergraph.changeNodePartition(1,0,1);
  hypergraph.changeNodePartition(2,0,1);
  ASSERT_THAT(hypergraph.partitionIndex(0), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(1));
  ASSERT_THAT(hypergraph.partitionIndex(2), Eq(1));  
  refiner.activate(0);
  refiner.activate(1);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(2));
  ASSERT_THAT(refiner._pq[1]->key(1), Eq(1));
  
  hypergraph.changeNodePartition(1,1,0);
  refiner._marked[1] = true;
  refiner.updateNeighbours(1,1,0);
  
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
}

TEST(AGainUpdateMethod, HandlesCase0To1) {
  HypergraphType hypergraph(4,1, HyperedgeIndexVector {0,4}, HyperedgeVector {0,1,2,3});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._pq[0]->insert(0, refiner.computeGain(0));
  refiner._pq[0]->insert(1, refiner.computeGain(1));
  refiner._pq[0]->insert(2, refiner.computeGain(2));
  refiner._pq[0]->insert(3, refiner.computeGain(3));
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(3), Eq(-1));

  hypergraph.changeNodePartition(3,0,1);
  refiner._marked[3] = true;
  refiner.updateNeighbours(3,0,1);
  
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(0));
}

TEST(AGainUpdateMethod, HandlesCase1To0) {
  HypergraphType hypergraph(5,2, HyperedgeIndexVector {0,4,8}, HyperedgeVector {0,1,2,3,0,1,2,4});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);
  hypergraph.changeNodePartition(3,0,1);
  hypergraph.changeNodePartition(4,0,1);  
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner.activate(0);
  refiner.activate(1);
  refiner.activate(2);
  refiner.activate(3);
  refiner.activate(4);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(3), Eq(1));

  hypergraph.changeNodePartition(3,1,0);
  refiner._marked[3] = true;
  refiner.updateNeighbours(3,1,0);

  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(-1));
}

TEST(AGainUpdateMethod, HandlesCase2To1) {
  HypergraphType hypergraph(4,1, HyperedgeIndexVector {0,4}, HyperedgeVector {0,1,2,3});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);
  hypergraph.changeNodePartition(2,0,1);
  hypergraph.changeNodePartition(3,0,1);
  refiner.activate(0);
  refiner.activate(1);
  refiner.activate(2);
  refiner.activate(3);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(2), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(3), Eq(0));

  hypergraph.changeNodePartition(3,1,0);
  refiner._marked[3] = true;
  refiner.updateNeighbours(3,1,0);
  
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(2), Eq(1));
}

TEST(AGainUpdateMethod, HandlesCase1To2) {
  HypergraphType hypergraph(4,1, HyperedgeIndexVector {0,4}, HyperedgeVector {0,1,2,3});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);
  hypergraph.changeNodePartition(3,0,1);
  refiner.activate(0);
  refiner.activate(1);
  refiner.activate(2);
  refiner.activate(3);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(3), Eq(1));

  hypergraph.changeNodePartition(2,0,1);
  refiner._marked[2] = true;
  refiner.updateNeighbours(2,0,1);
  
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(3), Eq(0));
}

TEST(AGainUpdateMethod, HandlesSpecialCaseOfHyperedgeWith3Pins) {
  HypergraphType hypergraph(3,1, HyperedgeIndexVector {0,3}, HyperedgeVector {0,1,2});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);
  hypergraph.changeNodePartition(2,0,1);
  refiner.activate(0);
  refiner.activate(1);
  refiner.activate(2);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(2), Eq(1));

  hypergraph.changeNodePartition(1,0,1);
  refiner._marked[1] = true;
  refiner.updateNeighbours(1,0,1);
  
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(1));
  ASSERT_THAT(refiner._pq[1]->key(2), Eq(0));
}

TEST(AGainUpdateMethod, RemovesNonBorderNodesFromPQ) {
  HypergraphType hypergraph(3,1, HyperedgeIndexVector {0,3}, HyperedgeVector {0,1,2});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);
  hypergraph.changeNodePartition(1,0,1);
  refiner.activate(0);
  refiner.activate(1);
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[1]->key(1), Eq(1));
  ASSERT_THAT(refiner._pq[0]->contains(2), Eq(false));
  ASSERT_THAT(refiner._pq[0]->contains(0), Eq(true));
  
  hypergraph.changeNodePartition(1,1,0);
  refiner._marked[1] = true;
  refiner.updateNeighbours(1,1,0);
  
  ASSERT_THAT(refiner._pq[1]->key(1), Eq(1));
  ASSERT_THAT(refiner._pq[0]->contains(0), Eq(false));
  ASSERT_THAT(refiner._pq[0]->contains(2), Eq(false));
  
}

TEST(AGainUpdateMethod, ActivatesUnmarkedNeighbors) {
  HypergraphType hypergraph(3,1, HyperedgeIndexVector {0,3}, HyperedgeVector {0,1,2});
  TwoWayFMRefiner<HypergraphType> refiner(hypergraph);

  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._pq[0]->insert(0, refiner.computeGain(0));
  refiner._pq[0]->insert(1, refiner.computeGain(1));
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->contains(2), Eq(false));

  hypergraph.changeNodePartition(1,0,1);
  refiner._marked[1] = true;
  refiner.updateNeighbours(1,0,1);
  
  ASSERT_THAT(refiner._pq[0]->key(0), Eq(0));
  ASSERT_THAT(refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(refiner._pq[0]->contains(2), Eq(true));
  ASSERT_THAT(refiner._pq[0]->key(2), Eq(0));
}
  
} // namespace partition
