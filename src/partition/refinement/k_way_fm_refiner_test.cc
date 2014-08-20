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
    // example hypergraph with one additional HN and one additional HE
    hypergraph(8, 5, HyperedgeIndexVector { 0, 2, 6, 9, 12, /*sentinel*/ 14 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6, 2, 7 }, 4),
    config(),
    refiner(nullptr) {
    config.partitioning.k = 4;
    hypergraph.setNodePart(0, 0);
    hypergraph.setNodePart(1, 0);
    hypergraph.setNodePart(2, 1);
    hypergraph.setNodePart(3, 2);
    hypergraph.setNodePart(4, 2);
    hypergraph.setNodePart(5, 3);
    hypergraph.setNodePart(6, 3);
    hypergraph.setNodePart(7, 1);
    config.two_way_fm.max_number_of_fruitless_moves = 50;
    refiner = new KWayFMRefinerSimpleStopping(hypergraph, config);
    refiner->initialize();
  }

  ~AKWayFMRefiner() {
    delete refiner;
  }

  Hypergraph hypergraph;
  Configuration config;
  KWayFMRefinerSimpleStopping* refiner;
};

TEST_F(AKWayFMRefiner, IdentifiesBorderHypernodes) {
  ASSERT_THAT(refiner->isBorderNode(0), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(6), Eq(true));
  ASSERT_THAT(refiner->isBorderNode(7), Eq(false));
}

TEST_F(AKWayFMRefiner, ComputesGainOfHypernodeMoves) {
  // hypergraph with positive, zero and negative gain nodes
  Hypergraph hgr(9, 5, HyperedgeIndexVector { 0, 2, 4, 8, 10, /*sentinel*/ 13 },
                 HyperedgeVector { 0, 1, 1, 2, 2, 3, 4, 5, 5, 6, 6, 7, 8 }, 4);
  hgr.setNodePart(0, 0);
  hgr.setNodePart(1, 1);
  hgr.setNodePart(2, 0);
  hgr.setNodePart(3, 3);
  hgr.setNodePart(4, 3);
  hgr.setNodePart(5, 2);
  hgr.setNodePart(6, 2);
  hgr.setNodePart(7, 2);
  hgr.setNodePart(8, 1);
  delete refiner;
  refiner = new KWayFMRefinerSimpleStopping(hgr, config);

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

TEST_F(AKWayFMRefiner, ActivatesBorderNodes) {
  refiner->activate(1);

  ASSERT_THAT(refiner->_pq.max(), Eq(1));
  ASSERT_THAT(refiner->_pq.maxKey(), Eq(0));
}

TEST_F(AKWayFMRefiner, DoesNotActivateInternalNodes) {
  refiner->activate(7);

  ASSERT_THAT(refiner->_pq.contains(7), Eq(false));
}
} // namespace partition
