/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph.h"
#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/refinement/HyperedgeFMRefiner.h"

using::testing::Test;
using::testing::Eq;

using datastructure::HypernodeWeightVector;

using defs::INVALID_PARTITION;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;
using datastructure::HyperedgeWeight;
using datastructure::HyperedgeID;

namespace partition {
class AHyperedgeFMRefiner : public Test {
  public:
  AHyperedgeFMRefiner() :
    hypergraph(nullptr),
    config() { }

  std::unique_ptr<HypergraphType> hypergraph;
  Configuration<HypergraphType> config;
};

class AHyperedgeMovementOperation : public AHyperedgeFMRefiner {
  public:
  AHyperedgeMovementOperation() :
    AHyperedgeFMRefiner() { }

  void SetUp() {
    hypergraph.reset(new HypergraphType(5, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 5 },
                                        HyperedgeVector { 0, 1, 2, 3, 4 }));
    hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
    hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
    hypergraph->changeNodePartition(2, INVALID_PARTITION, 0);
    hypergraph->changeNodePartition(3, INVALID_PARTITION, 0);
    hypergraph->changeNodePartition(4, INVALID_PARTITION, 1);
  }
};

class TheUpdateGainsMethod : public AHyperedgeFMRefiner {
  public:
  TheUpdateGainsMethod() :
    AHyperedgeFMRefiner() { }

  void SetUp() {
    hypergraph.reset(new HypergraphType(9, 7, HyperedgeIndexVector { 0, 3, 7, 9, 11, 13, 15, /*sentinel*/ 17 },
                                        HyperedgeVector { 3, 4, 8, 2, 5, 6, 7, 2, 3, 1, 2, 0, 3, 5, 4, 6, 7 }));
    hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
    hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
    hypergraph->changeNodePartition(2, INVALID_PARTITION, 0);
    hypergraph->changeNodePartition(3, INVALID_PARTITION, 0);
    hypergraph->changeNodePartition(4, INVALID_PARTITION, 1);
    hypergraph->changeNodePartition(5, INVALID_PARTITION, 1);
    hypergraph->changeNodePartition(6, INVALID_PARTITION, 1);
    hypergraph->changeNodePartition(7, INVALID_PARTITION, 1);
    hypergraph->changeNodePartition(8, INVALID_PARTITION, 1);
  }
};

TEST_F(AHyperedgeFMRefiner, DetectsNestedHyperedgesViaBitvectorProbing) {
  hypergraph.reset(new HypergraphType(3, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                      HyperedgeVector { 0, 1, 2, 0, 1 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 0);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.isNestedIntoInPartition(0, 1, 0), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner.isNestedIntoInPartition(1, 0, 0), Eq(true));
}

TEST_F(AHyperedgeFMRefiner, OnlyConsidersPinsInRelevantPartitionWhenDetectingNestedHyperedges) {
  hypergraph.reset(new HypergraphType(4, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                      HyperedgeVector { 0, 2, 3, 1, 2 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);

  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  ASSERT_THAT(hyperedge_fm_refiner.isNestedIntoInPartition(1, 0, 1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner.isNestedIntoInPartition(1, 0, 0), Eq(false));
}

TEST_F(AHyperedgeFMRefiner, ComputesGainOfMovingAllPinsFromOneToAnotherPartition) {
  hypergraph.reset(new HypergraphType(7, 3, HyperedgeIndexVector { 0, 3, 7, /*sentinel*/ 9 },
                                      HyperedgeVector { 0, 5, 6, 1, 2, 3, 4, 4, 5 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(4, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(5, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(6, INVALID_PARTITION, 1);

  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 0, 1), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 0, 1), Eq(0));
}

TEST_F(AHyperedgeFMRefiner, ComputesGainValuesOnModifiedHypergraph) {
  hypergraph.reset(new HypergraphType(7, 3, HyperedgeIndexVector { 0, 3, 7, /*sentinel*/ 9 },
                                      HyperedgeVector { 0, 5, 6, 1, 2, 3, 4, 4, 5 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(4, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(5, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(6, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 0, 1), Eq(0));

  hypergraph->changeNodePartition(5, 1, 0);
  hypergraph->changeNodePartition(6, 1, 0);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(2));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 1, 0), Eq(1));
}

TEST_F(AHyperedgeFMRefiner, ConsidersEachHyperedgeOnlyOnceDuringGainComputation) {
  hypergraph.reset(new HypergraphType(4, 3, HyperedgeIndexVector { 0, 2, 5, /*sentinel*/ 8 },
                                      HyperedgeVector { 0, 1, 1, 2, 3, 0, 2, 3 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(2));
}

TEST_F(AHyperedgeFMRefiner, IncreasesGainOfHyperedgeMovementByOneWhenNestedCutHyperedgesExist) {
  hypergraph.reset(new HypergraphType(6, 3, HyperedgeIndexVector { 0, 6, 9, /*sentinel*/ 11 },
                                      HyperedgeVector { 0, 1, 2, 3, 4, 5, 0, 2, 3, 1, 4 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(4, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(5, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(3));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(3));
}

TEST_F(AHyperedgeFMRefiner, DoesNotChangeGainOfHyperedgeMovementForNonNestedCutHyperedges) {
  hypergraph.reset(new HypergraphType(5, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 6 },
                                      HyperedgeVector { 0, 1, 2, 2, 4, 3 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(4, INVALID_PARTITION, 0);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(1));
}

TEST_F(AHyperedgeFMRefiner, DoesNotChangeGainOfHyperedgeMovementForNestedNonCutHyperedges) {
  hypergraph.reset(new HypergraphType(3, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                      HyperedgeVector { 0, 1, 2, 1, 2 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(1));
}

TEST_F(AHyperedgeFMRefiner, DecreasesGainOfHyperedgeMovementByOneWhenNonNestedNonCutHyperedgesExist) {
  hypergraph.reset(new HypergraphType(4, 3, HyperedgeIndexVector { 0, 2, 4, /*sentinel*/ 6 },
                                      HyperedgeVector { 0, 1, 1, 2, 1, 3 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 0);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(-1));
}

TEST_F(AHyperedgeFMRefiner, MaintainsSizeOfPartitionsWhichAreInitializedByCallingInitialize) {
  HypernodeWeightVector hypernode_weights { 4, 5 };
  hypergraph.reset(new HypergraphType(2, 1, HyperedgeIndexVector { 0, /*sentinel*/ 2 },
                                      HyperedgeVector { 0, 1 }, nullptr, &hypernode_weights));
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 1);

  hyperedge_fm_refiner.initialize();
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[0], Eq(4));
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[1], Eq(5));
}

TEST_F(AHyperedgeFMRefiner, ActivatesOnlyCutHyperedgesByInsertingThemIntoPQ) {
  hypergraph.reset(new HypergraphType(3, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 4 },
                                      HyperedgeVector { 0, 1, 1, 2 }));
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);

  hyperedge_fm_refiner.activateIncidentCutHyperedges(1);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(0), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(0), Eq(false));
}

TEST_F(AHyperedgeFMRefiner, ActivatesCutHyperedgesOnlyOnce) {
  hypergraph.reset(new HypergraphType(3, 1, HyperedgeIndexVector { 0, /*sentinel*/ 3 },
                                      HyperedgeVector { 0, 1, 2 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);

  hyperedge_fm_refiner.activateIncidentCutHyperedges(0);
  hyperedge_fm_refiner.activateIncidentCutHyperedges(1);
}

TEST_F(AHyperedgeMovementOperation, UpdatesPartitionSizes) {
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[0], Eq(4));
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[1], Eq(1));
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);

  hyperedge_fm_refiner.moveHyperedge(1, 0, 1);

  ASSERT_THAT(hyperedge_fm_refiner._partition_size[0], Eq(2));
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[1], Eq(3));
}

TEST_F(AHyperedgeMovementOperation, DeletesTheRemaningPQEntry) {
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(true));

  hyperedge_fm_refiner.moveHyperedge(1, 0, 1);
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(false));
}

TEST_F(AHyperedgeMovementOperation, LocksHyperedgeAfterPinsAreMoved) {
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);

  hyperedge_fm_refiner.moveHyperedge(1, 0, 1);

  ASSERT_THAT(hyperedge_fm_refiner.isLocked(1), Eq(true));
}

TEST_F(TheUpdateGainsMethod, IgnoresLockedHyperedges) {
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.lock(0);

  // assuming HE 2 has just been moved
  hyperedge_fm_refiner.lock(2);
  hyperedge_fm_refiner.updateNeighbours(2);

  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(0), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(0), Eq(false));
}

TEST_F(TheUpdateGainsMethod, EvaluatedEachHyperedgeOnlyOnce) {
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();

  hyperedge_fm_refiner._update_indicator.markAsEvaluated(1);
  hyperedge_fm_refiner.recomputeGainsForIncidentCutHyperedges(2);

  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(false));
}

TEST_F(TheUpdateGainsMethod, RemovesHyperedgesThatAreNoLongerCutHyperedgesFromPQs) {
  // simpler hypergraph since this is the only case that is not covered by the example graph
  hypergraph.reset(new HypergraphType(4, 2, HyperedgeIndexVector { 0, 4, /*sentinel*/ 6 },
                                      HyperedgeVector { 0, 1, 2, 3, 0, 3 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);

  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();

  hyperedge_fm_refiner.activateIncidentCutHyperedges(0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(0), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(0), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(true));
  hyperedge_fm_refiner.moveHyperedge(0, 1, 0);
  hyperedge_fm_refiner._pq[1]->remove(0);

  hyperedge_fm_refiner.updateNeighbours(0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(false));
}

TEST_F(TheUpdateGainsMethod, ActivatesHyperedgesThatBecameCutHyperedges) {
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(3);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(5), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(5), Eq(false));
  hyperedge_fm_refiner.moveHyperedge(0, 1, 0);
  hyperedge_fm_refiner._pq[1]->remove(0);

  hyperedge_fm_refiner.updateNeighbours(0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(5), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(5), Eq(true));
}

TEST_F(TheUpdateGainsMethod, RecomputesGainForHyperedgesThatRemainCutHyperedges) {
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);
  hyperedge_fm_refiner.activateIncidentCutHyperedges(3);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->key(1), Eq(0));
  hyperedge_fm_refiner.moveHyperedge(0, 1, 0);
  hyperedge_fm_refiner._pq[1]->remove(0);

  hyperedge_fm_refiner.updateNeighbours(0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->key(1), Eq(2));
}

TEST_F(AHyperedgeFMRefiner, ChoosesHyperedgeWithHighestGainAsNextMove) {
  hypergraph.reset(new HypergraphType(4, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 4 },
                                      HyperedgeVector { 2, 3, 0, 1 }));
  hypergraph->changeNodePartition(0, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(1, INVALID_PARTITION, 1);
  hypergraph->changeNodePartition(2, INVALID_PARTITION, 0);
  hypergraph->changeNodePartition(3, INVALID_PARTITION, 1);
  hypergraph->setEdgeWeight(0, 1);
  hypergraph->setEdgeWeight(1, 5);
  HyperedgeFMRefiner<HypergraphType> hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(1);
  hyperedge_fm_refiner.activateIncidentCutHyperedges(3);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->key(0), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->key(0), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->key(1), Eq(5));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->key(1), Eq(5));

  HyperedgeWeight max_gain = -1;
  HyperedgeID max_gain_he = -1;
  PartitionID from = -1;
  PartitionID to = -1;

  hyperedge_fm_refiner.chooseNextMove(max_gain, max_gain_he, from, to);

  ASSERT_THAT(max_gain, Eq(5));
  ASSERT_THAT(max_gain_he, Eq(1));
  ASSERT_THAT(from, Eq(1));
  ASSERT_THAT(to, Eq(0));
}
} // namespace partition
