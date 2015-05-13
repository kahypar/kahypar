/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "partition/refinement/MaxGainNodeKWayFMRefiner.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeight;
using defs::HypernodeID;

namespace partition {
using KWayFMRefinerSimpleStopping = MaxGainNodeKWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;

class AMaxGainNodeKWayFMRefiner : public Test {
 public:
  AMaxGainNodeKWayFMRefiner() :
    config(),
    // example hypergraph with one additional HN and one additional HE
    hypergraph(new Hypergraph(8, 5, HyperedgeIndexVector { 0, 2, 6, 9, 12,  /*sentinel*/ 14 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6, 2, 7 }, 4)),
    refiner() {
    config.partition.k = 4;
    hypergraph->setNodePart(0, 0);
    hypergraph->setNodePart(1, 0);
    hypergraph->setNodePart(2, 1);
    hypergraph->setNodePart(3, 2);
    hypergraph->setNodePart(4, 2);
    hypergraph->setNodePart(5, 3);
    hypergraph->setNodePart(6, 3);
    hypergraph->setNodePart(7, 1);
    config.fm_local_search.max_number_of_fruitless_moves = 50;
    config.partition.total_graph_weight = 8;
    refiner = std::make_unique<KWayFMRefinerSimpleStopping>(*hypergraph, config);
    // should be large enough to act as upper bound for both bucket- and heap-based PQ
    refiner->initialize(100);
  }

  Configuration config;
  std::unique_ptr<Hypergraph> hypergraph;
  std::unique_ptr<KWayFMRefinerSimpleStopping> refiner;
};

TEST_F(AMaxGainNodeKWayFMRefiner, IdentifiesBorderHypernodes) {
  ASSERT_THAT(refiner->isBorderNode(0), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(6), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(7), Eq(false));
}

TEST_F(AMaxGainNodeKWayFMRefiner, ActivatesBorderNodes) {
  refiner->activate(1, 42);

  ASSERT_THAT(refiner->_pq.max(), Eq(1));
  ASSERT_THAT(refiner->_pq.maxKey(), Eq(0));
}

TEST_F(AMaxGainNodeKWayFMRefiner, DoesNotActivateInternalNodes) {
  refiner->activate(7, 42);

  ASSERT_THAT(refiner->_pq.contains(7), Eq(false));
}

TEST_F(AMaxGainNodeKWayFMRefiner, ComputesGainOfHypernodeMoves) {
  // hypergraph with positive, zero and negative gain nodes
  hypergraph.reset(new Hypergraph(9, 5, HyperedgeIndexVector { 0, 2, 4, 8, 10,  /*sentinel*/ 13 },
                                  HyperedgeVector { 0, 1, 1, 2, 2, 3, 4, 5, 5, 6, 6, 7, 8 }, 4));
  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);
  hypergraph->setNodePart(2, 0);
  hypergraph->setNodePart(3, 3);
  hypergraph->setNodePart(4, 3);
  hypergraph->setNodePart(5, 2);
  hypergraph->setNodePart(6, 2);
  hypergraph->setNodePart(7, 2);
  hypergraph->setNodePart(8, 1);

  // positive gain
  ASSERT_THAT(refiner->computeMaxGainMove(1).first, Eq(2));
  ASSERT_THAT(refiner->computeMaxGainMove(1).second, Eq(0));

  // zero gain
  ASSERT_THAT(refiner->computeMaxGainMove(4).first, Eq(0));
  ASSERT_THAT(refiner->computeMaxGainMove(4).second, Eq(0));

  // negative gain
  ASSERT_THAT(refiner->computeMaxGainMove(6).first, Eq(-1));
  ASSERT_THAT(refiner->computeMaxGainMove(6).second, Eq(1));
}

TEST_F(AMaxGainNodeKWayFMRefiner, PerformsMovesThatDontLeadToImbalancedPartitions) {
  hypergraph.reset(new Hypergraph(8, 4, HyperedgeIndexVector { 0, 2, 4, 7,  /*sentinel*/ 9 },
                                  HyperedgeVector { 0, 1, 2, 3, 4, 5, 7, 5, 6 }, 4));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 1);
  hypergraph->setNodePart(3, 1);
  hypergraph->setNodePart(4, 2);
  hypergraph->setNodePart(5, 2);
  hypergraph->setNodePart(6, 3);
  hypergraph->setNodePart(7, 3);
  config.partition.k = 4;
  config.partition.epsilon = 1.0;
  config.partition.max_part_weight =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));

  refiner->moveHypernode(7, 3, 2);
  ASSERT_THAT(hypergraph->partID(7), Eq(2));
}

TEST_F(AMaxGainNodeKWayFMRefiner, PerformsCompleteRollbackIfNoImprovementCouldBeFound) {
  hypergraph.reset(new Hypergraph(8, 6, HyperedgeIndexVector { 0, 2, 5, 7, 9, 11,  /*sentinel*/ 13 },
                                  HyperedgeVector { 0, 1, 0, 1, 6, 1, 6, 2, 3, 4, 5, 6, 7 }, 4));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 1);
  hypergraph->setNodePart(3, 1);
  hypergraph->setNodePart(4, 2);
  hypergraph->setNodePart(5, 2);
  hypergraph->setNodePart(6, 3);
  hypergraph->setNodePart(7, 3);

  Hypergraph orig_hgr(8, 6, HyperedgeIndexVector { 0, 2, 5, 7, 9, 11,  /*sentinel*/ 13 },
                      HyperedgeVector { 0, 1, 0, 1, 6, 1, 6, 2, 3, 4, 5, 6, 7 }, 4);
  orig_hgr.setNodePart(0, 0);
  orig_hgr.setNodePart(1, 0);
  orig_hgr.setNodePart(2, 1);
  orig_hgr.setNodePart(3, 1);
  orig_hgr.setNodePart(4, 2);
  orig_hgr.setNodePart(5, 2);
  orig_hgr.setNodePart(6, 3);
  orig_hgr.setNodePart(7, 3);

  config.partition.k = 4;
  config.partition.epsilon = 1.0;
  config.partition.max_part_weight =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));
  // should be large enough to act as upper bound for both bucket- and heap-based PQ
  refiner->initialize(100);

  double old_imbalance = metrics::imbalance(*hypergraph);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(*hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 0, 1 };

  refiner->refine(refinement_nodes, 2, 0, old_cut, old_imbalance);

  ASSERT_THAT(verifyEquivalenceWithPartitionInfo(orig_hgr, *hypergraph), Eq(true));
}

TEST_F(AMaxGainNodeKWayFMRefiner, ComputesCorrectGainValues) {
  hypergraph.reset(new Hypergraph(10, 5, HyperedgeIndexVector { 0, 4, 7, 9, 12,  /*sentinel*/ 15 },
                                  HyperedgeVector { 1, 2, 5, 6, 2, 3, 4, 0, 2, 2, 8, 9, 0, 2, 8 }, 4));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 1);
  hypergraph->setNodePart(3, 2);
  hypergraph->setNodePart(4, 2);
  hypergraph->setNodePart(5, 2);
  hypergraph->setNodePart(6, 3);
  hypergraph->setNodePart(7, 3);
  hypergraph->setNodePart(8, 1);
  hypergraph->setNodePart(9, 1);

  config.partition.k = 4;
  config.partition.epsilon = 1.0;
  config.partition.max_part_weight =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));

  ASSERT_THAT(refiner->computeMaxGainMove(2).first, Eq(0));
  ASSERT_THAT(refiner->computeMaxGainMove(2).second, Eq(0));
}


TEST_F(AMaxGainNodeKWayFMRefiner, ComputesCorrectConnectivityDecreaseValues) {
  // hypergraph with positive, zero and negative gain nodes
  hypergraph.reset(new Hypergraph(8, 6, HyperedgeIndexVector { 0, 2, 7, 11,  /*sentinel*/ 13, 16, 20 },
                                  HyperedgeVector { 0, 2, 0, 1, 3, 4, 5, 0, 3, 4, 5, 0, 1, 0, 3, 5, 0, 2, 6, 7 }, 4));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 1);
  hypergraph->setNodePart(3, 1);
  hypergraph->setNodePart(4, 2);
  hypergraph->setNodePart(5, 3);
  hypergraph->setNodePart(6, 3);
  hypergraph->setNodePart(7, 0);
  hypergraph->printGraphState();
  config.partition.k = 4;
  config.partition.epsilon = 1.0;
  config.partition.max_part_weight =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));


  ASSERT_THAT(refiner->computeMaxGainMove(0).first, Eq(0));
}

TEST_F(AMaxGainNodeKWayFMRefiner, ChoosesMaxGainMoveHNWithHighesConnectivityDecrease) {
  // hypergraph with zero gain moves and different connectivity decrease values
  // Move of HN 3 to part 1 has decrease -1 and move to part 2 has decrease +1
  hypergraph.reset(new Hypergraph(10, 4, HyperedgeIndexVector { 0, 4, 7, 10,  /*sentinel*/ 13 },
                                  HyperedgeVector { 0, 1, 2, 3, 3, 4, 5, 3, 8, 9, 3, 6, 7 }, 3));

  config.partition.k = 3;
  config.partition.epsilon = 1.0;
  config.partition.max_part_weight =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  hypergraph->setNodePart(0, 1);
  hypergraph->setNodePart(1, 2);
  hypergraph->setNodePart(2, 0);
  hypergraph->setNodePart(3, 0);
  hypergraph->setNodePart(4, 1);
  hypergraph->setNodePart(5, 2);
  hypergraph->setNodePart(6, 2);
  hypergraph->setNodePart(7, 0);
  hypergraph->setNodePart(8, 2);
  hypergraph->setNodePart(9, 0);

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));

  ASSERT_THAT(refiner->computeMaxGainMove(3).first, Eq(0));
  ASSERT_THAT(refiner->computeMaxGainMove(3).second, Eq(2));
}

TEST_F(AMaxGainNodeKWayFMRefiner, ConsidersSingleNodeHEsDuringGainComputation) {
  hypergraph.reset(new Hypergraph(2, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 3 },
                                  HyperedgeVector { 0, 1, 0 }, 2));

  config.partition.k = 2;
  config.partition.epsilon = 1.0;
  config.partition.max_part_weight =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));

  ASSERT_THAT(refiner->computeMaxGainMove(0).first, Eq(1));
  ASSERT_THAT(refiner->computeMaxGainMove(0).second, Eq(1));
}
}  // namespace partition
