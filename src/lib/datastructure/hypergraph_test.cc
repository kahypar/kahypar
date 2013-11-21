#include <iostream>
#include "gmock/gmock.h"

#include "../definitions.h"
#include "hypergraph.h"

namespace hgr {

using ::testing::Eq;
using ::testing::Test;

class AHypergraph : public Test {
 public:
  typedef typename hgr::Hypergraph<HyperNodeID,HyperEdgeID,HyperNodeWeight,HyperEdgeWeight> HypergraphType;
  typedef typename HypergraphType::const_incidence_iterator ConstIncidenceIterator;
  
  AHypergraph() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}) {
  }

  HypergraphType hypergraph;
};

class AHypernodeIterator : public Test {
 public:
  typedef typename hgr::Hypergraph<HyperNodeID,HyperEdgeID,HyperNodeWeight,HyperEdgeWeight> HypergraphType;
  typedef typename HypergraphType::const_hypernode_iterator ConstHypernodeIterator;

  AHypernodeIterator() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      begin(),
      end() {}

  HypergraphType hypergraph;
  ConstHypernodeIterator begin;
  ConstHypernodeIterator end;
};

TEST_F(AHypergraph, InitializesInternalHypergraphRepresentation) {
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

TEST_F(AHypergraph, ReturnsHyperNodeDegree) {
  ASSERT_THAT(hypergraph.hypernode_degree(6), Eq(2));
}

TEST_F(AHypergraph, ReturnsHyperEdgeSize) {
  ASSERT_THAT(hypergraph.hyperedge_size(2), Eq(3));
}

TEST_F(AHypergraph, SetsAndGetsHyperNodeWeight) {
  hypergraph.set_hypernode_weight(0, 42);
  ASSERT_THAT(hypergraph.hypernode_weight(0), Eq(42));
}

TEST_F(AHypergraph, SetsAndGetsHyperEdgeWeight) {
  hypergraph.set_hyperedge_weight(1, 23);
  ASSERT_THAT(hypergraph.hyperedge_weight(1), Eq(23));
}

TEST_F(AHypergraph, ReturnsNumberOfHypernodes) {
  ASSERT_THAT(hypergraph.number_of_hypernodes(), Eq(7));
}

TEST_F(AHypergraph, ReturnsNumberOfHyperedges) {
  ASSERT_THAT(hypergraph.number_of_hyperedges(), Eq(4));
}

TEST_F(AHypergraph, ReturnsNumberOfPins) {
  ASSERT_THAT(hypergraph.number_of_pins(), Eq(12));
}

TEST_F(AHypergraph, DecrementsNumberOfHypernodesOnHypernodeRemoval) {
  EXPECT_THAT(hypergraph.number_of_hypernodes(), Eq(7));
  hypergraph.RemoveHyperNode(6);
  ASSERT_THAT(hypergraph.number_of_hypernodes(), Eq(6));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHypernodeRemoval) {
  EXPECT_THAT(hypergraph.number_of_pins(), Eq(12));
  hypergraph.RemoveHyperNode(6);
  ASSERT_THAT(hypergraph.number_of_pins(), Eq(10));
}

TEST_F(AHypergraph, DecrementsSizeOfAffectedHyperedgesOnHypernodeRemoval) {
 EXPECT_THAT(hypergraph.hyperedge_size(3), Eq(3));
 EXPECT_THAT(hypergraph.hyperedge_size(2), Eq(3));
 hypergraph.RemoveHyperNode(6);
 ASSERT_THAT(hypergraph.hyperedge_size(3), Eq(2));
 ASSERT_THAT(hypergraph.hyperedge_size(2), Eq(2));
}

TEST_F(AHypergraph, InvalidatesRemovedHypernode) {  
  EXPECT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(false));
  hypergraph.RemoveHyperNode(6);
  ASSERT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, DecrementsNumberOfHyperedgesOnHyperedgeRemoval) {
  EXPECT_THAT(hypergraph.number_of_hyperedges(), Eq(4));
  hypergraph.RemoveHyperEdge(2);
  ASSERT_THAT(hypergraph.number_of_hyperedges(), Eq(3));
}

TEST_F(AHypergraph, InvalidatesRemovedHyperedge) {
  EXPECT_THAT(hypergraph.hyperedge(2).isInvalid(), Eq(false));
  hypergraph.RemoveHyperEdge(2);
  ASSERT_THAT(hypergraph.hyperedge(2).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHyperedgeRemoval) {
  EXPECT_THAT(hypergraph.number_of_pins(), Eq(12));
  hypergraph.RemoveHyperEdge(2);
  ASSERT_THAT(hypergraph.number_of_pins(), Eq(9));
}

TEST_F(AHypergraph, DecrementsHypernodeDegreeOfAffectedHypernodesOnHyperedgeRemoval) {
  EXPECT_THAT(hypergraph.hypernode(3).size(), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(4).size(), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(6).size(), Eq(2));
  hypergraph.RemoveHyperEdge(2);
  ASSERT_THAT(hypergraph.hypernode(3).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(4).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(6).size(), Eq(1));
}

TEST_F(AHypergraph, DecrementsHypernodeDegreeAfterDisconnectingAHypernodeFromHyperedge) {
  EXPECT_THAT(hypergraph.hypernode_degree(4), Eq(2));
  hypergraph.Disconnect(4, 2);
  ASSERT_THAT(hypergraph.hypernode_degree(4), Eq(1));
}

TEST_F(AHypergraph, DecrementsHyperedgeSizeAfterDisconnectingAHypernodeFromHyperedge) {
  EXPECT_THAT(hypergraph.hyperedge_size(2), Eq(3));
  hypergraph.Disconnect(4, 2);
  ASSERT_THAT(hypergraph.hyperedge_size(2), Eq(2));
}

TEST_F(AHypergraph, DoesNotInvalidateHypernodeAfterDisconnectingFromHyperedge) {
  EXPECT_THAT(hypergraph.hypernode(4).isInvalid(), Eq(false));
  hypergraph.Disconnect(4, 2);
  ASSERT_THAT(hypergraph.hypernode(4).isInvalid(), Eq(false));
}

TEST_F(AHypergraph, InvalidatesContractedHypernode) {
  EXPECT_THAT(hypergraph.hypernode(2).isInvalid(), Eq(false));
  hypergraph.Contract(0,2);
  ASSERT_THAT(hypergraph.hypernode(2).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, RelinksHyperedgesOfContractedHypernodeToRepresentative) {
  EXPECT_THAT(hypergraph.hypernode_degree(0), Eq(2));
  hypergraph.Contract(0,2);
  ASSERT_THAT(hypergraph.hypernode_degree(0), Eq(3));
}

TEST_F(AHypergraph, AddsHypernodeWeightOfContractedNodeToRepresentative) {
  EXPECT_THAT(hypergraph.hypernode_weight(0), Eq(1));
  hypergraph.Contract(0,2);
  ASSERT_THAT(hypergraph.hypernode_weight(0), Eq(2));
}

TEST_F(AHypergraph, ReducesHyperedgeSizeOfHyperedgesAffectedByContraction) {
  EXPECT_THAT(hypergraph.hyperedge_size(0), Eq(2));
  hypergraph.Contract(0,2);
  ASSERT_THAT(hypergraph.hyperedge_size(0), Eq(1));
}

TEST_F(AHypergraph, AllowsIterationOverIncidentHyperedges) {
  ConstIncidenceIterator begin, end;
  std::tie(begin, end) = hypergraph.GetIncidentHyperedges(3);
  int i = 0;
  for (ConstIncidenceIterator iter = begin; iter != end; ++iter) {
    ASSERT_THAT(*iter, Eq(*(hypergraph.incidence_array_.begin() + hypergraph.hypernode(3).begin() + i)));
    ++i;
  }
}

TEST_F(AHypergraph, AllowsIterationOverPinsOfHyperedge) {
  ConstIncidenceIterator begin, end;
  std::tie(begin, end) = hypergraph.GetPins(1);
  int i = 0;
  for (ConstIncidenceIterator iter = begin; iter != end; ++iter) {
    ASSERT_THAT(*iter, Eq(*(hypergraph.incidence_array_.begin() + hypergraph.hyperedge(1).begin() + i)));
    ++i;
  }
}

TEST_F(AHypernodeIterator, StartsWithFirstHypernodeOnIteration) {
  std::tie(begin, end) = hypergraph.GetAllHypernodes();
  ASSERT_THAT((*begin), Eq(0));
}

TEST_F(AHypernodeIterator, BeginsWithTheFirstValidWhenIteratingOverHypernodes) {
  hypergraph.RemoveHyperNode(0);
  std::tie(begin, end) = hypergraph.GetAllHypernodes();
  ASSERT_THAT(*begin, Eq(1));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenForwardIteratingOverHypernodes) {
  hypergraph.RemoveHyperNode(1);
  hypergraph.RemoveHyperNode(2);
  std::tie(begin, end) = hypergraph.GetAllHypernodes();
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenBackwardIteratingOverHypernodes) {
  std::tie(begin, end) = hypergraph.GetAllHypernodes();
  ++begin;
  ++begin;
  ++begin;
  EXPECT_THAT(*begin, Eq(3));
  hypergraph.RemoveHyperNode(1);
  hypergraph.RemoveHyperNode(2);
  --begin;
  ASSERT_THAT(*begin, Eq(0));
}

} // namespace hgr

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();  
}
