#include <iostream>
#include "gmock/gmock.h"

#include "../definitions.h"
#include "hypergraph.h"

namespace hgr {

using ::testing::Eq;

class HypergraphTest : public ::testing::Test {
 public:
  HypergraphTest() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}) {
  }

  hgr::Hypergraph<HyperNodeID,HyperEdgeID,HyperNodeWeight,HyperEdgeWeight> hypergraph;
  
};

TEST_F(HypergraphTest, InitializesInternalHypergraphRepresentation) {
  EXPECT_THAT(hypergraph.number_of_hypernodes(), Eq(7));
  EXPECT_THAT(hypergraph.number_of_hyperedges(), Eq(4));
  EXPECT_THAT(hypergraph.number_of_pins(), Eq(12));

  EXPECT_THAT(hypergraph.hypernode(0).size(), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(1).size(), Eq(1));
  EXPECT_THAT(hypergraph.hypernode(2).size(), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(3).size(), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(4).size(), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(5).size(), Eq(1));
  EXPECT_THAT(hypergraph.hypernode(6).size(), Eq(2));

  EXPECT_THAT(hypergraph.hyperedge(0).size(), Eq(2));
  EXPECT_THAT(hypergraph.hyperedge(1).size(), Eq(4));
  EXPECT_THAT(hypergraph.hyperedge(2).size(), Eq(3));
  EXPECT_THAT(hypergraph.hyperedge(3).size(), Eq(3));
}

TEST_F(HypergraphTest, ReturnsHyperNodeDegree) {
  ASSERT_THAT(hypergraph.hypernode_degree(6), Eq(2));
}

TEST_F(HypergraphTest, ReturnsHyperEdgeSize) {
  ASSERT_THAT(hypergraph.hyperedge_size(2), Eq(3));
}

TEST_F(HypergraphTest, SetsAndGetsHyperNodeWeight) {
  hypergraph.set_hypernode_weight(0, 42);
  ASSERT_THAT(hypergraph.hypernode_weight(0), ::testing::Eq(42));
}

TEST_F(HypergraphTest, SetsAndGetsHyperEdgeWeight) {
  hypergraph.set_hyperedge_weight(1, 23);
  ASSERT_THAT(hypergraph.hyperedge_weight(1), ::testing::Eq(23));
}

TEST_F(HypergraphTest, ReturnsNumberOfHypernodes) {
  ASSERT_THAT(hypergraph.number_of_hypernodes(), Eq(7));
}

TEST_F(HypergraphTest, ReturnsNumberOfHyperedges) {
  ASSERT_THAT(hypergraph.number_of_hyperedges(), Eq(4));
}

TEST_F(HypergraphTest, ReturnsNumberOfPins) {
  ASSERT_THAT(hypergraph.number_of_pins(), Eq(12));
}

TEST_F(HypergraphTest, RemovesHypernodes) {  
  hypergraph.RemoveHyperNode(6);
  EXPECT_THAT(hypergraph.number_of_hypernodes(), Eq(6));
  EXPECT_THAT(hypergraph.number_of_pins(), Eq(10));
  EXPECT_THAT(hypergraph.hyperedge_size(3), Eq(2));
  EXPECT_THAT(hypergraph.hyperedge_size(2), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(true));
}

TEST_F(HypergraphTest, DisconnectsHypernodeFromHyperedge) {
  hypergraph.Disconnect(4, 2);

  EXPECT_THAT(hypergraph.hypernode_degree(4), Eq(1));
  EXPECT_THAT(hypergraph.hyperedge_size(2), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(4).isInvalid(), Eq(false));
}

} // namespace hgr

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();  
}
