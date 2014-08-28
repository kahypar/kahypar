/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <iostream>
#include <stack>

#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph_TestFixtures.h"
#include "lib/definitions.h"

using::testing::Eq;
using::testing::Test;

using defs::HypernodeID;
using defs::PartitionID;
using defs::IncidenceIterator;

namespace datastructure {
typedef Hypergraph::ContractionMemento Memento;
TEST_F(AHypergraph, InitializesInternalHypergraphRepresentation) {
  ASSERT_THAT(hypergraph.numNodes(), Eq(7));
  ASSERT_THAT(hypergraph.numEdges(), Eq(4));
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
  ASSERT_THAT(hypergraph.nodeDegree(0), Eq(2));
  ASSERT_THAT(hypergraph.nodeDegree(1), Eq(1));
  ASSERT_THAT(hypergraph.nodeDegree(2), Eq(2));
  ASSERT_THAT(hypergraph.nodeDegree(3), Eq(2));
  ASSERT_THAT(hypergraph.nodeDegree(4), Eq(2));
  ASSERT_THAT(hypergraph.nodeDegree(5), Eq(1));
  ASSERT_THAT(hypergraph.nodeDegree(6), Eq(2));

  ASSERT_THAT(hypergraph.edgeSize(0), Eq(2));
  ASSERT_THAT(hypergraph.edgeSize(1), Eq(4));
  ASSERT_THAT(hypergraph.edgeSize(2), Eq(3));
  ASSERT_THAT(hypergraph.edgeSize(3), Eq(3));
}

TEST_F(AHypergraph, ReturnsHyperNodeDegree) {
  ASSERT_THAT(hypergraph.nodeDegree(6), Eq(2));
}

TEST_F(AHypergraph, ReturnsHyperEdgeSize) {
  ASSERT_THAT(hypergraph.edgeSize(2), Eq(3));
}

TEST_F(AHypergraph, SetsAndGetsHyperNodeWeight) {
  hypergraph.setNodeWeight(0, 42);
  ASSERT_THAT(hypergraph.nodeWeight(0), Eq(42));
}

TEST_F(AHypergraph, SetsAndGetsHyperEdgeWeight) {
  hypergraph.setEdgeWeight(1, 23);
  ASSERT_THAT(hypergraph.edgeWeight(1), Eq(23));
}

TEST_F(AHypergraph, ReturnsNumberOfHypernodes) {
  ASSERT_THAT(hypergraph.numNodes(), Eq(7));
}

TEST_F(AHypergraph, ReturnsNumberOfHyperedges) {
  ASSERT_THAT(hypergraph.numEdges(), Eq(4));
}

TEST_F(AHypergraph, ReturnsNumberOfPins) {
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
}

TEST_F(AHypergraph, DecrementsNumberOfHypernodesOnHypernodeRemoval) {
  ASSERT_THAT(hypergraph.numNodes(), Eq(7));
  hypergraph.removeNode(6);
  ASSERT_THAT(hypergraph.numNodes(), Eq(6));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHypernodeRemoval) {
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
  hypergraph.removeNode(6);
  ASSERT_THAT(hypergraph.numPins(), Eq(10));
}

TEST_F(AHypergraph, DecrementsSizeOfAffectedHyperedgesOnHypernodeRemoval) {
  ASSERT_THAT(hypergraph.edgeSize(3), Eq(3));
  ASSERT_THAT(hypergraph.edgeSize(2), Eq(3));
  hypergraph.removeNode(6);
  ASSERT_THAT(hypergraph.edgeSize(3), Eq(2));
  ASSERT_THAT(hypergraph.edgeSize(2), Eq(2));
}

TEST_F(AHypergraph, InvalidatesRemovedHypernode) {
  ASSERT_THAT(hypergraph.nodeIsEnabled(6), Eq(true));
  hypergraph.removeNode(6);
  ASSERT_THAT(hypergraph.nodeIsEnabled(6), Eq(false));
}

TEST_F(AHypergraph, DecrementsNumberOfHyperedgesOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.numEdges(), Eq(4));
  hypergraph.removeEdge(2, false);
  ASSERT_THAT(hypergraph.numEdges(), Eq(3));
}

TEST_F(AHypergraph, InvalidatesRemovedHyperedge) {
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(true));
  hypergraph.removeEdge(2, false);
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(false));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
  hypergraph.removeEdge(2, false);
  ASSERT_THAT(hypergraph.numPins(), Eq(9));
}

TEST_F(AHypergraph, DecrementsHypernodeDegreeOfAffectedHypernodesOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.hypernode(3).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(4).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(6).size(), Eq(2));
  hypergraph.removeEdge(2, false);
  ASSERT_THAT(hypergraph.hypernode(3).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(4).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(6).size(), Eq(1));
}

TEST_F(AHypergraph, InvalidatesContractedHypernode) {
  ASSERT_THAT(hypergraph.nodeIsEnabled(2), Eq(true));
  hypergraph.contract(0, 2);
  ASSERT_THAT(hypergraph.nodeIsEnabled(2), Eq(false));
}

TEST_F(AHypergraph, RelinksHyperedgesOfContractedHypernodeToRepresentative) {
  ASSERT_THAT(hypergraph.nodeDegree(0), Eq(2));
  hypergraph.contract(0, 2);
  hypergraph.contract(0, 4);
  ASSERT_THAT(hypergraph.nodeDegree(0), Eq(4));
}

TEST_F(AHypergraph, AddsHypernodeWeightOfContractedNodeToRepresentative) {
  ASSERT_THAT(hypergraph.nodeWeight(0), Eq(1));
  hypergraph.contract(0, 2);
  ASSERT_THAT(hypergraph.nodeWeight(0), Eq(2));
}

TEST_F(AHypergraph, ReducesHyperedgeSizeOfHyperedgesAffectedByContraction) {
  ASSERT_THAT(hypergraph.edgeSize(0), Eq(2));
  hypergraph.contract(0, 2);
  ASSERT_THAT(hypergraph.edgeSize(0), Eq(1));
}

TEST_F(AHypergraph, ReducesNumberOfPinsOnContraction) {
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
  hypergraph.contract(3, 4);
  ASSERT_THAT(hypergraph.numPins(), Eq(10));
}

TEST_F(AHypergraph, ReducesTheNumberOfHypernodesOnContraction) {
  ASSERT_THAT(hypergraph.numNodes(), Eq(7));
  hypergraph.contract(3, 4);
  ASSERT_THAT(hypergraph.numNodes(), Eq(6));
}

TEST_F(AHypergraph, DoesNotRemoveParallelHyperedgesOnContraction) {
  ASSERT_THAT(hypergraph.nodeDegree(0), Eq(2));
  hypergraph.contract(5, 6);
  hypergraph.contract(0, 5);
  ASSERT_THAT(hypergraph.nodeDegree(0), Eq(4));
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(true));
  ASSERT_THAT(hypergraph.edgeIsEnabled(3), Eq(true));
  ASSERT_THAT(hypergraph.edgeWeight(0), Eq(1));
  ASSERT_THAT(hypergraph.edgeWeight(3), Eq(1));
}

TEST_F(AHypergraph, DoesNotRemoveHyperedgesOfSizeOneOnContraction) {
  hypergraph.contract(0, 2);
  ASSERT_THAT(hypergraph.edgeSize(0), Eq(1));

  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(true));
}

TEST_F(AHypernodeIterator, StartsWithFirstHypernode) {
  ASSERT_THAT(*(hypergraph.nodes().begin()), Eq(0));
}

TEST_F(AHypernodeIterator, BeginsWithTheFirstValidWhenIterating) {
  hypergraph.removeNode(0);
  ASSERT_THAT(*(hypergraph.nodes().begin()), Eq(1));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenForwardIterating) {
  hypergraph.removeNode(1);
  hypergraph.removeNode(2);
  auto begin = hypergraph.nodes().begin();
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenBackwardIterating) {
  auto begin = hypergraph.nodes().begin();
  ++begin;
  ++begin;
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
  hypergraph.removeNode(1);
  hypergraph.removeNode(2);
  --begin;
  ASSERT_THAT(*begin, Eq(0));
}

TEST_F(AHyperedgeIterator, StartsWithFirstHyperedge) {
  ASSERT_THAT(*(hypergraph.edges().begin()), Eq(0));
}

TEST_F(AHyperedgeIterator, StartsWithTheFirstValidHyperedge) {
  hypergraph.removeEdge(0, false);
  ASSERT_THAT(*(hypergraph.edges().begin()), Eq(1));
}

TEST_F(AHyperedgeIterator, SkipsInvalidHyperedgesWhenForwardIterating) {
  hypergraph.removeEdge(1, false);
  hypergraph.removeEdge(2, false);
  auto begin = hypergraph.edges().begin();
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHyperedgeIterator, SkipsInvalidHyperedgesWhenBackwardIterating) {
  auto begin = hypergraph.edges().begin();
  ++begin;
  ++begin;
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
  hypergraph.removeEdge(1, false);
  hypergraph.removeEdge(2, false);
  --begin;
  ASSERT_THAT(*begin, Eq(0));
}

TEST_F(AHypergraphMacro, IteratesOverAllHypernodes) {
  HypernodeID hypernode_count = 0;

  for (auto && hn : hypergraph.nodes()) {
    ASSERT_THAT(hn, Eq(hypernode_count));
    ++hypernode_count;
  }
  ASSERT_THAT(hypernode_count, Eq(7));
}

TEST_F(AHypergraphMacro, IteratesOverAllHyperedges) {
  HyperedgeID hyperedge_count = 0;

  for (auto && he : hypergraph.edges()) {
    ASSERT_THAT(he, Eq(hyperedge_count));
    ++hyperedge_count;
  }
  ASSERT_THAT(hyperedge_count, Eq(4));
}

TEST_F(AHypergraphMacro, IteratesOverAllIncidentHyperedges) {
  int i = 0;

  for (auto && he : hypergraph.incidentEdges(6)) {
    ASSERT_THAT(he, Eq(*(hypergraph._incidence_array.begin() +
                         hypergraph.hypernode(6).firstEntry() + i)));
    ++i;
  }
}

TEST_F(AHypergraphMacro, IteratesOverAllPinsOfAHyperedge) {
  int i = 0;
  for (auto && pin : hypergraph.pins(2)) {
    ASSERT_THAT(pin, Eq(*(hypergraph._incidence_array.begin() +
                          hypergraph.hyperedge(2).firstEntry() + i)));
    ++i;
  }
}

TEST_F(AContractionMemento, StoresOldStateOfInvolvedHypernodes) {
  HypernodeID u_id = 4;
  HypernodeID u_offset = hypergraph.hypernode(u_id).firstEntry();
  HypernodeID u_size = hypergraph.hypernode(u_id).size();
  HypernodeID v_id = 6;

  Memento memento = hypergraph.contract(4, 6);

  ASSERT_THAT(memento.u, Eq(u_id));
  ASSERT_THAT(memento.u_first_entry, Eq(u_offset));
  ASSERT_THAT(memento.u_size, Eq(u_size));
  ASSERT_THAT(memento.v, Eq(v_id));
}

TEST_F(AnUncontractionOperation, ReEnablesTheInvalidatedHypernode) {
  Memento memento = hypergraph.contract(4, 6);

  ASSERT_THAT(hypergraph.nodeIsEnabled(6), Eq(false));

  hypergraph.uncontract(memento);

  ASSERT_THAT(hypergraph.nodeIsEnabled(6), Eq(true));
}

TEST_F(AnUncontractionOperation, ResetsWeightOfRepresentative) {
  ASSERT_THAT(hypergraph.nodeWeight(4), Eq(1));
  Memento memento = hypergraph.contract(4, 6);
  ASSERT_THAT(hypergraph.nodeWeight(4), Eq(2));

  hypergraph.uncontract(memento);

  ASSERT_THAT(hypergraph.nodeWeight(4), Eq(1));
}

TEST_F(AnUncontractionOperation, DisconnectsHyperedgesAddedToRepresenativeDuringContraction) {
  ASSERT_THAT(hypergraph.nodeDegree(4), Eq(2));
  Memento memento = hypergraph.contract(4, 6);
  ASSERT_THAT(hypergraph.nodeDegree(4), Eq(3));

  hypergraph.uncontract(memento);

  ASSERT_THAT(hypergraph.nodeDegree(4), Eq(2));
}

TEST_F(AnUncontractionOperation, DeletesIncidenceInfoAddedDuringContraction) {
  ASSERT_THAT(hypergraph._incidence_array.size(), Eq(24));
  Memento memento = hypergraph.contract(4, 6);
  ASSERT_THAT(hypergraph._incidence_array.size(), Eq(27));

  hypergraph.uncontract(memento);
  ASSERT_THAT(hypergraph._incidence_array.size(), Eq(24));
}

TEST_F(AnUncontractionOperation, RestoresIncidenceInfoForHyperedgesAddedToRepresentative) {
  ASSERT_THAT(std::count(hypergraph.pins(3).begin(), hypergraph.pins(3).end(), 6), Eq(1));
  ASSERT_THAT(std::count(hypergraph.pins(2).begin(), hypergraph.pins(2).end(), 6), Eq(1));
  Memento memento = hypergraph.contract(4, 6);
  ASSERT_THAT(std::count(hypergraph.pins(3).begin(), hypergraph.pins(3).end(), 6), Eq(0));
  ASSERT_THAT(std::count(hypergraph.pins(2).begin(), hypergraph.pins(2).end(), 6), Eq(0));

  hypergraph.uncontract(memento);

  ASSERT_THAT(std::count(hypergraph.pins(3).begin(), hypergraph.pins(3).end(), 6), Eq(1));
  ASSERT_THAT(std::count(hypergraph.pins(2).begin(), hypergraph.pins(2).end(), 6), Eq(1));
}

TEST_F(AnUncontractionOperation, RestoresIncidenceInfoForHyperedgesAlredyExistingAtRepresentative) {
  IncidenceIterator begin, end;

  ASSERT_THAT(std::count(hypergraph.pins(2).begin(), hypergraph.pins(2).end(), 4), Eq(1));
  ASSERT_THAT(std::count(hypergraph.pins(1).begin(), hypergraph.pins(1).end(), 4), Eq(1));
  Memento memento = hypergraph.contract(3, 4);
  ASSERT_THAT(std::count(hypergraph.pins(2).begin(), hypergraph.pins(2).end(), 4), Eq(0));
  ASSERT_THAT(std::count(hypergraph.pins(1).begin(), hypergraph.pins(1).end(), 4), Eq(0));

  hypergraph.uncontract(memento);

  ASSERT_THAT(std::count(hypergraph.pins(2).begin(), hypergraph.pins(2).end(), 4), Eq(1));
  ASSERT_THAT(std::count(hypergraph.pins(1).begin(), hypergraph.pins(1).end(), 4), Eq(1));
}

TEST_F(AnUncontractionOperation, RestoresNumberOfPinsOnUncontraction) {
  hypergraph.uncontract(hypergraph.contract(3, 4));
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
}

TEST_F(AnUncontractionOperation, RestoresHyperedgeSizeOfHyperedgesAffectedByContraction) {
  hypergraph.uncontract(hypergraph.contract(0, 2));
  ASSERT_THAT(hypergraph.edgeSize(0), Eq(2));
}

TEST_F(AnUncontractedHypergraph, EqualsTheInitialHypergraphBeforeContraction) {
  std::stack<Memento> contraction_history;
  contraction_history.emplace(modified_hypergraph.contract(4, 6));
  contraction_history.emplace(modified_hypergraph.contract(3, 4));
  contraction_history.emplace(modified_hypergraph.contract(0, 2));
  contraction_history.emplace(modified_hypergraph.contract(0, 1));
  contraction_history.emplace(modified_hypergraph.contract(0, 5));
  contraction_history.emplace(modified_hypergraph.contract(0, 3));
  ASSERT_THAT(modified_hypergraph.nodeWeight(0), Eq(7));
  modified_hypergraph.setNodePart(0, 0);

  while (!contraction_history.empty()) {
    modified_hypergraph.uncontract(contraction_history.top());
    contraction_history.pop();
  }

  ASSERT_THAT(verifyEquivalence(hypergraph, modified_hypergraph), Eq(true));
}

TEST_F(AHypergraph, ReturnsInitialNumberOfHypernodesAfterHypergraphModification) {
  ASSERT_THAT(hypergraph.initialNumNodes(), Eq(7));
  hypergraph.removeNode(6);
  ASSERT_THAT(hypergraph.initialNumNodes(), Eq(7));
}

TEST_F(AHypergraph, ReturnsInitialNumberOfPinsAfterHypergraphModification) {
  ASSERT_THAT(hypergraph.initialNumPins(), Eq(12));
  hypergraph.removeNode(6);
  ASSERT_THAT(hypergraph.initialNumPins(), Eq(12));
}

TEST_F(AHypergraph, ReturnsInitialNumberHyperedgesAfterHypergraphModification) {
  ASSERT_THAT(hypergraph.initialNumEdges(), Eq(4));
  hypergraph.removeEdge(2, false);
  ASSERT_THAT(hypergraph.initialNumEdges(), Eq(4));
}

TEST_F(AnUncontractionOperation, UpdatesPartitionIndexOfUncontractedNode) {
  ASSERT_THAT(hypergraph.partID(2), Eq(0));

  Memento memento = hypergraph.contract(0, 2);
  hypergraph.changeNodePart(0, 0, 1);
  hypergraph.uncontract(memento);

  ASSERT_THAT(hypergraph.partID(2), Eq(1));
}

TEST(AnUnconnectedHypernode, IsRemovedTogetherWithLastEdgeIfFlagIsTrue) {
  Hypergraph hypergraph(1, 1, HyperedgeIndexVector { 0, /*sentinel*/ 1 },
                        HyperedgeVector { 0 });

  hypergraph.removeEdge(0, true);
  ASSERT_THAT(hypergraph.nodeIsEnabled(0), Eq(false));
}

TEST(AnUnconnectedHypernode, IsNotRemovedTogetherWithLastEdgeIfFlagIsFalse) {
  Hypergraph hypergraph(1, 1, HyperedgeIndexVector { 0, /*sentinel*/ 1 },
                        HyperedgeVector { 0 });

  hypergraph.removeEdge(0, false);
  ASSERT_THAT(hypergraph.nodeIsEnabled(0), Eq(true));
}

TEST_F(AHypergraph, ReducesPinCountOfAffectedHEsOnContraction) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 0);
  hypergraph.setNodePart(6, 0);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 0), Eq(3));
  hypergraph.contract(3, 4);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(3));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 0), Eq(2));
}

TEST_F(AHypergraph, IncreasesPinCountOfAffectedHEsOnUnContraction) {
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 1), Eq(3));
  Memento memento = hypergraph.contract(3, 4);
  hypergraph.changeNodePart(3, 1, 0);
  hypergraph.uncontract(memento);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 1), Eq(1));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 0), Eq(2));
}

TEST_F(APartitionedHypergraph, StoresPartitionPinCountsForHyperedges) {
  ASSERT_THAT(hypergraph.pinCountInPart(0, 0), Eq(1));
  ASSERT_THAT(hypergraph.pinCountInPart(0, 1), Eq(1));
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(0));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 0), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 1), Eq(1));
  ASSERT_THAT(hypergraph.pinCountInPart(3, 0), Eq(0));
  ASSERT_THAT(hypergraph.pinCountInPart(3, 1), Eq(3));
}

TEST_F(AHypergraph, InvalidatesPartitionPinCountsOnHyperedgeRemoval) {
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(4));

  hypergraph.removeEdge(1, false);

  // We do not use accessor pinCountInPart here since this asserts HE validity
  for (PartitionID i = 0; i < 2; ++i) {
    auto element_it = hypergraph._partition_pin_count[i].find(1);
    if (element_it != hypergraph._partition_pin_count[i].end()) {
      ASSERT_THAT(element_it->second, Eq(hypergraph.kInvalidCount));
    }
  }
}

TEST_F(AHypergraph, RestoresInvalidatedPartitionPinCountsOnHyperedgeRestore) {
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(4));
  hypergraph.removeEdge(1, false);
  hypergraph.restoreEdge(1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(4));
}

TEST_F(AHypergraph, DecrementsPartitionPinCountOnHypernodeRemoval) {
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(6, 1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 1), Eq(3));
  hypergraph.removeNode(3);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(3));
  ASSERT_THAT(hypergraph.pinCountInPart(2, 1), Eq(2));
}

TEST_F(AHypergraph, UpdatesPartitionPinCountsIfANodeChangesPartition) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(0));
  hypergraph.changeNodePart(1, 0, 1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(3));
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(1));
}

TEST_F(AHypergraph, CalculatesPinCountsOfAHyperedge) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 0);
  hypergraph.setNodePart(6, 0);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(0));
  hypergraph.changeNodePart(1, 0, 1);
  hypergraph.changeNodePart(4, 0, 1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(2));
}

TEST_F(AnUnPartitionedHypergraph, HasAllNodesInInvalidPartition) {
  for (auto && hn : hypergraph.nodes()) {
    ASSERT_THAT(hypergraph.partID(hn), Eq(Hypergraph::kInvalidPartition));
  }
}

TEST_F(AHypergraph, MaintainsAConnectivitySetForEachHyperedge) {
  ASSERT_THAT(hypergraph.connectivity(0), Eq(0));
  ASSERT_THAT(hypergraph.connectivity(1), Eq(0));
  ASSERT_THAT(hypergraph.connectivity(2), Eq(0));
  ASSERT_THAT(hypergraph.connectivity(3), Eq(0));
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  ASSERT_THAT(hypergraph.connectivity(0), Eq(2));
  ASSERT_THAT(hypergraph.connectivity(1), Eq(1));
  ASSERT_THAT(hypergraph.connectivity(2), Eq(2));
  ASSERT_THAT(hypergraph.connectivity(3), Eq(1));
}

TEST_F(AHypergraph, RemovesPartFromConnectivitySetIfHEDoesNotConnectThatPart) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  ASSERT_THAT(hypergraph.connectivity(0), Eq(2));

  hypergraph.changeNodePart(2, 1, 0);

  ASSERT_THAT(hypergraph.connectivity(0), Eq(1));
}

TEST_F(AHypergraph, AddsPartToConnectivitySetIfHEConnectsThatPart) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  ASSERT_THAT(hypergraph.connectivity(0), Eq(1));

  hypergraph.changeNodePart(2, 0, 1);

  ASSERT_THAT(hypergraph.connectivity(0), Eq(2));
}


TEST_F(AHypergraph, AllowsIterationOverConnectivitySetOfAHyperege) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(2, 1);
  ASSERT_THAT(hypergraph.connectivity(0), Eq(2));

  int part_id = 0;
  for (auto && part : hypergraph.connectivitySet(0)) {
    ASSERT_THAT(part, Eq(part_id));
    ++part_id;
  }
}

TEST(ConnectivitySets, AreCleardWhenSingleNodeHyperedgesAreRemoved) {
  Hypergraph hypergraph(1, 1, HyperedgeIndexVector { 0, /*sentinel*/ 1 },
                        HyperedgeVector { 0 });
  hypergraph.setNodePart(0, 0);
  ASSERT_THAT(hypergraph.connectivity(0), Eq(1));
  ASSERT_THAT(*(hypergraph.connectivitySet(0).begin()), Eq(0));

  hypergraph.removeEdge(0, false);
  hypergraph.changeNodePart(0, 0, 1);
  hypergraph.restoreEdge(0);

  ASSERT_THAT(hypergraph.connectivity(0), Eq(1));
  ASSERT_THAT(*(hypergraph.connectivitySet(0).begin()), Eq(1));
}
} // namespace datastructure
