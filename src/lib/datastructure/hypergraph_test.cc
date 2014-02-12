/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <iostream>
#include <stack>

#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph.h"
#include "lib/datastructure/Hypergraph_TestFixtures.h"

using::testing::Eq;
using::testing::Test;

using defs::INVALID_PARTITION;

namespace datastructure {
typedef HypergraphType::ContractionMemento Memento;

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
  std::tie(begin, end) = hypergraph.nodes();
  ASSERT_THAT((*begin), Eq(0));
}

TEST_F(AHypernodeIterator, BeginsWithTheFirstValidWhenIterating) {
  hypergraph.removeNode(0);
  std::tie(begin, end) = hypergraph.nodes();
  ASSERT_THAT(*begin, Eq(1));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenForwardIterating) {
  hypergraph.removeNode(1);
  hypergraph.removeNode(2);
  std::tie(begin, end) = hypergraph.nodes();
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenBackwardIterating) {
  std::tie(begin, end) = hypergraph.nodes();
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
  std::tie(begin, end) = hypergraph.edges();
  ASSERT_THAT((*begin), Eq(0));
}

TEST_F(AHyperedgeIterator, StartsWithTheFirstValidHyperedge) {
  hypergraph.removeEdge(0, false);
  std::tie(begin, end) = hypergraph.edges();
  ASSERT_THAT(*begin, Eq(1));
}

TEST_F(AHyperedgeIterator, SkipsInvalidHyperedgesWhenForwardIterating) {
  hypergraph.removeEdge(1, false);
  hypergraph.removeEdge(2, false);
  std::tie(begin, end) = hypergraph.edges();
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHyperedgeIterator, SkipsInvalidHyperedgesWhenBackwardIterating) {
  std::tie(begin, end) = hypergraph.edges();
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
  HypernodeIterator hn_iter;
  HypernodeID hypernode_count = 0;

  forall_hypernodes(hn_iter, hypergraph) {
    ASSERT_THAT(*hn_iter, Eq(hypernode_count));
    ++hypernode_count;
  } endfor
    ASSERT_THAT(hypernode_count, Eq(7));
}

TEST_F(AHypergraphMacro, IteratesOverAllHyperedges) {
  HyperedgeIterator he_iter;
  HyperedgeID hyperedge_count = 0;

  forall_hyperedges(he_iter, hypergraph) {
    ASSERT_THAT(*he_iter, Eq(hyperedge_count));
    ++hyperedge_count;
  } endfor
    ASSERT_THAT(hyperedge_count, Eq(4));
}

TEST_F(AHypergraphMacro, IteratesOverAllIncidentHyperedges) {
  IncidenceIterator he_iter;
  int i = 0;

  forall_incident_hyperedges(he_iter, 6, hypergraph) {
    ASSERT_THAT(*he_iter, Eq(*(hypergraph._incidence_array.begin() +
                               hypergraph.hypernode(6).firstEntry() + i)));
    ++i;
  } endfor
}

TEST_F(AHypergraphMacro, IteratesOverAllPinsOfAHyperedge) {
  IncidenceIterator pin_iter;
  int i = 0;

  forall_pins(pin_iter, 2, hypergraph) {
    ASSERT_THAT(*pin_iter, Eq(*(hypergraph._incidence_array.begin() +
                                hypergraph.hyperedge(2).firstEntry() + i)));
    ++i;
  } endfor
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
  IncidenceIterator begin, end;

  std::tie(begin, end) = hypergraph.pins(3);
  ASSERT_THAT(std::count(begin, end, 6), Eq(1));
  std::tie(begin, end) = hypergraph.pins(2);
  ASSERT_THAT(std::count(begin, end, 6), Eq(1));
  Memento memento = hypergraph.contract(4, 6);
  std::tie(begin, end) = hypergraph.pins(3);
  ASSERT_THAT(std::count(begin, end, 6), Eq(0));
  std::tie(begin, end) = hypergraph.pins(2);
  ASSERT_THAT(std::count(begin, end, 6), Eq(0));

  hypergraph.uncontract(memento);

  std::tie(begin, end) = hypergraph.pins(3);
  ASSERT_THAT(std::count(begin, end, 6), Eq(1));
  std::tie(begin, end) = hypergraph.pins(2);
  ASSERT_THAT(std::count(begin, end, 6), Eq(1));
}

TEST_F(AnUncontractionOperation, RestoresIncidenceInfoForHyperedgesAlredyExistingAtRepresentative) {
  IncidenceIterator begin, end;

  std::tie(begin, end) = hypergraph.pins(2);
  ASSERT_THAT(std::count(begin, end, 4), Eq(1));
  std::tie(begin, end) = hypergraph.pins(1);
  ASSERT_THAT(std::count(begin, end, 4), Eq(1));
  Memento memento = hypergraph.contract(3, 4);
  std::tie(begin, end) = hypergraph.pins(2);
  ASSERT_THAT(std::count(begin, end, 4), Eq(0));
  std::tie(begin, end) = hypergraph.pins(1);
  ASSERT_THAT(std::count(begin, end, 4), Eq(0));

  hypergraph.uncontract(memento);

  std::tie(begin, end) = hypergraph.pins(2);
  ASSERT_THAT(std::count(begin, end, 4), Eq(1));
  std::tie(begin, end) = hypergraph.pins(1);
  ASSERT_THAT(std::count(begin, end, 4), Eq(1));
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
  modified_hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);

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
  ASSERT_THAT(hypergraph.partitionIndex(2), Eq(0));
  Memento memento = hypergraph.contract(0, 2);

  hypergraph.changeNodePartition(0, 0, 1);
  hypergraph.uncontract(memento);

  ASSERT_THAT(hypergraph.partitionIndex(2), Eq(1));
}

TEST(AnUnconnectedHypernode, IsRemovedTogetherWithLastEdgeIfFlagIsTrue) {
  HypergraphType hypergraph(1, 1, HyperedgeIndexVector { 0, /*sentinel*/ 1 },
                            HyperedgeVector { 0 });

  hypergraph.removeEdge(0, true);
  ASSERT_THAT(hypergraph.nodeIsEnabled(0), Eq(false));
}

TEST(AnUnconnectedHypernode, IsNotRemovedTogetherWithLastEdgeIfFlagIsFalse) {
  HypergraphType hypergraph(1, 1, HyperedgeIndexVector { 0, /*sentinel*/ 1 },
                            HyperedgeVector { 0 });

  hypergraph.removeEdge(0, false);
  ASSERT_THAT(hypergraph.nodeIsEnabled(0), Eq(true));
}

TEST_F(AHypergraph, ReducesPinCountOfAffectedHEsOnContraction) {
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, INVALID_PARTITION), Eq(3));
  hypergraph.contract(3, 4);
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(3));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, INVALID_PARTITION), Eq(2));
}

TEST_F(AHypergraph, IncreasesPinCountOfAffectedHEsOnUnContraction) {
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, INVALID_PARTITION), Eq(3));
  Memento memento = hypergraph.contract(3, 4);
  hypergraph.changeNodePartition(3, INVALID_PARTITION, 0);
  hypergraph.uncontract(memento);
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPartition(1, 0), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, INVALID_PARTITION), Eq(1));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, 0), Eq(2));
}

TEST_F(APartitionedHypergraph, StoresPartitionPinCountsForHyperedges) {
  hypergraph.calculatePartitionPinCounts();
  ASSERT_THAT(hypergraph.pinCountInPartition(0, 0), Eq(1));
  ASSERT_THAT(hypergraph.pinCountInPartition(0, 1), Eq(1));
  ASSERT_THAT(hypergraph.pinCountInPartition(1, 0), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPartition(1, 1), Eq(0));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, 0), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, 1), Eq(1));
  ASSERT_THAT(hypergraph.pinCountInPartition(3, 0), Eq(0));
  ASSERT_THAT(hypergraph.pinCountInPartition(3, 1), Eq(3));
}

TEST_F(AHypergraph, InvalidatesPartitionPinCountsOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(4));

  hypergraph.removeEdge(1, false);

  // We do not use accessor pinCountInPartition here since this asserts HE validity
  ASSERT_THAT(hypergraph._partition_pin_counts[3], Eq(hypergraph.INVALID_COUNT));
  ASSERT_THAT(hypergraph._partition_pin_counts[4], Eq(hypergraph.INVALID_COUNT));
  ASSERT_THAT(hypergraph._partition_pin_counts[5], Eq(hypergraph.INVALID_COUNT));
}

TEST_F(AHypergraph, RestoresInvalidatedPartitionPinCountsOnHyperedgeRestore) {
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(4));
  hypergraph.removeEdge(1, false);
  hypergraph.restoreEdge(1);
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(4));
}

TEST_F(AHypergraph, DecrementsPartitionPinCountOnHypernodeRemoval) {
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, INVALID_PARTITION), Eq(3));
  hypergraph.removeNode(3);
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(3));
  ASSERT_THAT(hypergraph.pinCountInPartition(2, INVALID_PARTITION), Eq(2));
}

TEST_F(AHypergraph, UpdatesPartitionPinCountsIfANodeChangesPartition) {
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPartition(1, 1), Eq(0));
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 1);
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(3));
  ASSERT_THAT(hypergraph.pinCountInPartition(1, 1), Eq(1));
}

TEST_F(AHypergraph, CalculatesPinCountsOfAHyperedge) {
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(INVALID_PARTITION));
  ASSERT_THAT(hypergraph.partitionIndex(4), Eq(INVALID_PARTITION));
  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(4));
  ASSERT_THAT(hypergraph.pinCountInPartition(1, 1), Eq(0));
  hypergraph.changeNodePartition(1, INVALID_PARTITION, 1);
  hypergraph.changeNodePartition(4, INVALID_PARTITION, 1);

  hypergraph.calculatePartitionPinCount(1);

  ASSERT_THAT(hypergraph.pinCountInPartition(1, INVALID_PARTITION), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPartition(1, 1), Eq(2));
}

TEST_F(AnUnPartitionedHypergraph, HasAllNodesInInvalidPartition) {
  forall_hypernodes(hn, hypergraph) {
    ASSERT_THAT(hypergraph.partitionIndex(*hn), Eq(INVALID_PARTITION));
  } endfor
}
} // namespace datastructure
