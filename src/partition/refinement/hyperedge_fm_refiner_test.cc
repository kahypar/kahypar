/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/refinement/HyperedgeFMRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMQueueSelectionPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;

using defs::HypernodeWeightVector;
using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeight;
using defs::HyperedgeID;

namespace partition {
typedef HyperedgeFMRefiner<NumberOfFruitlessMovesStopsSearch,
                           EligibleTopGain,
                           OnlyRemoveIfBothQueuesClogged> HyperedgeFMRefinerSimpleStopping;

class AHyperedgeFMRefiner : public Test {
  public:
  AHyperedgeFMRefiner() :
    hypergraph(nullptr),
    config() { }

  std::unique_ptr<Hypergraph> hypergraph;
  Configuration config;
};

class AHyperedgeMovementOperation : public AHyperedgeFMRefiner {
  public:
  AHyperedgeMovementOperation() :
    AHyperedgeFMRefiner() { }

  void SetUp() {
    hypergraph.reset(new Hypergraph(5, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 5 },
                                    HyperedgeVector { 0, 1, 2, 3, 4 }));
    hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 1);
  }
};

class TheUpdateGainsMethod : public AHyperedgeFMRefiner {
  public:
  TheUpdateGainsMethod() :
    AHyperedgeFMRefiner() { }

  void SetUp() {
    hypergraph.reset(new Hypergraph(9, 7, HyperedgeIndexVector { 0, 3, 7, 9, 11, 13, 15, /*sentinel*/ 17 },
                                    HyperedgeVector { 3, 4, 8, 2, 5, 6, 7, 2, 3, 1, 2, 0, 3, 5, 4, 6, 7 }));
    hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 1);
    hypergraph->changeNodePartition(5, hypergraph->invalidPartitionID(), 1);
    hypergraph->changeNodePartition(6, hypergraph->invalidPartitionID(), 1);
    hypergraph->changeNodePartition(7, hypergraph->invalidPartitionID(), 1);
    hypergraph->changeNodePartition(8, hypergraph->invalidPartitionID(), 1);
  }
};

class RollBackInformation : public AHyperedgeFMRefiner {
  public:
  RollBackInformation() :
    AHyperedgeFMRefiner(),
    hyperedge_fm_refiner(nullptr) { }

  void SetUp() {
    hypergraph.reset(new Hypergraph(9, 3, HyperedgeIndexVector { 0, 4, 7, /*sentinel*/ 9 },
                                    HyperedgeVector { 0, 1, 2, 3, 4, 5, 6, 7, 8 }));
    hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
    hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(5, hypergraph->invalidPartitionID(), 1);
    hypergraph->changeNodePartition(6, hypergraph->invalidPartitionID(), 1);
    hypergraph->changeNodePartition(7, hypergraph->invalidPartitionID(), 0);
    hypergraph->changeNodePartition(8, hypergraph->invalidPartitionID(), 1);
    config.partitioning.epsilon = 1;
    hyperedge_fm_refiner.reset(new HyperedgeFMRefinerSimpleStopping(*hypergraph, config));
    hyperedge_fm_refiner->initialize();
    hyperedge_fm_refiner->activateIncidentCutHyperedges(0);
    hyperedge_fm_refiner->activateIncidentCutHyperedges(5);
    hyperedge_fm_refiner->activateIncidentCutHyperedges(8);
  }

  std::unique_ptr<HyperedgeFMRefinerSimpleStopping> hyperedge_fm_refiner;
};

TEST_F(AHyperedgeFMRefiner, DetectsNestedHyperedgesViaBitvectorProbing) {
  hypergraph.reset(new Hypergraph(3, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                  HyperedgeVector { 0, 1, 2, 0, 1 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);

  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  ASSERT_THAT(hyperedge_fm_refiner.isNestedIntoInPartition(0, 1, 0), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner.isNestedIntoInPartition(1, 0, 0), Eq(true));
}

TEST_F(AHyperedgeFMRefiner, OnlyConsidersPinsInRelevantPartitionWhenDetectingNestedHyperedges) {
  hypergraph.reset(new Hypergraph(4, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                  HyperedgeVector { 0, 2, 3, 1, 2 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);

  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  ASSERT_THAT(hyperedge_fm_refiner.isNestedIntoInPartition(1, 0, 1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner.isNestedIntoInPartition(1, 0, 0), Eq(false));
}

TEST_F(AHyperedgeFMRefiner, ComputesGainOfMovingAllPinsFromOneToAnotherPartition) {
  hypergraph.reset(new Hypergraph(7, 3, HyperedgeIndexVector { 0, 3, 7, /*sentinel*/ 9 },
                                  HyperedgeVector { 0, 5, 6, 1, 2, 3, 4, 4, 5 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(5, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(6, hypergraph->invalidPartitionID(), 1);

  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 0, 1), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 1, 0), Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(2, 0, 1), Eq(0));
}

TEST_F(AHyperedgeFMRefiner, ComputesGainValuesOnModifiedHypergraph) {
  hypergraph.reset(new Hypergraph(7, 3, HyperedgeIndexVector { 0, 3, 7, /*sentinel*/ 9 },
                                  HyperedgeVector { 0, 5, 6, 1, 2, 3, 4, 4, 5 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(5, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(6, hypergraph->invalidPartitionID(), 1);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
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
  hypergraph.reset(new Hypergraph(4, 3, HyperedgeIndexVector { 0, 2, 5, /*sentinel*/ 8 },
                                  HyperedgeVector { 0, 1, 1, 2, 3, 0, 2, 3 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(1, 1, 0), Eq(2));
}

TEST_F(AHyperedgeFMRefiner, IncreasesGainOfHyperedgeMovementByOneWhenNestedCutHyperedgesExist) {
  hypergraph.reset(new Hypergraph(6, 3, HyperedgeIndexVector { 0, 6, 9, /*sentinel*/ 11 },
                                  HyperedgeVector { 0, 1, 2, 3, 4, 5, 0, 2, 3, 1, 4 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(5, hypergraph->invalidPartitionID(), 1);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(3));
  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(3));
}

TEST_F(AHyperedgeFMRefiner, DoesNotChangeGainOfHyperedgeMovementForNonNestedCutHyperedges) {
  hypergraph.reset(new Hypergraph(5, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 6 },
                                  HyperedgeVector { 0, 1, 2, 2, 4, 3 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 0);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(1));
}

TEST_F(AHyperedgeFMRefiner, DoesNotChangeGainOfHyperedgeMovementForNestedNonCutHyperedges) {
  hypergraph.reset(new Hypergraph(3, 2, HyperedgeIndexVector { 0, 3, /*sentinel*/ 5 },
                                  HyperedgeVector { 0, 1, 2, 1, 2 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 1, 0), Eq(1));
}

TEST_F(AHyperedgeFMRefiner, DecreasesGainOfHyperedgeMovementByOneWhenNonNestedNonCutHyperedgesExist) {
  hypergraph.reset(new Hypergraph(4, 3, HyperedgeIndexVector { 0, 2, 4, /*sentinel*/ 6 },
                                  HyperedgeVector { 0, 1, 1, 2, 1, 3 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 0);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);

  ASSERT_THAT(hyperedge_fm_refiner.computeGain(0, 0, 1), Eq(-1));
}

TEST_F(AHyperedgeFMRefiner, MaintainsSizeOfPartitionsWhichAreInitializedByCallingInitialize) {
  HypernodeWeightVector hypernode_weights { 4, 5 };
  hypergraph.reset(new Hypergraph(2, 1, HyperedgeIndexVector { 0, /*sentinel*/ 2 },
                                  HyperedgeVector { 0, 1 }, 2, nullptr, &hypernode_weights));
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 1);

  hyperedge_fm_refiner.initialize();
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[0], Eq(4));
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[1], Eq(5));
}

TEST_F(AHyperedgeFMRefiner, ActivatesOnlyCutHyperedgesByInsertingThemIntoPQ) {
  hypergraph.reset(new Hypergraph(3, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 4 },
                                  HyperedgeVector { 0, 1, 1, 2 }));
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);

  hyperedge_fm_refiner.activateIncidentCutHyperedges(1);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(0), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(0), Eq(false));
}

TEST_F(AHyperedgeFMRefiner, ActivatesCutHyperedgesOnlyOnce) {
  hypergraph.reset(new Hypergraph(3, 1, HyperedgeIndexVector { 0, /*sentinel*/ 3 },
                                  HyperedgeVector { 0, 1, 2 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);

  hyperedge_fm_refiner.activateIncidentCutHyperedges(0);
  hyperedge_fm_refiner.activateIncidentCutHyperedges(1);
}

TEST_F(AHyperedgeFMRefiner, ChoosesHyperedgeWithHighestGainAsNextMove) {
  hypergraph.reset(new Hypergraph(4, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 4 },
                                  HyperedgeVector { 2, 3, 0, 1 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->setEdgeWeight(0, 1);
  hypergraph->setEdgeWeight(1, 5);
  config.partitioning.partition_size_upper_bound = (1 + config.partitioning.epsilon)
                                                   * ceil(hypergraph->initialNumNodes() /
                                                          static_cast<double>(config.partitioning.k));

  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(1);
  hyperedge_fm_refiner.activateIncidentCutHyperedges(3);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->key(0), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->key(0), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->key(1), Eq(5));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->key(1), Eq(5));

  bool pq0_eligible = false;
  bool pq1_eligible = false;
  hyperedge_fm_refiner.checkPQsForEligibleMoves(pq0_eligible, pq1_eligible);
  bool chosen_pq_index = hyperedge_fm_refiner.selectQueue(pq0_eligible, pq1_eligible);

  ASSERT_THAT(hyperedge_fm_refiner._pq[chosen_pq_index]->maxKey(), Eq(5));
  ASSERT_THAT(hyperedge_fm_refiner._pq[chosen_pq_index]->max(), Eq(1));
  ASSERT_THAT(chosen_pq_index, Eq(1));
}

TEST_F(AHyperedgeFMRefiner, ChecksIfHyperedgeMovePreservesBalanceConstraint) {
  hypergraph.reset(new Hypergraph(6, 2, HyperedgeIndexVector { 0, 4, /*sentinel*/ 6 },
                                  HyperedgeVector { 0, 1, 2, 3, 4, 5 }));
  config.partitioning.epsilon = 0.02;
  config.partitioning.partition_size_upper_bound = (1 + config.partitioning.epsilon)
                                                   * ceil(6 / static_cast<double>(config.partitioning.k));
  DBG(true, "config.partitioning.partition_size_upper_bound=" << config.partitioning.partition_size_upper_bound);
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(5, hypergraph->invalidPartitionID(), 1);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();

  ASSERT_THAT(hyperedge_fm_refiner.movePreservesBalanceConstraint(0, 0, 1), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner.movePreservesBalanceConstraint(1, 0, 1), Eq(true));
}

TEST_F(AHyperedgeFMRefiner, RemovesHyperedgeMovesFromPQsIfBothPQsAreNotEligible) {
  hypergraph.reset(new Hypergraph(8, 2, HyperedgeIndexVector { 0, 7, /*sentinel*/ 9 },
                                  HyperedgeVector { 0, 1, 2, 3, 4, 5, 7, 0, 6 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(5, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(6, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(7, hypergraph->invalidPartitionID(), 0);
  config.partitioning.epsilon = 0.02;
  config.partitioning.partition_size_upper_bound = (1 + config.partitioning.epsilon)
                                                   * ceil(hypergraph->initialNumNodes() /
                                                          static_cast<double>(config.partitioning.k));
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();

  // bypass activation because current version would not insert HEs in the first place
  hyperedge_fm_refiner._pq[0]->reInsert(0, hyperedge_fm_refiner.computeGain(0, 0, 1));
  hyperedge_fm_refiner._pq[1]->reInsert(0, hyperedge_fm_refiner.computeGain(0, 1, 0));
  hyperedge_fm_refiner._pq[0]->reInsert(1, hyperedge_fm_refiner.computeGain(1, 0, 1));
  hyperedge_fm_refiner._pq[1]->reInsert(1, hyperedge_fm_refiner.computeGain(1, 1, 0));
  hypergraph->setEdgeWeight(0, 10);

  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(0), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(0), Eq(true));

  bool pq0_eligible = false;
  bool pq1_eligible = false;
  hyperedge_fm_refiner.checkPQsForEligibleMoves(pq0_eligible, pq1_eligible);
  OnlyRemoveIfBothQueuesClogged::removeCloggingQueueEntries(pq0_eligible, pq1_eligible,
                                                            hyperedge_fm_refiner._pq[0],
                                                            hyperedge_fm_refiner._pq[1]);

  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(0), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(0), Eq(false));
}

TEST_F(TheUpdateGainsMethod, IgnoresLockedHyperedges) {
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.markAsMoved(0);

  // assuming HE 2 has just been moved
  hyperedge_fm_refiner.markAsMoved(2);
  hyperedge_fm_refiner.updateNeighbours(2);

  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(0), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(0), Eq(false));
}

TEST_F(TheUpdateGainsMethod, EvaluatedEachHyperedgeOnlyOnce) {
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();

  hyperedge_fm_refiner._update_indicator.markAsEvaluated(1);
  hyperedge_fm_refiner.recomputeGainsForIncidentCutHyperedges(2);

  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(false));
}

TEST_F(TheUpdateGainsMethod, RemovesHyperedgesThatAreNoLongerCutHyperedgesFromPQs) {
  // simpler hypergraph since this is the only case that is not covered by the example graph
  hypergraph.reset(new Hypergraph(4, 2, HyperedgeIndexVector { 0, 4, /*sentinel*/ 6 },
                                  HyperedgeVector { 0, 1, 2, 3, 0, 3 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);

  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();

  hyperedge_fm_refiner.activateIncidentCutHyperedges(0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(0), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(0), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(true));
  hyperedge_fm_refiner.moveHyperedge(0, 1, 0, 0);
  hyperedge_fm_refiner._pq[1]->remove(0);

  hyperedge_fm_refiner.updateNeighbours(0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(false));
}

TEST_F(TheUpdateGainsMethod, ActivatesHyperedgesThatBecameCutHyperedges) {
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(3);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(5), Eq(false));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(5), Eq(false));
  hyperedge_fm_refiner.moveHyperedge(0, 1, 0, 0);
  hyperedge_fm_refiner._pq[1]->remove(0);

  hyperedge_fm_refiner.updateNeighbours(0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(5), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(5), Eq(true));
}

TEST_F(TheUpdateGainsMethod, RecomputesGainForHyperedgesThatRemainCutHyperedges) {
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);
  hyperedge_fm_refiner.activateIncidentCutHyperedges(3);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->key(1), Eq(0));
  hyperedge_fm_refiner.moveHyperedge(0, 1, 0, 0);
  hyperedge_fm_refiner._pq[1]->remove(0);

  hyperedge_fm_refiner.updateNeighbours(0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->key(1), Eq(-1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->key(1), Eq(2));
}

TEST_F(AHyperedgeMovementOperation, UpdatesPartitionSizes) {
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[0], Eq(4));
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[1], Eq(1));
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);

  hyperedge_fm_refiner.moveHyperedge(1, 0, 1, 0);

  ASSERT_THAT(hyperedge_fm_refiner._partition_size[0], Eq(2));
  ASSERT_THAT(hyperedge_fm_refiner._partition_size[1], Eq(3));
}

TEST_F(AHyperedgeMovementOperation, DeletesTheRemaningPQEntry) {
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);
  ASSERT_THAT(hyperedge_fm_refiner._pq[0]->contains(1), Eq(true));
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(true));

  hyperedge_fm_refiner.moveHyperedge(1, 0, 1, 0);
  ASSERT_THAT(hyperedge_fm_refiner._pq[1]->contains(1), Eq(false));
}

TEST_F(AHyperedgeMovementOperation, LocksHyperedgeAfterPinsAreMoved) {
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);

  hyperedge_fm_refiner.moveHyperedge(1, 0, 1, 0);

  ASSERT_THAT(hyperedge_fm_refiner.isMarkedAsMoved(1), Eq(true));
}


TEST_F(AHyperedgeMovementOperation, ChoosesTheMaxGainMoveIfBothPQsAreEligible) {
  hypergraph.reset(new Hypergraph(4, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 4 },
                                  HyperedgeVector { 0, 1, 2, 3 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->setEdgeWeight(1, 5);
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();
  hyperedge_fm_refiner.activateIncidentCutHyperedges(0);
  hyperedge_fm_refiner.activateIncidentCutHyperedges(2);
  bool pq0_eligible = false;
  bool pq1_eligible = false;
  hyperedge_fm_refiner.checkPQsForEligibleMoves(pq0_eligible, pq1_eligible);
  ASSERT_THAT(pq0_eligible, Eq(true));
  ASSERT_THAT(pq1_eligible, Eq(true));

  bool chosen_pq_index = hyperedge_fm_refiner.selectQueue(pq0_eligible, pq1_eligible);

  ASSERT_THAT(hyperedge_fm_refiner._pq[chosen_pq_index]->maxKey(), Eq(5));
  ASSERT_THAT(hyperedge_fm_refiner._pq[chosen_pq_index]->max(), Eq(1));
  ASSERT_THAT(chosen_pq_index, Eq(0));
}

TEST_F(AHyperedgeMovementOperation, ChoosesTheMaxGainMoveFromEligiblePQ) {
  hypergraph.reset(new Hypergraph(12, 5, HyperedgeIndexVector { 0, 3, 6, 8, 11, /*sentinel*/ 20 },
                                  HyperedgeVector { 0, 7, 8, 2, 3, 4, 5, 6, 9, 10, 11, 0, 1, 2, 4, 7, 8, 5, 6, 9 }));
  hypergraph->changeNodePartition(0, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(1, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(2, hypergraph->invalidPartitionID(), 0);
  hypergraph->changeNodePartition(3, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(4, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(5, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(6, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(7, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(8, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(9, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(10, hypergraph->invalidPartitionID(), 1);
  hypergraph->changeNodePartition(11, hypergraph->invalidPartitionID(), 1);
  config.partitioning.epsilon = 0.02;
  config.partitioning.partition_size_upper_bound = (1 + config.partitioning.epsilon)
                                                   * ceil(12 / static_cast<double>(config.partitioning.k));
  HyperedgeFMRefinerSimpleStopping hyperedge_fm_refiner(*hypergraph, config);
  hyperedge_fm_refiner.initialize();

  hyperedge_fm_refiner.activateIncidentCutHyperedges(0);
  hyperedge_fm_refiner.activateIncidentCutHyperedges(1);

  bool pq0_eligible = false;
  bool pq1_eligible = false;
  hyperedge_fm_refiner.checkPQsForEligibleMoves(pq0_eligible, pq1_eligible);
  if (!pq0_eligible && !pq1_eligible) {
    OnlyRemoveIfBothQueuesClogged::removeCloggingQueueEntries(pq0_eligible, pq1_eligible,
                                                              hyperedge_fm_refiner._pq[0],
                                                              hyperedge_fm_refiner._pq[1]);
  }

  hyperedge_fm_refiner.checkPQsForEligibleMoves(pq0_eligible, pq1_eligible);
  bool chosen_pq_index = hyperedge_fm_refiner.selectQueue(pq0_eligible, pq1_eligible);
  ASSERT_THAT(hyperedge_fm_refiner._pq[chosen_pq_index]->maxKey(), Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner._pq[chosen_pq_index]->max(), Eq(0));
  ASSERT_THAT(chosen_pq_index, Eq(1));
}

TEST_F(RollBackInformation, StoresIDsOfMovedPins) {
  hyperedge_fm_refiner->moveHyperedge(0, 0, 1, 0);
  hyperedge_fm_refiner->moveHyperedge(1, 1, 0, 1);
  hyperedge_fm_refiner->moveHyperedge(2, 1, 0, 2);

  ASSERT_THAT(hyperedge_fm_refiner->_performed_moves[0], Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner->_performed_moves[1], Eq(1));
  ASSERT_THAT(hyperedge_fm_refiner->_performed_moves[2], Eq(2));
  ASSERT_THAT(hyperedge_fm_refiner->_performed_moves[3], Eq(5));
  ASSERT_THAT(hyperedge_fm_refiner->_performed_moves[4], Eq(6));
  ASSERT_THAT(hyperedge_fm_refiner->_performed_moves[5], Eq(8));
  ASSERT_THAT(hyperedge_fm_refiner->_movement_indices[0], Eq(0));
  ASSERT_THAT(hyperedge_fm_refiner->_movement_indices[1], Eq(3));
  ASSERT_THAT(hyperedge_fm_refiner->_movement_indices[2], Eq(5));
  ASSERT_THAT(hyperedge_fm_refiner->_movement_indices[3], Eq(6));
}

TEST_F(RollBackInformation, IsUsedToRollBackMovementsToGivenIndex) {
  hyperedge_fm_refiner->initialize();
  hyperedge_fm_refiner->activateIncidentCutHyperedges(0);
  hyperedge_fm_refiner->activateIncidentCutHyperedges(5);
  hyperedge_fm_refiner->activateIncidentCutHyperedges(8);
  hyperedge_fm_refiner->moveHyperedge(0, 0, 1, 0);
  hyperedge_fm_refiner->moveHyperedge(1, 1, 0, 1);
  hyperedge_fm_refiner->moveHyperedge(2, 1, 0, 2);

  hyperedge_fm_refiner->rollback(2, 0, *hypergraph);

  ASSERT_THAT(hyperedge_fm_refiner->_partition_size[0], Eq(2));
  ASSERT_THAT(hyperedge_fm_refiner->_partition_size[1], Eq(7));
  ASSERT_THAT(hypergraph->partitionIndex(0), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(1), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(2), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(3), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(4), Eq(0));
  ASSERT_THAT(hypergraph->partitionIndex(5), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(6), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(7), Eq(0));
  ASSERT_THAT(hypergraph->partitionIndex(8), Eq(1));
}

TEST_F(RollBackInformation, IsUsedToRollBackMovementsToInitialStateIfNoImprovementWasFound) {
  hyperedge_fm_refiner->initialize();
  hyperedge_fm_refiner->activateIncidentCutHyperedges(0);
  hyperedge_fm_refiner->activateIncidentCutHyperedges(5);
  hyperedge_fm_refiner->activateIncidentCutHyperedges(8);
  hyperedge_fm_refiner->moveHyperedge(0, 0, 1, 0);
  hyperedge_fm_refiner->moveHyperedge(1, 1, 0, 1);
  hyperedge_fm_refiner->moveHyperedge(2, 1, 0, 2);

  hyperedge_fm_refiner->rollback(2, -1, *hypergraph);

  ASSERT_THAT(hyperedge_fm_refiner->_partition_size[0], Eq(5));
  ASSERT_THAT(hyperedge_fm_refiner->_partition_size[1], Eq(4));
  ASSERT_THAT(hypergraph->partitionIndex(0), Eq(0));
  ASSERT_THAT(hypergraph->partitionIndex(1), Eq(0));
  ASSERT_THAT(hypergraph->partitionIndex(2), Eq(0));
  ASSERT_THAT(hypergraph->partitionIndex(3), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(4), Eq(0));
  ASSERT_THAT(hypergraph->partitionIndex(5), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(6), Eq(1));
  ASSERT_THAT(hypergraph->partitionIndex(7), Eq(0));
  ASSERT_THAT(hypergraph->partitionIndex(8), Eq(1));
}
}                // namespace partition
