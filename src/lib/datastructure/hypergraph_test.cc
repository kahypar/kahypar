#include <iostream>

#include "gmock/gmock.h"

#include "../definitions.h"
#include "Hypergraph.h"

namespace hgr {

using ::testing::Eq;
using ::testing::Test;

typedef typename hgr::Hypergraph<HyperNodeID,HyperEdgeID,HyperNodeWeight,HyperEdgeWeight> HypergraphType;
typedef typename HypergraphType::ConstIncidenceIterator ConstIncidenceIterator;
typedef typename HypergraphType::ConstHypernodeIterator ConstHypernodeIterator;
typedef typename HypergraphType::ConstHyperedgeIterator ConstHyperedgeIterator;
typedef typename HypergraphType::HypernodeID HypernodeID;
typedef typename HypergraphType::HyperedgeID HyperedgeID;

class AHypergraph : public Test {
 public:
  AHypergraph() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}) {}
  HypergraphType hypergraph;
};

class AHypernodeIterator : public Test {
 public:
  AHypernodeIterator() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      begin(),
      end() {}
  HypergraphType hypergraph;
  ConstHypernodeIterator begin;
  ConstHypernodeIterator end;
};

class AHyperedgeIterator : public Test {
 public:
  AHyperedgeIterator() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      begin(),
      end() {}
  HypergraphType hypergraph;
  ConstHyperedgeIterator begin;
  ConstHyperedgeIterator end;
};

class AHypergraphMacro : public Test {
 public:
  AHypergraphMacro() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}) {}
  HypergraphType hypergraph;
};

TEST_F(AHypergraph, InitializesInternalHypergraphRepresentation) {
  EXPECT_THAT(hypergraph.numHypernodes(), Eq(7));
  EXPECT_THAT(hypergraph.numHyperdeges(), Eq(4));
  EXPECT_THAT(hypergraph.numPins(), Eq(12));

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
  ASSERT_THAT(hypergraph.hypernodeDegree(6), Eq(2));
}

TEST_F(AHypergraph, ReturnsHyperEdgeSize) {
  ASSERT_THAT(hypergraph.hyperedgeSize(2), Eq(3));
}

TEST_F(AHypergraph, SetsAndGetsHyperNodeWeight) {
  hypergraph.setHypernodeWeight(0, 42);
  ASSERT_THAT(hypergraph.hypernodeWeight(0), Eq(42));
}

TEST_F(AHypergraph, SetsAndGetsHyperEdgeWeight) {
  hypergraph.setHyperedgeWeight(1, 23);
  ASSERT_THAT(hypergraph.hyperedgeWeight(1), Eq(23));
}

TEST_F(AHypergraph, ReturnsNumberOfHypernodes) {
  ASSERT_THAT(hypergraph.numHypernodes(), Eq(7));
}

TEST_F(AHypergraph, ReturnsNumberOfHyperedges) {
  ASSERT_THAT(hypergraph.numHyperdeges(), Eq(4));
}

TEST_F(AHypergraph, ReturnsNumberOfPins) {
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
}

TEST_F(AHypergraph, DecrementsNumberOfHypernodesOnHypernodeRemoval) {
  EXPECT_THAT(hypergraph.numHypernodes(), Eq(7));
  hypergraph.removeHypernode(6);
  ASSERT_THAT(hypergraph.numHypernodes(), Eq(6));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHypernodeRemoval) {
  EXPECT_THAT(hypergraph.numPins(), Eq(12));
  hypergraph.removeHypernode(6);
  ASSERT_THAT(hypergraph.numPins(), Eq(10));
}

TEST_F(AHypergraph, DecrementsSizeOfAffectedHyperedgesOnHypernodeRemoval) {
 EXPECT_THAT(hypergraph.hyperedgeSize(3), Eq(3));
 EXPECT_THAT(hypergraph.hyperedgeSize(2), Eq(3));
 hypergraph.removeHypernode(6);
 ASSERT_THAT(hypergraph.hyperedgeSize(3), Eq(2));
 ASSERT_THAT(hypergraph.hyperedgeSize(2), Eq(2));
}

TEST_F(AHypergraph, InvalidatesRemovedHypernode) {  
  EXPECT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(false));
  hypergraph.removeHypernode(6);
  ASSERT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, DecrementsNumberOfHyperedgesOnHyperedgeRemoval) {
  EXPECT_THAT(hypergraph.numHyperdeges(), Eq(4));
  hypergraph.removeHyperedge(2);
  ASSERT_THAT(hypergraph.numHyperdeges(), Eq(3));
}

TEST_F(AHypergraph, InvalidatesRemovedHyperedge) {
  EXPECT_THAT(hypergraph.hyperedge(2).isInvalid(), Eq(false));
  hypergraph.removeHyperedge(2);
  ASSERT_THAT(hypergraph.hyperedge(2).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHyperedgeRemoval) {
  EXPECT_THAT(hypergraph.numPins(), Eq(12));
  hypergraph.removeHyperedge(2);
  ASSERT_THAT(hypergraph.numPins(), Eq(9));
}

TEST_F(AHypergraph, DecrementsHypernodeDegreeOfAffectedHypernodesOnHyperedgeRemoval) {
  EXPECT_THAT(hypergraph.hypernode(3).size(), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(4).size(), Eq(2));
  EXPECT_THAT(hypergraph.hypernode(6).size(), Eq(2));
  hypergraph.removeHyperedge(2);
  ASSERT_THAT(hypergraph.hypernode(3).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(4).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(6).size(), Eq(1));
}

TEST_F(AHypergraph, DecrementsHypernodeDegreeAfterDisconnectingAHypernodeFromHyperedge) {
  EXPECT_THAT(hypergraph.hypernodeDegree(4), Eq(2));
  hypergraph.disconnect(4, 2);
  ASSERT_THAT(hypergraph.hypernodeDegree(4), Eq(1));
}

TEST_F(AHypergraph, DecrementsHyperedgeSizeAfterDisconnectingAHypernodeFromHyperedge) {
  EXPECT_THAT(hypergraph.hyperedgeSize(2), Eq(3));
  hypergraph.disconnect(4, 2);
  ASSERT_THAT(hypergraph.hyperedgeSize(2), Eq(2));
}

TEST_F(AHypergraph, DoesNotInvalidateHypernodeAfterDisconnectingFromHyperedge) {
  EXPECT_THAT(hypergraph.hypernode(4).isInvalid(), Eq(false));
  hypergraph.disconnect(4, 2);
  ASSERT_THAT(hypergraph.hypernode(4).isInvalid(), Eq(false));
}

TEST_F(AHypergraph, InvalidatesContractedHypernode) {
  EXPECT_THAT(hypergraph.hypernode(2).isInvalid(), Eq(false));
  hypergraph.contract(0,2);
  ASSERT_THAT(hypergraph.hypernode(2).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, RelinksHyperedgesOfContractedHypernodeToRepresentative) {
  EXPECT_THAT(hypergraph.hypernodeDegree(0), Eq(2));
  hypergraph.contract(0,2);
  ASSERT_THAT(hypergraph.hypernodeDegree(0), Eq(3));
}

TEST_F(AHypergraph, AddsHypernodeWeightOfContractedNodeToRepresentative) {
  EXPECT_THAT(hypergraph.hypernodeWeight(0), Eq(1));
  hypergraph.contract(0,2);
  ASSERT_THAT(hypergraph.hypernodeWeight(0), Eq(2));
}

TEST_F(AHypergraph, ReducesHyperedgeSizeOfHyperedgesAffectedByContraction) {
  EXPECT_THAT(hypergraph.hyperedgeSize(0), Eq(2));
  hypergraph.contract(0,2);
  ASSERT_THAT(hypergraph.hyperedgeSize(0), Eq(1));
}

TEST_F(AHypernodeIterator, StartsWithFirstHypernode) {
  std::tie(begin, end) = hypergraph.hypernodes();
  ASSERT_THAT((*begin), Eq(0));
}

TEST_F(AHypernodeIterator, BeginsWithTheFirstValidWhenIterating) {
  hypergraph.removeHypernode(0);
  std::tie(begin, end) = hypergraph.hypernodes();
  ASSERT_THAT(*begin, Eq(1));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenForwardIterating) {
  hypergraph.removeHypernode(1);
  hypergraph.removeHypernode(2);
  std::tie(begin, end) = hypergraph.hypernodes();
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenBackwardIterating) {
  std::tie(begin, end) = hypergraph.hypernodes();
  ++begin;
  ++begin;
  ++begin;
  EXPECT_THAT(*begin, Eq(3));
  hypergraph.removeHypernode(1);
  hypergraph.removeHypernode(2);
  --begin;
  ASSERT_THAT(*begin, Eq(0));
}

TEST_F(AHyperedgeIterator, StartsWithFirstHyperedge) {
  std::tie(begin, end) = hypergraph.hyperedges();
  ASSERT_THAT((*begin), Eq(0));
}

TEST_F(AHyperedgeIterator, StartsWithTheFirstValidHyperedge) {
  hypergraph.removeHyperedge(0);
  std::tie(begin, end) = hypergraph.hyperedges();
  ASSERT_THAT(*begin, Eq(1));
}

TEST_F(AHyperedgeIterator, SkipsInvalidHyperedgesWhenForwardIterating) {
  hypergraph.removeHyperedge(1);
  hypergraph.removeHyperedge(2);
  std::tie(begin, end) = hypergraph.hyperedges();
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHyperedgeIterator, SkipsInvalidHyperedgesWhenBackwardIterating) {
  std::tie(begin, end) = hypergraph.hyperedges();
  ++begin;
  ++begin;
  ++begin;
  EXPECT_THAT(*begin, Eq(3));
  hypergraph.removeHyperedge(1);
  hypergraph.removeHyperedge(2);
  --begin;
  ASSERT_THAT(*begin, Eq(0));
}

TEST_F(AHypergraphMacro, IteratesOverAllHypernodes) {
  ConstHypernodeIterator hn_iter;
  HypernodeID hypernode_count = 0;
  forall_hypernodes(hn_iter, hypergraph) {
    ASSERT_THAT(*hn_iter, Eq(hypernode_count));
    ++hypernode_count;
  } endfor
  ASSERT_THAT(hypernode_count, Eq(7));
}

TEST_F(AHypergraphMacro, IteratesOverAllHyperedges) {
  ConstHyperedgeIterator he_iter;
  HyperedgeID hyperedge_count = 0;
  forall_hyperedges(he_iter, hypergraph) {
    ASSERT_THAT(*he_iter, Eq(hyperedge_count));
    ++hyperedge_count;
  } endfor
  ASSERT_THAT(hyperedge_count, Eq(4));
}

TEST_F(AHypergraphMacro, IteratesOverAllIncidentHyperedges) {
  ConstIncidenceIterator he_iter;
  int i = 0;
  forall_incident_hyperedges(he_iter, 6, hypergraph) {
  ASSERT_THAT(*he_iter, Eq(*(hypergraph._incidence_array.begin() + hypergraph.hypernode(6).firstEntry() + i)));
    ++i;
  } endfor
}

TEST_F(AHypergraphMacro, IteratesOverAllPinsOfAHyperedge) {
  ConstIncidenceIterator pin_iter;
  int i = 0;
  forall_pins(pin_iter, 2, hypergraph) {
  ASSERT_THAT(*pin_iter, Eq(*(hypergraph._incidence_array.begin() + hypergraph.hyperedge(2).firstEntry() + i)));
    ++i;
  } endfor
}

} // namespace hgr

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();  
}
