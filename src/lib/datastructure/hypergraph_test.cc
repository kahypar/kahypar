#include <iostream>
#include <stack>

#include "gmock/gmock.h"

#include "../definitions.h"
#include "Hypergraph.h"

namespace hgr {

using ::testing::Eq;
using ::testing::Test;

typedef hgr::Hypergraph<HyperNodeID,HyperEdgeID,HyperNodeWeight,HyperEdgeWeight> HypergraphType;
typedef HypergraphType::ConstIncidenceIterator ConstIncidenceIterator;
typedef HypergraphType::ConstHypernodeIterator ConstHypernodeIterator;
typedef HypergraphType::ConstHyperedgeIterator ConstHyperedgeIterator;
typedef HypergraphType::HypernodeID HypernodeID;
typedef HypergraphType::HyperedgeID HyperedgeID;
typedef HypergraphType::ContractionMemento Memento;

class AHypergraph : public Test {
 public:
  AHypergraph() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}, nullptr, nullptr) {}
  HypergraphType hypergraph;
};

class AHypernodeIterator : public AHypergraph {
 public:
  AHypernodeIterator() :
      AHypergraph(),
      begin(),
      end() {}

  ConstHypernodeIterator begin;
  ConstHypernodeIterator end;
};

class AHyperedgeIterator : public AHypergraph {
 public:
  AHyperedgeIterator() :
      AHypergraph(),
      begin(),
      end() {}

  ConstHyperedgeIterator begin;
  ConstHyperedgeIterator end;
};

class AHypergraphMacro : public AHypergraph {
 public:
  AHypergraphMacro() : AHypergraph() {}
};

class AContractionMemento : public AHypergraph {
 public:
  AContractionMemento() : AHypergraph() {}
};

class AnUncontractionOperation : public AHypergraph {
 public:
  AnUncontractionOperation() : AHypergraph() {}
};

class AnUncontractedHypergraph : public AHypergraph {
 public:
  AnUncontractedHypergraph() :
      AHypergraph(),
      modified_hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                          hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}, nullptr, nullptr) {}

  HypergraphType modified_hypergraph;
};

TEST_F(AHypergraph, InitializesInternalHypergraphRepresentation) {
  ASSERT_THAT(hypergraph.numHypernodes(), Eq(7));
  ASSERT_THAT(hypergraph.numHyperdeges(), Eq(4));
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
  ASSERT_THAT(hypergraph.hypernode(0).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(1).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(2).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(3).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(4).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(5).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(6).size(), Eq(2));

  ASSERT_THAT(hypergraph.hyperedge(0).size(), Eq(2));
  ASSERT_THAT(hypergraph.hyperedge(1).size(), Eq(4));
  ASSERT_THAT(hypergraph.hyperedge(2).size(), Eq(3));
  ASSERT_THAT(hypergraph.hyperedge(3).size(), Eq(3));
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
  ASSERT_THAT(hypergraph.numHypernodes(), Eq(7));
  hypergraph.removeHypernode(6);
  ASSERT_THAT(hypergraph.numHypernodes(), Eq(6));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHypernodeRemoval) {
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
  hypergraph.removeHypernode(6);
  ASSERT_THAT(hypergraph.numPins(), Eq(10));
}

TEST_F(AHypergraph, DecrementsSizeOfAffectedHyperedgesOnHypernodeRemoval) {
  ASSERT_THAT(hypergraph.hyperedgeSize(3), Eq(3));
  ASSERT_THAT(hypergraph.hyperedgeSize(2), Eq(3));
  hypergraph.removeHypernode(6);
  ASSERT_THAT(hypergraph.hyperedgeSize(3), Eq(2));
  ASSERT_THAT(hypergraph.hyperedgeSize(2), Eq(2));
}

TEST_F(AHypergraph, InvalidatesRemovedHypernode) {  
  ASSERT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(false));
  hypergraph.removeHypernode(6);
  ASSERT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, DecrementsNumberOfHyperedgesOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.numHyperdeges(), Eq(4));
  hypergraph.removeHyperedge(2);
  ASSERT_THAT(hypergraph.numHyperdeges(), Eq(3));
}

TEST_F(AHypergraph, InvalidatesRemovedHyperedge) {
  ASSERT_THAT(hypergraph.hyperedge(2).isInvalid(), Eq(false));
  hypergraph.removeHyperedge(2);
  ASSERT_THAT(hypergraph.hyperedge(2).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, DecrementsNumberOfPinsOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.numPins(), Eq(12));
  hypergraph.removeHyperedge(2);
  ASSERT_THAT(hypergraph.numPins(), Eq(9));
}

TEST_F(AHypergraph, DecrementsHypernodeDegreeOfAffectedHypernodesOnHyperedgeRemoval) {
  ASSERT_THAT(hypergraph.hypernode(3).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(4).size(), Eq(2));
  ASSERT_THAT(hypergraph.hypernode(6).size(), Eq(2));
  hypergraph.removeHyperedge(2);
  ASSERT_THAT(hypergraph.hypernode(3).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(4).size(), Eq(1));
  ASSERT_THAT(hypergraph.hypernode(6).size(), Eq(1));
}

TEST_F(AHypergraph, DecrementsHypernodeDegreeAfterDisconnectingAHypernodeFromHyperedge) {
  ASSERT_THAT(hypergraph.hypernodeDegree(4), Eq(2));
  hypergraph.disconnect(4, 2);
  ASSERT_THAT(hypergraph.hypernodeDegree(4), Eq(1));
}

TEST_F(AHypergraph, DecrementsHyperedgeSizeAfterDisconnectingAHypernodeFromHyperedge) {
  ASSERT_THAT(hypergraph.hyperedgeSize(2), Eq(3));
  hypergraph.disconnect(4, 2);
  ASSERT_THAT(hypergraph.hyperedgeSize(2), Eq(2));
}

TEST_F(AHypergraph, DoesNotInvalidateHypernodeAfterDisconnectingFromHyperedge) {
  ASSERT_THAT(hypergraph.hypernode(4).isInvalid(), Eq(false));
  hypergraph.disconnect(4, 2);
  ASSERT_THAT(hypergraph.hypernode(4).isInvalid(), Eq(false));
}

TEST_F(AHypergraph, InvalidatesContractedHypernode) {
  ASSERT_THAT(hypergraph.hypernode(2).isInvalid(), Eq(false));
  hypergraph.contract(0,2);
  ASSERT_THAT(hypergraph.hypernode(2).isInvalid(), Eq(true));
}

TEST_F(AHypergraph, RelinksHyperedgesOfContractedHypernodeToRepresentative) {
  ASSERT_THAT(hypergraph.hypernodeDegree(0), Eq(2));
  hypergraph.contract(0,2);
  hypergraph.contract(0,4);
  ASSERT_THAT(hypergraph.hypernodeDegree(0), Eq(4));
}

TEST_F(AHypergraph, AddsHypernodeWeightOfContractedNodeToRepresentative) {
  ASSERT_THAT(hypergraph.hypernodeWeight(0), Eq(1));
  hypergraph.contract(0,2);
  ASSERT_THAT(hypergraph.hypernodeWeight(0), Eq(2));
}

TEST_F(AHypergraph, ReducesHyperedgeSizeOfHyperedgesAffectedByContraction) {
  ASSERT_THAT(hypergraph.hyperedgeSize(0), Eq(2));
  hypergraph.contract(0,2);
  ASSERT_THAT(hypergraph.hyperedgeSize(0), Eq(1));
}

TEST_F(AHypergraph, DoesNotRemoveParallelHyperedgesOnContraction) {
  ASSERT_THAT(hypergraph.hypernodeDegree(0), Eq(2));
  hypergraph.contract(5,6);
  hypergraph.contract(0,5);
  ASSERT_THAT(hypergraph.hypernodeDegree(0), Eq(4));
  ASSERT_THAT(hypergraph.hyperedge(0).isInvalid(), Eq(false));
  ASSERT_THAT(hypergraph.hyperedge(3).isInvalid(), Eq(false));
  ASSERT_THAT(hypergraph.hyperedge(0).weight(), Eq(1));
  ASSERT_THAT(hypergraph.hyperedge(3).weight(), Eq(1));
}

TEST_F(AHypergraph, DoesNotRemoveHyperedgesOfSizeOneOnContraction) {
  hypergraph.contract(0,2);
  ASSERT_THAT(hypergraph.hyperedgeSize(0), Eq(1));
  
  ASSERT_THAT(hypergraph.hyperedge(0).isInvalid(), Eq(false));
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
  ASSERT_THAT(*begin, Eq(3));
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
  ASSERT_THAT(*begin, Eq(3));
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
  ASSERT_THAT(*he_iter, Eq(*(hypergraph._incidence_array.begin() +
                             hypergraph.hypernode(6).firstEntry() + i)));
    ++i;
  } endfor
}

TEST_F(AHypergraphMacro, IteratesOverAllPinsOfAHyperedge) {
  ConstIncidenceIterator pin_iter;
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
  HypernodeID v_offset = hypergraph.hypernode(v_id).firstEntry();
  HypernodeID v_size = hypergraph.hypernode(v_id).size();

  Memento memento = hypergraph.contract(4,6);

  ASSERT_THAT(memento.u, Eq(u_id));
  ASSERT_THAT(memento.u_first_entry, Eq(u_offset));
  ASSERT_THAT(memento.u_size, Eq(u_size));
  ASSERT_THAT(memento.v, Eq(v_id));
  ASSERT_THAT(memento.v_first_entry, Eq(v_offset));
  ASSERT_THAT(memento.v_size, Eq(v_size));
}

TEST_F(AnUncontractionOperation, NeedsAContractionMementoAsInput) {
  Memento memento = hypergraph.contract(4,6);
  hypergraph.uncontract(memento);
}

TEST_F(AnUncontractionOperation, ReEnablesTheInvalidatedHypernode) {
  Memento memento = hypergraph.contract(4,6);
  ASSERT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(true));

  hypergraph.uncontract(memento);
  
  ASSERT_THAT(hypergraph.hypernode(6).isInvalid(), Eq(false));
}

TEST_F(AnUncontractionOperation, ResetsWeightOfRepresentative) {
  ASSERT_THAT(hypergraph.hypernodeWeight(4), Eq(1));
  Memento memento = hypergraph.contract(4,6);
  ASSERT_THAT(hypergraph.hypernodeWeight(4), Eq(2));
  
  hypergraph.uncontract(memento);
  
  ASSERT_THAT(hypergraph.hypernodeWeight(4), Eq(1));
}

TEST_F(AnUncontractionOperation, DisconnectsHyperedgesAddedToRepresenativeDuringContraction) {
  ASSERT_THAT(hypergraph.hypernodeDegree(4), Eq(2));
  Memento memento = hypergraph.contract(4,6);
  ASSERT_THAT(hypergraph.hypernodeDegree(4), Eq(3));

  hypergraph.uncontract(memento);
  ASSERT_THAT(hypergraph.hypernodeDegree(4), Eq(2));
}

TEST_F(AnUncontractionOperation, DeletesIncidenceInfoAddedDuringContraction) {
  ASSERT_THAT(hypergraph._incidence_array.size(), Eq(24));
  Memento memento = hypergraph.contract(4,6);
  ASSERT_THAT(hypergraph._incidence_array.size(), Eq(27));

  hypergraph.uncontract(memento);
  ASSERT_THAT(hypergraph._incidence_array.size(), Eq(24));
}

TEST_F(AnUncontractionOperation, RestoresIncidenceInfoForHyperedgesAddedToRepresentative) {
  ConstIncidenceIterator begin, end;
  std::tie(begin, end) = hypergraph.pins(3);
  ASSERT_THAT(std::count(begin, end, 6), Eq(1));
  std::tie(begin, end) = hypergraph.pins(2);
  ASSERT_THAT(std::count(begin, end, 6), Eq(1));
  Memento memento = hypergraph.contract(4,6);
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
  ConstIncidenceIterator begin, end;
  std::tie(begin, end) = hypergraph.pins(2);
  ASSERT_THAT(std::count(begin, end, 4), Eq(1));
  std::tie(begin, end) = hypergraph.pins(1);
  ASSERT_THAT(std::count(begin, end, 4), Eq(1));
  Memento memento = hypergraph.contract(3,4);
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

TEST_F(AnUncontractedHypergraph, EqualsTheInitialHypergraphBeforeContraction) {
  std::stack<Memento> contraction_history;
  contraction_history.emplace(modified_hypergraph.contract(4,6));
  contraction_history.emplace(modified_hypergraph.contract(3,4));
  contraction_history.emplace(modified_hypergraph.contract(0,2));
  contraction_history.emplace(modified_hypergraph.contract(0,1));
  contraction_history.emplace(modified_hypergraph.contract(0,5));
  contraction_history.emplace(modified_hypergraph.contract(0,3));
  ASSERT_THAT(modified_hypergraph.hypernodeWeight(0), Eq(7));

  while (!contraction_history.empty()) {
    modified_hypergraph.uncontract(contraction_history.top());
    contraction_history.pop();
  }
  
  ASSERT_THAT(verifyEquivalence(hypergraph, modified_hypergraph), Eq(true));
}
  
} // namespace hgr
