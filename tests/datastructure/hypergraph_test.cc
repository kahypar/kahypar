/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <iostream>
#include <stack>
#include <tuple>

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/partition/coarsening/coarsening_memento.h"
#include "kahypar/partition/coarsening/hypergraph_pruner.h"
#include "tests/datastructure/hypergraph_test_fixtures.h"

using ::testing::Eq;
using ::testing::ContainerEq;
using ::testing::Test;

namespace kahypar {
namespace ds {
using Memento = Hypergraph::ContractionMemento;
TEST_F(AHypergraph, InitializesInternalHypergraphRepresentation) {
  ASSERT_THAT(hypergraph.currentNumNodes(), Eq(7));
  ASSERT_THAT(hypergraph.currentNumEdges(), Eq(4));
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(12));
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
  ASSERT_THAT(hypergraph.currentNumNodes(), Eq(7));
}

TEST_F(AHypergraph, ReturnsNumberOfHyperedges) {
  ASSERT_THAT(hypergraph.currentNumEdges(), Eq(4));
}

TEST_F(AHypergraph, ReturnsNumberOfPins) {
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(12));
}

TEST_F(AHypergraph, DecrementsNumberOfHypernodesOnHypernodeRemoval) {
  ASSERT_THAT(hypergraph.currentNumNodes(), Eq(7));
  hypergraph.removeNode(6);
  ASSERT_THAT(hypergraph.currentNumNodes(), Eq(6));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHypernodeRemoval) {
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(12));
  hypergraph.removeNode(6);
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(10));
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
  ASSERT_THAT(hypergraph.currentNumEdges(), Eq(4));
  hypergraph.removeEdge(2);
  ASSERT_THAT(hypergraph.currentNumEdges(), Eq(3));
}

TEST_F(AHypergraph, InvalidatesRemovedHyperedge) {
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(true));
  hypergraph.removeEdge(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(false));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(12));
  hypergraph.removeEdge(2);
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(9));
}

TEST_F(AHypergraph, DecrementsHypernodeDegreeOfAffectedHypernodesOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.hypernode(3).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(4).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(6).size(), Eq(2));
  hypergraph.removeEdge(2);
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
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(12));
  hypergraph.contract(3, 4);
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(10));
}

TEST_F(AHypergraph, ReducesTheNumberOfHypernodesOnContraction) {
  ASSERT_THAT(hypergraph.currentNumNodes(), Eq(7));
  hypergraph.contract(3, 4);
  ASSERT_THAT(hypergraph.currentNumNodes(), Eq(6));
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
  ASSERT_THAT(*(hypergraph.nodes().first), Eq(0));
}

TEST_F(AHypernodeIterator, BeginsWithTheFirstValidWhenIterating) {
  hypergraph.removeNode(0);
  ASSERT_THAT(*(hypergraph.nodes().first), Eq(1));
}

TEST_F(AHypernodeIterator, SkipsInvalidHypernodesWhenForwardIterating) {
  hypergraph.removeNode(1);
  hypergraph.removeNode(2);
  auto begin = hypergraph.nodes().first;
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHyperedgeIterator, StartsWithFirstHyperedge) {
  ASSERT_THAT(*(hypergraph.edges().first), Eq(0));
}

TEST_F(AHyperedgeIterator, StartsWithTheFirstValidHyperedge) {
  hypergraph.removeEdge(0);
  ASSERT_THAT(*(hypergraph.edges().first), Eq(1));
}

TEST_F(AHyperedgeIterator, SkipsInvalidHyperedgesWhenForwardIterating) {
  hypergraph.removeEdge(1);
  hypergraph.removeEdge(2);
  auto begin = hypergraph.edges().first;
  ++begin;
  ASSERT_THAT(*begin, Eq(3));
}

TEST_F(AHypergraphMacro, IteratesOverAllHypernodes) {
  HypernodeID hypernode_count = 0;

  for (const HypernodeID& hn : hypergraph.nodes()) {
    ASSERT_THAT(hn, Eq(hypernode_count));
    ++hypernode_count;
  }
  ASSERT_THAT(hypernode_count, Eq(7));
}

TEST_F(AHypergraphMacro, IteratesOverAllHyperedges) {
  HyperedgeID hyperedge_count = 0;

  for (const HyperedgeID& he : hypergraph.edges()) {
    ASSERT_THAT(he, Eq(hyperedge_count));
    ++hyperedge_count;
  }
  ASSERT_THAT(hyperedge_count, Eq(4));
}

TEST_F(AHypergraphMacro, IteratesOverAllIncidentHyperedges) {
  int i = 0;
  std::vector<HyperedgeID> incident_nets{ 2, 3 };
  for (const HyperedgeID& he : hypergraph.incidentEdges(6)) {
    ASSERT_THAT(he, incident_nets[i]);
    ++i;
  }
}

TEST_F(AHypergraphMacro, IteratesOverAllPinsOfAHyperedge) {
  int i = 0;
  for (const HypernodeID& pin : hypergraph.pins(2)) {
    ASSERT_THAT(pin, Eq(*(hypergraph._incidence_array.begin() +
                          hypergraph.hyperedge(2).firstEntry() + i)));
    ++i;
  }
}

TEST_F(AContractionMemento, StoresOldStateOfInvolvedHypernodes) {
  HypernodeID u_id = 4;
  HypernodeID v_id = 6;

  Memento memento = hypergraph.contract(4, 6);

  ASSERT_THAT(memento.u, Eq(u_id));
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

TEST_F(AnUncontractionOperation, RestoresIncidenceInfoForHyperedgesAddedToRepresentative) {
  ASSERT_THAT(std::count(hypergraph.pins(3).first, hypergraph.pins(3).second, 6), Eq(1));
  ASSERT_THAT(std::count(hypergraph.pins(2).first, hypergraph.pins(2).second, 6), Eq(1));
  Memento memento = hypergraph.contract(4, 6);
  ASSERT_THAT(std::count(hypergraph.pins(3).first, hypergraph.pins(3).second, 6), Eq(0));
  ASSERT_THAT(std::count(hypergraph.pins(2).first, hypergraph.pins(2).second, 6), Eq(0));

  hypergraph.uncontract(memento);

  ASSERT_THAT(std::count(hypergraph.pins(3).first, hypergraph.pins(3).second, 6), Eq(1));
  ASSERT_THAT(std::count(hypergraph.pins(2).first, hypergraph.pins(2).second, 6), Eq(1));
}

TEST_F(AnUncontractionOperation, RestoresIncidenceInfoForHyperedgesAlredyExistingAtRepresentative) {
  ASSERT_THAT(std::count(hypergraph.pins(2).first, hypergraph.pins(2).second, 4), Eq(1));
  ASSERT_THAT(std::count(hypergraph.pins(1).first, hypergraph.pins(1).second, 4), Eq(1));
  Memento memento = hypergraph.contract(3, 4);
  ASSERT_THAT(std::count(hypergraph.pins(2).first, hypergraph.pins(2).second, 4), Eq(0));
  ASSERT_THAT(std::count(hypergraph.pins(1).first, hypergraph.pins(1).second, 4), Eq(0));

  hypergraph.uncontract(memento);

  ASSERT_THAT(std::count(hypergraph.pins(2).first, hypergraph.pins(2).second, 4), Eq(1));
  ASSERT_THAT(std::count(hypergraph.pins(1).first, hypergraph.pins(1).second, 4), Eq(1));
}

TEST_F(AnUncontractionOperation, RestoresNumberOfPinsOnUncontraction) {
  hypergraph.uncontract(hypergraph.contract(3, 4));
  ASSERT_THAT(hypergraph.currentNumPins(), Eq(12));
}

TEST_F(AnUncontractionOperation, RestoresHyperedgeSizeOfHyperedgesAffectedByContraction) {
  hypergraph.uncontract(hypergraph.contract(0, 2));
  ASSERT_THAT(hypergraph.edgeSize(0), Eq(2));
}

TEST_F(AnUncontractedHypergraph, EqualsTheInitialHypergraphBeforeContraction) {
  std::vector<std::pair<HypernodeID, HypernodeID> > contractions { { 4, 6 }, { 3, 4 }, { 0, 2 },
                                                                   { 0, 1 }, { 0, 5 }, { 0, 3 } };
  std::stack<CoarseningMemento> contraction_history;
  HypergraphPruner hypergraph_pruner(modified_hypergraph.initialNumNodes());
  for (const auto& contraction : contractions) {
    contraction_history.emplace(modified_hypergraph.contract(contraction.first,
                                                             contraction.second));
    hypergraph_pruner.removeSingleNodeHyperedges(modified_hypergraph, contraction_history.top());
  }

  ASSERT_THAT(modified_hypergraph.nodeWeight(0), Eq(7));
  modified_hypergraph.setNodePart(0, 0);

  while (!contraction_history.empty()) {
    hypergraph_pruner.restoreSingleNodeHyperedges(modified_hypergraph,
                                                  contraction_history.top());
    modified_hypergraph.uncontract(contraction_history.top().contraction_memento);
    contraction_history.pop();
  }

  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(hypergraph, modified_hypergraph), Eq(true));
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
  hypergraph.removeEdge(2);
  ASSERT_THAT(hypergraph.initialNumEdges(), Eq(4));
}

TEST_F(AnUncontractionOperation, UpdatesPartitionIndexOfUncontractedNode) {
  ASSERT_THAT(hypergraph.partID(2), Eq(0));

  Memento memento = hypergraph.contract(0, 2);
  hypergraph.changeNodePart(0, 0, 1);
  hypergraph.uncontract(memento);

  ASSERT_THAT(hypergraph.partID(2), Eq(1));
}

TEST(AnUnconnectedHypernode, IsNotRemovedTogetherWithLastEdgeIfFlagIsFalse) {
  Hypergraph hypergraph(1, 1, HyperedgeIndexVector { 0,  /*sentinel*/ 1 },
                        HyperedgeVector { 0 });

  hypergraph.removeEdge(0);
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

  hypergraph.removeEdge(1);

  for (PartitionID part = 0; part < hypergraph._k; ++part) {
    // bypass pinCountInPart because of assertions
    const HypernodeID num_pins = hypergraph._pins_in_part[1 * hypergraph._k + part];
    ASSERT_THAT(num_pins, Eq(hypergraph.kInvalidCount));
  }
}

TEST_F(AHypergraph, RestoresInvalidatedPartitionPinCountsOnHyperedgeRestore) {
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(4));
  hypergraph.removeEdge(1);
  hypergraph.restoreEdge(1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(4));
}


TEST_F(AHypergraph, MaintainsPartitionPinCountsForRestoredSingleNodeOrParallelHEs) {
  hypergraph.removeEdge(1);

  // pseudo initial partitioning
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);

  hypergraph.restoreEdge(1);
  ASSERT_THAT(hypergraph.pinCountInPart(1, 1), Eq(3));
  ASSERT_THAT(hypergraph.pinCountInPart(1, 0), Eq(1));
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
  for (const HypernodeID& hn : hypergraph.nodes()) {
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
  hypergraph.initializeNumCutHyperedges();
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
  hypergraph.initializeNumCutHyperedges();
  ASSERT_THAT(hypergraph.connectivity(0), Eq(1));

  hypergraph.changeNodePart(2, 0, 1);

  ASSERT_THAT(hypergraph.connectivity(0), Eq(2));
}


TEST_F(AHypergraph, AllowsIterationOverConnectivitySetOfAHyperege) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(2, 1);
  ASSERT_THAT(hypergraph.connectivity(0), Eq(2));

  int part_id = 0;
  for (const PartitionID& part : hypergraph.connectivitySet(0)) {
    ASSERT_THAT(part, Eq(part_id));
    ++part_id;
  }
}

TEST(ConnectivitySets, AreCleardWhenSingleNodeHyperedgesAreRemoved) {
  Hypergraph hypergraph(1, 1, HyperedgeIndexVector { 0,  /*sentinel*/ 1 },
                        HyperedgeVector { 0 });
  hypergraph.setNodePart(0, 0);
  ASSERT_THAT(hypergraph.connectivity(0), Eq(1));
  ASSERT_THAT(*hypergraph.connectivitySet(0).begin(), Eq(0));

  hypergraph.removeEdge(0);
  hypergraph.changeNodePart(0, 0, 1);
  hypergraph.restoreEdge(0);

  ASSERT_THAT(hypergraph.connectivity(0), Eq(1));
  ASSERT_THAT(*hypergraph.connectivitySet(0).begin(), Eq(1));
}

TEST_F(AHypergraph, MaintainsCorrectPartSizesDuringUncontraction) {
  std::stack<Memento> mementos;
  mementos.push(hypergraph.contract(0, 1));
  mementos.push(hypergraph.contract(0, 3));
  mementos.push(hypergraph.contract(0, 4));
  mementos.push(hypergraph.contract(2, 5));
  mementos.push(hypergraph.contract(2, 6));
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.initializeNumCutHyperedges();
  ASSERT_THAT(hypergraph.partSize(0), Eq(1));
  ASSERT_THAT(hypergraph.partSize(1), Eq(1));

  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_THAT(hypergraph.partSize(1), Eq(2));
  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_THAT(hypergraph.partSize(1), Eq(3));
  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_THAT(hypergraph.partSize(0), Eq(2));
  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_THAT(hypergraph.partSize(0), Eq(3));
  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_THAT(hypergraph.partSize(0), Eq(4));
}

TEST_F(AHypergraph, MaintainsItsTotalWeight) {
  ASSERT_THAT(hypergraph.totalWeight(), Eq(7));
}

TEST_F(APartitionedHypergraph, CanBeDecomposedIntoHypergraphs) {
  hypergraph._communities[0] = 0;
  hypergraph._communities[1] = 2;
  hypergraph._communities[2] = 4;
  hypergraph._communities[3] = 6;
  hypergraph._communities[4] = 8;
  hypergraph._communities[5] = 10;
  hypergraph._communities[6] = 12;

  auto extr_part0 = extractPartAsUnpartitionedHypergraphForBisection(hypergraph, 0, Objective::cut);
  auto extr_part1 = extractPartAsUnpartitionedHypergraphForBisection(hypergraph, 1, Objective::cut);

  Hypergraph& part0_hypergraph = *extr_part0.first;
  Hypergraph& part1_hypergraph = *extr_part1.first;

  ASSERT_THAT(part0_hypergraph.initialNumNodes(), Eq(4));
  ASSERT_THAT(part1_hypergraph.initialNumNodes(), Eq(3));

  ASSERT_THAT(part0_hypergraph.currentNumEdges(), Eq(1));
  ASSERT_THAT(part1_hypergraph.currentNumEdges(), Eq(1));

  ASSERT_THAT(part0_hypergraph.edgeSize(0), Eq(4));
  ASSERT_THAT(part1_hypergraph.edgeSize(0), Eq(3));

  const std::vector<HypernodeID>& mapping_0 = extr_part0.second;
  const std::vector<HypernodeID>& mapping_1 = extr_part1.second;

  ASSERT_THAT(mapping_0, ContainerEq(std::vector<HypernodeID>{ 0, 1, 3, 4 }));
  ASSERT_THAT(mapping_1, ContainerEq(std::vector<HypernodeID>{ 2, 5, 6 }));

  ASSERT_THAT(part0_hypergraph._communities, ContainerEq(std::vector<PartitionID>{ 0, 2, 6, 8 }));
  ASSERT_THAT(part1_hypergraph._communities, ContainerEq(std::vector<PartitionID>{ 4, 10, 12 }));
}

TEST_F(AHypergraph, WithOnePartitionEqualsTheExtractedHypergraphExceptForPartitionRelatedInfos) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 0);
  hypergraph.setNodePart(6, 0);

  hypergraph._communities[0] = 0;
  hypergraph._communities[1] = 2;
  hypergraph._communities[2] = 4;
  hypergraph._communities[3] = 6;
  hypergraph._communities[4] = 8;
  hypergraph._communities[5] = 10;
  hypergraph._communities[6] = 12;

  auto extr_part0 = extractPartAsUnpartitionedHypergraphForBisection(hypergraph, 0, Objective::cut);
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(hypergraph, *extr_part0.first), Eq(true));
}

TEST_F(AHypergraph, ExtractedFromAPartitionedHypergraphHasInitializedPartitionInformation) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 0);
  hypergraph.setNodePart(6, 0);
  auto extr_part0 = extractPartAsUnpartitionedHypergraphForBisection(hypergraph, 0, Objective::cut);

  ASSERT_THAT(extr_part0.first->_part_info.size(), Eq(2));
  ASSERT_THAT(extr_part0.first->_pins_in_part.size(), Eq(8));

  for (const HyperedgeID& he : extr_part0.first->edges()) {
    ASSERT_THAT(extr_part0.first->connectivity(he), Eq(0));
  }

  for (const HypernodeID& hn : extr_part0.first->nodes()) {
    ASSERT_THAT(extr_part0.first->partID(hn), Eq(-1));
  }
}


TEST_F(AHypergraph, ExtractedFromAPartitionedHypergraphHasCorrectNumberOfHyperedges) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 0);
  auto extr_part0 = extractPartAsUnpartitionedHypergraphForBisection(hypergraph, 0, Objective::cut);
  ASSERT_THAT(extr_part0.first->currentNumEdges(), Eq(2));
}

TEST_F(AHypergraph, ExtractedFromAPartitionedHypergraphHasCorrectNumberOfHypernodes) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 0);
  auto extr_part0 = extractPartAsUnpartitionedHypergraphForBisection(hypergraph, 0, Objective::cut);
  ASSERT_THAT(extr_part0.first->initialNumNodes(), Eq(5));
}

TEST_F(AHypergraph, CanBeDecomposedIntoHypergraphsUsingCutNetSplitting) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  auto extr_part0 = extractPartAsUnpartitionedHypergraphForBisection(hypergraph, 0, Objective::km1);
  auto extr_part1 = extractPartAsUnpartitionedHypergraphForBisection(hypergraph, 1, Objective::km1);
  Hypergraph& part0_hypergraph = *extr_part0.first;
  Hypergraph& part1_hypergraph = *extr_part1.first;

  ASSERT_THAT(part0_hypergraph.initialNumNodes(), Eq(4));
  ASSERT_THAT(part1_hypergraph.initialNumNodes(), Eq(3));

  ASSERT_THAT(part0_hypergraph.currentNumEdges(), Eq(2));
  ASSERT_THAT(part1_hypergraph.currentNumEdges(), Eq(1));

  ASSERT_THAT(part0_hypergraph.edgeSize(0), Eq(4));
  ASSERT_THAT(part0_hypergraph.edgeSize(1), Eq(2));
  ASSERT_THAT(part1_hypergraph.edgeSize(0), Eq(3));

  const std::vector<HypernodeID>& mapping_0 = extr_part0.second;
  const std::vector<HypernodeID>& mapping_1 = extr_part1.second;

  ASSERT_THAT(mapping_0, ContainerEq(std::vector<HypernodeID>{ 0, 1, 3, 4 }));
  ASSERT_THAT(mapping_1, ContainerEq(std::vector<HypernodeID>{ 2, 5, 6 }));
}

TEST_F(AHypergraph, CreatedViaReindexingIsACopyOfTheOriginalHypergraph) {
  verifyEquivalenceWithoutPartitionInfo(hypergraph, *reindex(hypergraph).first);
}

TEST_F(AHypergraph, WithContractedHypernodesCanBeReindexed) {
  hypergraph.contract(1, 4);
  hypergraph.contract(0, 2);
  hypergraph.removeEdge(1);

  hypergraph._communities[0] = 0;
  hypergraph._communities[1] = 2;
  hypergraph._communities[2] = 4;
  hypergraph._communities[3] = 6;
  hypergraph._communities[4] = 8;
  hypergraph._communities[5] = 10;
  hypergraph._communities[6] = 12;

  auto reindexed = reindex(hypergraph);

  ASSERT_THAT(reindexed.first->initialNumNodes(), Eq(5));
  ASSERT_THAT(reindexed.first->currentNumEdges(), Eq(3));
  ASSERT_THAT(reindexed.second.size(), Eq(5));
  ASSERT_THAT(reindexed.second, ContainerEq(std::vector<HypernodeID>{ 0, 1, 3, 5, 6 }));
  ASSERT_THAT(reindexed.first->_communities,
              ContainerEq(std::vector<PartitionID>{ 0, 2, 6, 10, 12 }));
}

TEST_F(APartitionedHypergraph, CanBeResetToUnpartitionedState) {
  hypergraph.resetPartitioning();
  ASSERT_THAT(verifyEquivalenceWithPartitionInfo(hypergraph, original_hypergraph), Eq(true));
}


TEST_F(APartitionedHypergraph, IdentifiesBorderHypernodes) {
  hypergraph.initializeNumCutHyperedges();
  ASSERT_THAT(hypergraph.isBorderNode(0), Eq(true));
  ASSERT_THAT(hypergraph.isBorderNode(1), Eq(false));
  ASSERT_THAT(hypergraph.isBorderNode(2), Eq(true));
  ASSERT_THAT(hypergraph.isBorderNode(3), Eq(true));
  ASSERT_THAT(hypergraph.isBorderNode(4), Eq(true));
  ASSERT_THAT(hypergraph.isBorderNode(5), Eq(false));
  ASSERT_THAT(hypergraph.isBorderNode(6), Eq(true));
}

TEST_F(AHypergraph, StoresCorrectNumberOfFixedVertices) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(6, 1);

  ASSERT_THAT(hypergraph.numFixedVertices(), Eq(3));
}

TEST_F(AHypergraph, IndicatesCorrectFixedVertices) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(6, 1);

  ASSERT_TRUE(hypergraph.isFixedVertex(0));
  ASSERT_TRUE(hypergraph.isFixedVertex(1));
  ASSERT_FALSE(hypergraph.isFixedVertex(2));
  ASSERT_FALSE(hypergraph.isFixedVertex(3));
  ASSERT_FALSE(hypergraph.isFixedVertex(4));
  ASSERT_FALSE(hypergraph.isFixedVertex(5));
  ASSERT_TRUE(hypergraph.isFixedVertex(6));
}

TEST_F(AHypergraph, StoresCorrectFixedVertexTotalWeight) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(6, 1);

  ASSERT_THAT(hypergraph.fixedVertexTotalWeight(), Eq(3));
}

TEST_F(AHypergraph, IteratesOverCorrectFixedVertices) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(6, 1);

  ASSERT_THAT(hypergraph.numFixedVertices(), Eq(3));
  std::vector<HypernodeID> fixed_vertices = { 0, 1, 6 };
  size_t i = 0;
  for (const HypernodeID& hn : hypergraph.fixedVertices()) {
    ASSERT_EQ(hn, fixed_vertices[i++]);
  }
}

TEST_F(AHypergraph, StoresCorrectFixedVertexPartIDs) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(6, 1);

  ASSERT_THAT(hypergraph.numFixedVertices(), Eq(3));
  std::vector<HypernodeID> fixed_vertices_part_id = { 0, 0, 1 };
  size_t i = 0;
  for (const HypernodeID& hn : hypergraph.fixedVertices()) {
    ASSERT_EQ(hypergraph.fixedVertexPartID(hn), fixed_vertices_part_id[i++]);
  }
}

TEST_F(AHypergraph, StoresCorrectFixedVertexPartWeights) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(6, 1);

  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 2);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 1);
}

TEST_F(AHypergraph, UpdatesFixedVertexPartWeightsAfterContraction) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(6, 1);

  hypergraph.contract(0, 2);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 3);

  hypergraph.contract(6, 5);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 2);
}

TEST_F(AHypergraph, UpdatesFixedVertexPartWeightsAfterFixedVertexContraction) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(5, 1);
  hypergraph.setFixedVertex(6, 1);

  ASSERT_EQ(hypergraph.numFixedVertices(), 4);

  hypergraph.contract(0, 1);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 2);
  ASSERT_EQ(hypergraph.numFixedVertices(), 3);

  hypergraph.contract(6, 5);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 2);
  ASSERT_EQ(hypergraph.numFixedVertices(), 2);
}

TEST_F(AHypergraph, UpdatesFixedVertexPartWeightsAfterUncontraction) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(6, 1);

  std::stack<Memento> mementos;
  mementos.push(hypergraph.contract(0, 2));
  mementos.push(hypergraph.contract(6, 5));
  mementos.push(hypergraph.contract(1, 3));
  mementos.push(hypergraph.contract(6, 4));

  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(6, 1);
  hypergraph.initializeNumCutHyperedges();

  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 4);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 3);

  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 2);

  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 3);

  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 1);

  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 2);
}

TEST_F(AHypergraph, UpdatesFixedVertexPartWeightsAfterFixedVertexUncontraction) {
  hypergraph.setFixedVertex(0, 0);
  hypergraph.setFixedVertex(1, 0);
  hypergraph.setFixedVertex(5, 1);
  hypergraph.setFixedVertex(6, 1);

  std::stack<Memento> mementos;
  mementos.push(hypergraph.contract(0, 1));
  mementos.push(hypergraph.contract(6, 5));

  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(6, 1);
  hypergraph.initializeNumCutHyperedges();

  ASSERT_EQ(hypergraph.numFixedVertices(), 2);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 2);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 2);

  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_EQ(hypergraph.numFixedVertices(), 3);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(1), 2);

  hypergraph.uncontract(mementos.top());
  mementos.pop();
  ASSERT_EQ(hypergraph.numFixedVertices(), 4);
  ASSERT_EQ(hypergraph.fixedVertexPartWeight(0), 2);
}


static std::vector<HyperedgeID> getIncidentEdges(const Hypergraph& hypergraph,
                                                 const HypernodeID hn) {
  std::vector<HyperedgeID> temp;
  std::copy(hypergraph.incidentEdges(hn).first,
            hypergraph.incidentEdges(hn).second,
            std::back_inserter(temp));
  return temp;
}


static std::vector<HypernodeID> getPins(const Hypergraph& hypergraph,
                                        const HypernodeID he) {
  std::vector<HyperedgeID> temp;
  std::copy(hypergraph.pins(he).first,
            hypergraph.pins(he).second,
            std::back_inserter(temp));
  return temp;
}

TEST_F(AHypergraph, CanBeConvertedToDualHypergraph) {
  hypergraph.setNodeWeight(2, 42);
  hypergraph.setEdgeWeight(3, 23);
  hypergraph.setType(Hypergraph::Type::EdgeAndNodeWeights);

  Hypergraph dual_hypergraph = constructDualHypergraph(hypergraph);

  ASSERT_EQ(dual_hypergraph.edgeWeight(2), 42);
  ASSERT_EQ(dual_hypergraph.nodeWeight(3), 23);

  ASSERT_THAT(getIncidentEdges(dual_hypergraph, 0),
              ContainerEq(getPins(hypergraph, 0)));
  ASSERT_THAT(getIncidentEdges(dual_hypergraph, 1),
              ContainerEq(getPins(hypergraph, 1)));
  ASSERT_THAT(getIncidentEdges(dual_hypergraph, 2),
              ContainerEq(getPins(hypergraph, 2)));
  ASSERT_THAT(getIncidentEdges(dual_hypergraph, 3),
              ContainerEq(getPins(hypergraph, 3)));

  ASSERT_THAT(getPins(dual_hypergraph, 0),
              ContainerEq(getIncidentEdges(hypergraph, 0)));
  ASSERT_THAT(getPins(dual_hypergraph, 1),
              ContainerEq(getIncidentEdges(hypergraph, 1)));
  ASSERT_THAT(getPins(dual_hypergraph, 2),
              ContainerEq(getIncidentEdges(hypergraph, 2)));
  ASSERT_THAT(getPins(dual_hypergraph, 3),
              ContainerEq(getIncidentEdges(hypergraph, 3)));
  ASSERT_THAT(getPins(dual_hypergraph, 4),
              ContainerEq(getIncidentEdges(hypergraph, 4)));
  ASSERT_THAT(getPins(dual_hypergraph, 5),
              ContainerEq(getIncidentEdges(hypergraph, 5)));
  ASSERT_THAT(getPins(dual_hypergraph, 6),
              ContainerEq(getIncidentEdges(hypergraph, 6)));
}

void assignCommunities(Hypergraph& hg,
                       const std::vector<std::vector<HypernodeID>>& communities) {
  PartitionID current_community = 0;
  for ( const auto& community : communities ) {
    for ( const HypernodeID& hn : community ) {
      hg.setNodeCommunity(hn, current_community);
    } 
    current_community++;
  }
}

TEST_F(AHypergraph, ExtractsCommunityZeroAsSectionHypergraph) {
  assignCommunities(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});

  Hypergraph expected_hg(7, 3, HyperedgeIndexVector { 0, 2, 6, 9 },
                        HyperedgeVector { 0, 2, 0, 1, 3, 4, 2, 5, 6 });
  assignCommunities(expected_hg, {{0, 1, 2}, {3, 4}, {5, 6}});
  std::vector<HypernodeID> expected_hn_mapping = {0, 1, 2, 3, 4, 5, 6};
  std::vector<CommunityHyperedge<Hypergraph>> expected_he_mapping =
    { {0, 0, 2}, {1, 0, 2}, {3, 0, 1} };

  auto extr_comm0 = extractCommunityInducedSectionHypergraph(hypergraph, 0, true);
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(expected_hg, *extr_comm0.subhypergraph), Eq(true));
  ASSERT_THAT(extr_comm0.subhypergraph_to_hypergraph_hn, Eq(expected_hn_mapping));
  ASSERT_THAT(extr_comm0.subhypergraph_to_hypergraph_he, Eq(expected_he_mapping));
  ASSERT_THAT(extr_comm0.num_hn_not_in_community, Eq(4));
  ASSERT_THAT(extr_comm0.num_pins_not_in_community, Eq(4));
}

TEST_F(AHypergraph, ExtractsCommunityOneAsSectionHypergraph) {
  assignCommunities(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});

  Hypergraph expected_hg(5, 2, HyperedgeIndexVector { 0, 4, 7 },
                         HyperedgeVector { 0, 1, 2, 3, 2, 3, 4 });
  assignCommunities(expected_hg, {{0, 1}, {2, 3}, {4}});
  std::vector<HypernodeID> expected_hn_mapping = {0, 1, 3, 4, 6};
  std::vector<CommunityHyperedge<Hypergraph>> expected_he_mapping =
    { {1, 2, 4}, {2, 0, 2} };

  auto extr_comm1 = extractCommunityInducedSectionHypergraph(hypergraph, 1, true); 
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(expected_hg, *extr_comm1.subhypergraph), Eq(true));
  ASSERT_THAT(extr_comm1.subhypergraph_to_hypergraph_hn, Eq(expected_hn_mapping));
  ASSERT_THAT(extr_comm1.subhypergraph_to_hypergraph_he, Eq(expected_he_mapping));
  ASSERT_THAT(extr_comm1.num_hn_not_in_community, Eq(3));
  ASSERT_THAT(extr_comm1.num_pins_not_in_community, Eq(3));
}

TEST_F(AHypergraph, ExtractsCommunityTwoAsSectionHypergraph) {
  assignCommunities(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});

  Hypergraph expected_hg(5, 2, HyperedgeIndexVector { 0, 3, 6 },
                         HyperedgeVector { 1, 2, 4, 0, 3, 4 });
  assignCommunities(expected_hg, {{0}, {1, 2}, {3, 4}});
  std::vector<HypernodeID> expected_hn_mapping = {2, 3, 4, 5, 6};
  std::vector<CommunityHyperedge<Hypergraph>> expected_he_mapping =
    { {2, 2, 3}, {3, 1, 3} };

  auto extr_comm2 = extractCommunityInducedSectionHypergraph(hypergraph, 2, true); 
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(expected_hg, *extr_comm2.subhypergraph), Eq(true));
  ASSERT_THAT(extr_comm2.subhypergraph_to_hypergraph_hn, Eq(expected_hn_mapping));
  ASSERT_THAT(extr_comm2.subhypergraph_to_hypergraph_he, Eq(expected_he_mapping));
  ASSERT_THAT(extr_comm2.num_hn_not_in_community, Eq(3));
  ASSERT_THAT(extr_comm2.num_pins_not_in_community, Eq(3));
}

Hypergraph::ContractionMemento contract(CommunitySubhypergraph<Hypergraph>& community,
                                        const HypernodeID u, const HypernodeID v) {
  using Memento = typename Hypergraph::ContractionMemento;
  Memento memento = community.subhypergraph->contract(u, v);
  return { community.subhypergraph_to_hypergraph_hn[memento.u], 
           community.subhypergraph_to_hypergraph_hn[memento.v] };
}

void assingRandomParition(Hypergraph& hg1, Hypergraph& hg2) {
  ASSERT(hg1.currentNumNodes() == hg2.currentNumNodes());
  for ( const HypernodeID& hn : hg1.nodes() ) {
    PartitionID part = hn % 2;
    hg1.setNodePart(hn, part);
    hg2.setNodePart(hn, part);
  }
  hg1.initializeNumCutHyperedges();
  hg2.initializeNumCutHyperedges();
}

struct CommunityContraction {
  const PartitionID community;
  const HypernodeID u;
  const HypernodeID v;
};

void verifyCommunityMergeStep(Hypergraph& hypergraph,
                              const std::vector<std::vector<HypernodeID>>& community_ids,
                              const std::vector<CommunityContraction>& contractions = {},
                              const std::vector<std::pair<PartitionID, HyperedgeID>>& disabled_hes = {}) {
  using CommunitySubhypergraph = CommunitySubhypergraph<Hypergraph>;
  using Memento = typename Hypergraph::ContractionMemento;
  
  assignCommunities(hypergraph, community_ids);
  std::vector<CommunitySubhypergraph> communities;
  communities.emplace_back(extractCommunityInducedSectionHypergraph(hypergraph, 0, true));
  communities.emplace_back(extractCommunityInducedSectionHypergraph(hypergraph, 1, true));
  communities.emplace_back(extractCommunityInducedSectionHypergraph(hypergraph, 2, true));

  std::vector<Memento> history;
  std::vector<Memento> original_hg_history;
  for ( const CommunityContraction& contraction : contractions ) {
    Memento memento = contract(communities[contraction.community], contraction.u, contraction.v);
    history.push_back(memento);
    original_hg_history.push_back(hypergraph.contract(memento.u, memento.v));
  }

  std::vector<HyperedgeID> original_disabled_hes;
  for ( const auto& disabled_he : disabled_hes ) {
    const PartitionID community_id = disabled_he.first;
    const HyperedgeID he = disabled_he.second;
    const HyperedgeID original_he = communities[community_id].subhypergraph_to_hypergraph_he[he].original_he;
    communities[community_id].subhypergraph->removeEdge(he);
    hypergraph.removeEdge(original_he);
    original_disabled_hes.push_back(original_he);
  }

  Hypergraph merged_hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                              HyperedgeVector { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
  assignCommunities(merged_hypergraph, community_ids);
  mergeCommunityInducedSectionHypergraphs(merged_hypergraph, communities, history);
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(hypergraph, merged_hypergraph), Eq(true));

  for ( const HyperedgeID& he : original_disabled_hes ) {
    hypergraph.restoreEdge(he);
    merged_hypergraph.restoreEdge(he);
  }

  assingRandomParition(hypergraph, merged_hypergraph);
  for ( int i = history.size() - 1; i >= 0; --i ) {
    merged_hypergraph.uncontract(history[i]);
    hypergraph.uncontract(original_hg_history[i]);
    ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(hypergraph, merged_hypergraph), Eq(true));
  }
}

TEST_F(AHypergraph, MergesThreeCommunitySubhypergraphs) {
  verifyCommunityMergeStep(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});
}

TEST_F(AHypergraph, MergesThreeCommunitySubhypergraphsWithOneContraction) {
  verifyCommunityMergeStep(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}});
}

TEST_F(AHypergraph, MergesThreeCommunitySubhypergraphsWithTwoContractions) {
  verifyCommunityMergeStep(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}, {2, 3, 4}});
}

TEST_F(AHypergraph, MergesThreeCommunitySubhypergraphsWithThreeContractions) {
  verifyCommunityMergeStep(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}, {1, 2, 3}, {2, 3, 4}});
}

TEST_F(AHypergraph, MergesThreeCommunitySubhypergraphsWithFourContractions) {
  verifyCommunityMergeStep(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}, {1, 2, 3}, {0, 1, 0}, {2, 3, 4}});
}

TEST_F(AHypergraph, MergesThreeCommunitySubhypergraphsWithFourContractionsWithOneDisabledHyperedge) {
  verifyCommunityMergeStep(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}, {1, 2, 3}, {0, 1, 0}, {2, 3, 4}},
    {{0, 0}});
}

TEST(Hypergraphs, CanBeStrippedOfAllParallelHyperedges) {
  Hypergraph hypergraph(5, 7, HyperedgeIndexVector { 0, 1, 4, 6, 10, 13, 14, 17 },
                        HyperedgeVector { 0, 1, 2, 3, 0, 1, 1, 2, 3, 4, 1, 2, 3, 0, 1, 2, 3 });

  // hypergraph.printGraphState();

  auto removed_parallel_hes = removeParallelHyperedges(hypergraph);

  // hypergraph.printGraphState();

  ASSERT_EQ(hypergraph.currentNumEdges(), 4);
  ASSERT_EQ(hypergraph.edgeWeight(0), 2);
  ASSERT_EQ(hypergraph.edgeWeight(1), 3);
  ASSERT_EQ(hypergraph.edgeWeight(2), 1);
  ASSERT_EQ(hypergraph.edgeWeight(3), 1);
}

TEST(Hypergraphs, CanBeStrippedOfAllIdenticalVertices) {
  Hypergraph hypergraph(7, 2, HyperedgeIndexVector { 0, 5, 10 },
                        HyperedgeVector { 6, 1, 0, 2, 5, 3, 5, 4, 0, 6 });

  ASSERT_EQ(hypergraph.currentNumNodes(), 7);
  ASSERT_EQ(hypergraph.edgeSize(0), 5);
  ASSERT_EQ(hypergraph.edgeSize(1), 5);

  auto removed_identical_nodes = removeIdenticalNodes(hypergraph);

  ASSERT_EQ(hypergraph.currentNumNodes(), 3);
  ASSERT_EQ(hypergraph.edgeSize(0), 2);
  ASSERT_EQ(hypergraph.edgeSize(1), 2);
}

}  // namespace ds
}  // namespace kahypar
