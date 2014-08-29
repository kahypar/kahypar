/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeight;
using defs::HypernodeID;

namespace partition {
typedef KWayFMRefiner<NumberOfFruitlessMovesStopsSearch> KWayFMRefinerSimpleStopping;

class AKWayFMRefiner : public Test {
  public:
  AKWayFMRefiner() :
    config(),
    // example hypergraph with one additional HN and one additional HE
    hypergraph(new Hypergraph(8, 5, HyperedgeIndexVector { 0, 2, 6, 9, 12, /*sentinel*/ 14 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6, 2, 7 }, 4)),
    refiner(new KWayFMRefinerSimpleStopping(*hypergraph, config)) {
    config.partition.k = 4;
    hypergraph->setNodePart(0, 0);
    hypergraph->setNodePart(1, 0);
    hypergraph->setNodePart(2, 1);
    hypergraph->setNodePart(3, 2);
    hypergraph->setNodePart(4, 2);
    hypergraph->setNodePart(5, 3);
    hypergraph->setNodePart(6, 3);
    hypergraph->setNodePart(7, 1);
    config.two_way_fm.max_number_of_fruitless_moves = 50;
    refiner->initialize();
  }

  Configuration config;
  std::unique_ptr<Hypergraph> hypergraph;
  std::unique_ptr<KWayFMRefinerSimpleStopping> refiner;
};

TEST_F(AKWayFMRefiner, IdentifiesBorderHypernodes) {
  ASSERT_THAT(refiner->isBorderNode(0), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(6), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(7), Eq(false));
}

TEST_F(AKWayFMRefiner, ActivatesBorderNodes) {
  refiner->activate(1);

  ASSERT_THAT(refiner->_pq.max(), Eq(1));
  ASSERT_THAT(refiner->_pq.maxKey(), Eq(0));
}

TEST_F(AKWayFMRefiner, DoesNotActivateInternalNodes) {
  refiner->activate(7);

  ASSERT_THAT(refiner->_pq.contains(7), Eq(false));
}

TEST_F(AKWayFMRefiner, ComputesGainOfHypernodeMoves) {
  // hypergraph with positive, zero and negative gain nodes
  hypergraph.reset(new Hypergraph(9, 5, HyperedgeIndexVector { 0, 2, 4, 8, 10, /*sentinel*/ 13 },
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
  ASSERT_THAT(refiner->computeMaxGain(1).first, Eq(2));
  ASSERT_THAT(refiner->computeMaxGain(1).second, Eq(0));

  // zero gain
  ASSERT_THAT(refiner->computeMaxGain(4).first, Eq(0));
  ASSERT_THAT(refiner->computeMaxGain(4).second, Eq(0));

  // negative gain
  ASSERT_THAT(refiner->computeMaxGain(6).first, Eq(-1));
  ASSERT_THAT(refiner->computeMaxGain(6).second, Eq(1));
}

TEST_F(AKWayFMRefiner, DoesNotPerformMovesThatWouldLeadToImbalancedPartitions) {
  hypergraph.reset(new Hypergraph(8, 4, HyperedgeIndexVector { 0, 2, 4, 7, /*sentinel*/ 9 },
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
  config.partition.epsilon = 0.02;
  config.partition.max_part_size = (1 + config.partition.epsilon)
                                   * ceil(hypergraph->numNodes() /
                                          static_cast<double>(config.partition.k));

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));

  ASSERT_THAT(refiner->moveHypernode(7, 3, 2), Eq(false));
}

TEST_F(AKWayFMRefiner, PerformsMovesThatDontLeadToImbalancedPartitions) {
  hypergraph.reset(new Hypergraph(8, 4, HyperedgeIndexVector { 0, 2, 4, 7, /*sentinel*/ 9 },
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
  config.partition.max_part_size =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));

  ASSERT_THAT(refiner->moveHypernode(7, 3, 2), Eq(true));
}

TEST_F(AKWayFMRefiner, PerformsCompleteRollbackIfNoImprovementCouldBeFound) {
  hypergraph.reset(new Hypergraph(8, 6, HyperedgeIndexVector { 0, 2, 5, 7, 9, 11, /*sentinel*/ 13 },
                                  HyperedgeVector { 0, 1, 0, 1, 6, 1, 6, 2, 3, 4, 5, 6, 7 }, 4));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 1);
  hypergraph->setNodePart(3, 1);
  hypergraph->setNodePart(4, 2);
  hypergraph->setNodePart(5, 2);
  hypergraph->setNodePart(6, 3);
  hypergraph->setNodePart(7, 3);

  Hypergraph orig_hgr(8, 6, HyperedgeIndexVector { 0, 2, 5, 7, 9, 11, /*sentinel*/ 13 },
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
  config.partition.max_part_size =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));

  double old_imbalance = metrics::imbalance(*hypergraph);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(*hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 0, 1 };

  refiner->refine(refinement_nodes, 2, old_cut, old_imbalance);

  ASSERT_THAT(verifyEquivalence(orig_hgr, *hypergraph), Eq(true));
}

TEST_F(AKWayFMRefiner, ComputesCorrectGainValues) {
  hypergraph.reset(new Hypergraph(10, 5, HyperedgeIndexVector { 0, 4, 7, 9, 12, /*sentinel*/ 15 },
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
  config.partition.max_part_size =
    (1 + config.partition.epsilon)
    * ceil(hypergraph->numNodes() / static_cast<double>(config.partition.k));

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, config));

  ASSERT_THAT(refiner->computeMaxGain(2).first, Eq(0));
  ASSERT_THAT(refiner->computeMaxGain(2).second, Eq(0));
}
} // namespace partition
