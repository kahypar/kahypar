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

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/meta/registrar.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/partition/refinement/do_nothing_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
#define REGISTER_REFINER(id, refiner)                                 \
  static meta::Registrar<RefinerFactory> register_ ## refiner(        \
    id,                                                               \
    [](Hypergraph& hypergraph, const Context& context) -> IRefiner* { \
    return new refiner(hypergraph, context);                          \
  })

REGISTER_REFINER(RefinementAlgorithm::do_nothing, DoNothingRefiner);

using TwoWayFMRefinerSimpleStopping = TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;

class ATwoWayFMRefiner : public Test {
 public:
  ATwoWayFMRefiner() :
    hypergraph(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    context(),
    refiner(nullptr),
    changes() {
    hypergraph->setNodePart(0, 0);
    hypergraph->setNodePart(1, 1);
    hypergraph->setNodePart(2, 1);
    hypergraph->setNodePart(3, 0);
    hypergraph->setNodePart(4, 0);
    hypergraph->setNodePart(5, 1);
    hypergraph->setNodePart(6, 1);
    hypergraph->initializeNumCutHyperedges();
    context.partition.epsilon = 0.15;
    context.partition.k = 2;
    context.partition.objective = Objective::cut;
    context.partition.perfect_balance_part_weights.push_back(ceil(hypergraph->totalWeight() /
                                                                  static_cast<double>(context.partition.k)));
    context.partition.perfect_balance_part_weights.push_back(ceil(hypergraph->totalWeight() /
                                                                  static_cast<double>(context.partition.k)));

    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[0]);
    context.partition.max_part_weights.push_back(context.partition.max_part_weights[0]);
    context.local_search.fm.max_number_of_fruitless_moves = 50;
    refiner = std::make_unique<TwoWayFMRefinerSimpleStopping>(*hypergraph, context);
    changes.representative.push_back(0);
    changes.contraction_partner.push_back(0);
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Context context;
  std::unique_ptr<TwoWayFMRefinerSimpleStopping> refiner;
  UncontractionGainChanges changes;
};

class AGainUpdateMethod : public Test {
 public:
  AGainUpdateMethod() :
    context() {
    context.partition.k = 2;
    context.partition.objective = Objective::cut;
    context.local_search.fm.max_number_of_fruitless_moves = 50;
  }

  Context context;
};

using ATwoWayFMRefinerDeathTest = ATwoWayFMRefiner;

TEST_F(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement) {
  refiner->initialize(100);
  ASSERT_THAT(refiner->computeGain(6), Eq(0));
  ASSERT_THAT(refiner->computeGain(1), Eq(1));
  ASSERT_THAT(refiner->computeGain(5), Eq(-1));
}

TEST_F(ATwoWayFMRefiner, ActivatesBorderNodes) {
  refiner->initialize(100);
  refiner->activate(1,  /* dummy max-part-weight */ { 42, 42 });
  HypernodeID hn;
  HyperedgeWeight gain;
  PartitionID to_part;
  refiner->_pq.deleteMax(hn, gain, to_part);
  ASSERT_THAT(hn, Eq(1));
  ASSERT_THAT(gain, Eq(1));
}

TEST_F(ATwoWayFMRefiner, CalculatesNodeCountsInBothPartitions) {
  refiner->initialize(100);
  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(3));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(4));
}

TEST_F(ATwoWayFMRefiner, DoesNotViolateTheBalanceConstraint) {
  Metrics old_metrics = { metrics::hyperedgeCut(*hypergraph),
                          metrics::km1(*hypergraph),
                          metrics::imbalance(*hypergraph, context) };
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->initialize(100);
  refiner->refine(refinement_nodes, { 42, 42 }, changes, old_metrics);

  EXPECT_PRED_FORMAT2(::testing::DoubleLE, metrics::imbalance(*hypergraph, context),
                      old_metrics.imbalance);
}


TEST_F(ATwoWayFMRefiner, UpdatesNodeCountsOnNodeMovements) {
  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(3));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(4));

  refiner->initialize(100);
  refiner->moveHypernode(1, 1, 0);

  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(4));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(3));
}

TEST_F(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack) {
  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(3));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(4));
  Metrics old_metrics = { metrics::hyperedgeCut(*hypergraph),
                          metrics::km1(*hypergraph),
                          metrics::imbalance(*hypergraph, context) };
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->initialize(100);
  refiner->refine(refinement_nodes, { 42, 42 }, changes, old_metrics);

  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(4));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(3));
}

TEST_F(ATwoWayFMRefiner, PerformsCompleteRollBackIfNoImprovementCouldBeFound) {
  refiner.reset(new TwoWayFMRefinerSimpleStopping(*hypergraph, context));
  hypergraph->changeNodePart(1, 1, 0);
  ASSERT_THAT(hypergraph->partID(6), Eq(1));
  ASSERT_THAT(hypergraph->partID(2), Eq(1));
  refiner->initialize(100);

  Metrics old_metrics = { metrics::hyperedgeCut(*hypergraph),
                          metrics::km1(*hypergraph),
                          metrics::imbalance(*hypergraph, context) };
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->refine(refinement_nodes, { 42, 42 }, changes, old_metrics);

  ASSERT_THAT(hypergraph->partID(6), Eq(1));
  ASSERT_THAT(hypergraph->partID(2), Eq(1));
}

TEST_F(ATwoWayFMRefiner, RollsBackAllNodeMovementsIfCutCouldNotBeImproved) {
  Metrics old_metrics = { metrics::hyperedgeCut(*hypergraph),
                          metrics::km1(*hypergraph),
                          metrics::imbalance(*hypergraph, context) };
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->initialize(100);
  refiner->refine(refinement_nodes, { 42, 42 }, changes, old_metrics);

  ASSERT_THAT(old_metrics.cut, Eq(metrics::hyperedgeCut(*hypergraph)));
  ASSERT_THAT(hypergraph->partID(1), Eq(0));
  ASSERT_THAT(hypergraph->partID(5), Eq(1));
}

// Ugly: We could seriously need Mocks here!
TEST_F(AGainUpdateMethod, RespectsPositiveGainUpdateSpecialCaseForHyperedgesOfSize2) {
  Hypergraph hypergraph(2, 1, HyperedgeIndexVector { 0, 2 }, HyperedgeVector { 0, 1 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._hg.activate(0);
  refiner._hg.activate(1);
  refiner._gain_cache.setValue(0, refiner.computeGain(0));
  refiner._gain_cache.setValue(1, refiner.computeGain(1));
  refiner._pq.insert(0, 1, refiner._gain_cache.value(0));
  refiner._pq.insert(1, 1, refiner._gain_cache.value(0));
  refiner._pq.enablePart(1);
  ASSERT_THAT(refiner._pq.key(0, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(-1));

  hypergraph.changeNodePart(1, 0, 1);
  refiner._hg.mark(1);

  refiner.updateNeighbours(1, 0, 1,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(1));
  // updateNeighbours does not delete the current max_gain node, neither does it update its gain
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(-1));
}

TEST_F(AGainUpdateMethod, RespectsNegativeGainUpdateSpecialCaseForHyperedgesOfSize2) {
  Hypergraph hypergraph(3, 2, HyperedgeIndexVector { 0, 2, 4 }, HyperedgeVector { 0, 1, 0, 2 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 1);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  refiner.activate(0,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(1,  /* dummy max-part-weight */ { 42, 42 });
  ASSERT_THAT(refiner._pq.key(0, 1), Eq(2));
  ASSERT_THAT(refiner._pq.key(1, 0), Eq(1));
  refiner._pq.enablePart(0);
  refiner._pq.enablePart(1);

  hypergraph.changeNodePart(1, 1, 0);
  refiner._hg.mark(1);
  refiner.updateNeighbours(1, 1, 0,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
}

TEST_F(AGainUpdateMethod, HandlesCase0To1) {
  Hypergraph hypergraph(4, 1, HyperedgeIndexVector { 0, 4 }, HyperedgeVector { 0, 1, 2, 3 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 0);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._hg.activate(0);
  refiner._hg.activate(1);
  refiner._hg.activate(2);
  refiner._hg.activate(3);
  refiner._gain_cache.setValue(0, refiner.computeGain(0));
  refiner._gain_cache.setValue(1, refiner.computeGain(1));
  refiner._gain_cache.setValue(2, refiner.computeGain(2));
  refiner._gain_cache.setValue(3, refiner.computeGain(3));
  refiner._pq.insert(0, 1, refiner._gain_cache.value(0));
  refiner._pq.insert(1, 1, refiner._gain_cache.value(1));
  refiner._pq.insert(2, 1, refiner._gain_cache.value(2));
  refiner._pq.insert(3, 1, refiner._gain_cache.value(3));
  refiner._pq.enablePart(1);

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.key(2, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.key(3, 1), Eq(-1));

  hypergraph.changeNodePart(3, 0, 1);
  refiner._hg.mark(3);
  refiner.updateNeighbours(3, 0, 1,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(2, 1), Eq(0));
}

TEST_F(AGainUpdateMethod, HandlesCase1To0) {
  Hypergraph hypergraph(5, 2, HyperedgeIndexVector { 0, 4, 8 }, HyperedgeVector { 0, 1, 2, 3, 0, 1, 2, 4 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._gain_cache.setValue(0, refiner.computeGain(0));
  refiner._gain_cache.setValue(1, refiner.computeGain(1));
  refiner._gain_cache.setValue(2, refiner.computeGain(2));
  refiner._gain_cache.setValue(3, refiner.computeGain(3));
  refiner._gain_cache.setValue(4, refiner.computeGain(4));
  refiner.activate(0,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(1,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(2,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(3,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(4,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(2, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(3, 0), Eq(1));

  hypergraph.changeNodePart(3, 1, 0);
  refiner._hg.mark(3);
  refiner.updateNeighbours(3, 1, 0,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.key(2, 1), Eq(-1));
}

TEST_F(AGainUpdateMethod, HandlesCase2To1) {
  Hypergraph hypergraph(4, 1, HyperedgeIndexVector { 0, 4 }, HyperedgeVector { 0, 1, 2, 3 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  refiner.activate(0,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(1,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(2,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(3,  /* dummy max-part-weight */ { 42, 42 });
  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(2, 0), Eq(0));
  ASSERT_THAT(refiner._pq.key(3, 0), Eq(0));

  hypergraph.changeNodePart(3, 1, 0);
  refiner._hg.mark(3);
  refiner.updateNeighbours(3, 1, 0,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(2, 0), Eq(1));
}

TEST_F(AGainUpdateMethod, HandlesCase1To2) {
  Hypergraph hypergraph(4, 1, HyperedgeIndexVector { 0, 4 }, HyperedgeVector { 0, 1, 2, 3 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  refiner.activate(0,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(1,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(2,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(3,  /* dummy max-part-weight */ { 42, 42 });
  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(2, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(3, 0), Eq(1));

  hypergraph.changeNodePart(2, 0, 1);
  refiner._hg.mark(2);
  refiner.updateNeighbours(2, 0, 1,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(3, 0), Eq(0));
}

TEST_F(AGainUpdateMethod, HandlesSpecialCaseOfHyperedgeWith3Pins) {
  Hypergraph hypergraph(3, 1, HyperedgeIndexVector { 0, 3 }, HyperedgeVector { 0, 1, 2 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  refiner.activate(0,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(1,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(2,  /* dummy max-part-weight */ { 42, 42 });
  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(2, 0), Eq(1));

  hypergraph.changeNodePart(1, 0, 1);
  refiner._hg.mark(1);
  refiner.updateNeighbours(1, 0, 1,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(1));
  ASSERT_THAT(refiner._pq.key(2, 0), Eq(0));
}

TEST_F(AGainUpdateMethod, RemovesNonBorderNodesFromPQ) {
  Hypergraph hypergraph(3, 1, HyperedgeIndexVector { 0, 3 }, HyperedgeVector { 0, 1, 2 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 0);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  refiner.activate(0,  /* dummy max-part-weight */ { 42, 42 });
  refiner.activate(1,  /* dummy max-part-weight */ { 42, 42 });
  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 0), Eq(1));
  ASSERT_THAT(refiner._pq.contains(2, 1), Eq(false));
  ASSERT_THAT(refiner._pq.contains(0, 1), Eq(true));

  hypergraph.changeNodePart(1, 1, 0, refiner._non_border_hns_to_remove);
  refiner._hg.mark(1);
  refiner.updateNeighbours(1, 1, 0,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(1, 0), Eq(1));
  ASSERT_THAT(refiner._pq.contains(0), Eq(false));
  ASSERT_THAT(refiner._pq.contains(2), Eq(false));
}

TEST_F(AGainUpdateMethod, ActivatesUnmarkedNeighbors) {
  Hypergraph hypergraph(3, 1, HyperedgeIndexVector { 0, 3 }, HyperedgeVector { 0, 1, 2 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._gain_cache.setValue(0, refiner.computeGain(0));
  refiner._gain_cache.setValue(1, refiner.computeGain(1));
  refiner._pq.insert(0, 1, refiner._gain_cache.value(0));
  refiner._pq.insert(1, 1, refiner._gain_cache.value(1));
  refiner._hg.activate(0);
  refiner._hg.activate(1);
  refiner._pq.enablePart(1);
  ASSERT_THAT(refiner._pq.key(0, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.contains(2), Eq(false));

  hypergraph.changeNodePart(1, 0, 1);
  refiner._hg.mark(1);
  refiner.updateNeighbours(1, 0, 1,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.key(0, 1), Eq(0));
  ASSERT_THAT(refiner._pq.key(1, 1), Eq(-1));
  ASSERT_THAT(refiner._pq.contains(2, 1), Eq(true));
  ASSERT_THAT(refiner._pq.key(2, 1), Eq(0));
}

TEST_F(AGainUpdateMethod, DoesNotDeleteJustActivatedNodes) {
  Hypergraph hypergraph(5, 3, HyperedgeIndexVector { 0, 2, 5, 8 },
                        HyperedgeVector { 0, 1, 2, 3, 4, 2, 3, 4 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 0);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);

  // bypassing activate
  refiner._pq.insert(2, 1, refiner.computeGain(2));
  refiner._gain_cache.setValue(2, refiner.computeGain(2));
  refiner._hg.activate(2);
  refiner._pq.enablePart(1);
  refiner.moveHypernode(2, 0, 1);
  refiner._hg.mark(2);
  refiner.updateNeighbours(2, 0, 1,  /* dummy max-part-weight */ { 42, 42 });

  ASSERT_THAT(refiner._pq.contains(4, 1), Eq(true));
  ASSERT_THAT(refiner._pq.contains(3, 0), Eq(true));
}

TEST(ARefiner, ChecksIfMovePreservesBalanceConstraint) {
  Hypergraph hypergraph(4, 1, HyperedgeIndexVector { 0, 4 },
                        HyperedgeVector { 0, 1, 2, 3 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.initializeNumCutHyperedges();

  Context context;
  context.partition.epsilon = 0.02;
  context.partition.k = 2;
  context.partition.objective = Objective::cut;
  // To ensure that we can call moveIsFeasable.
  // This test is actcually legacy, since the current implementation does not
  // use the moveIsFeasible method anymore.
  context.partition.mode = Mode::direct_kway;
  context.partition.perfect_balance_part_weights.push_back(ceil(hypergraph.initialNumNodes() /
                                                                static_cast<double>(context.partition.k)));
  context.partition.perfect_balance_part_weights.push_back(ceil(hypergraph.initialNumNodes() /
                                                                static_cast<double>(context.partition.k)));

  context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                               * context.partition.perfect_balance_part_weights[0]);
  context.partition.max_part_weights.push_back(context.partition.max_part_weights[0]);

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, context);
  refiner.initialize(100);
  ASSERT_THAT(refiner.moveIsFeasible(1, 0, 1), Eq(true));
  ASSERT_THAT(refiner.moveIsFeasible(3, 1, 0), Eq(false));
}

TEST_F(ATwoWayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInitialGainComputation) {
  hypergraph.reset(new Hypergraph(2, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 3 },
                                  HyperedgeVector { 0, 1, 0 }, 2));

  context.local_search.fm.max_number_of_fruitless_moves = 50;
  context.partition.k = 2;
  context.partition.rb_lower_k = 0;
  context.partition.rb_upper_k = context.partition.k - 1;
  context.partition.epsilon = 1.0;
  context.partition.perfect_balance_part_weights.push_back(ceil(hypergraph->totalWeight() /
                                                                static_cast<double>(context.partition.k)));
  context.partition.perfect_balance_part_weights.push_back(ceil(hypergraph->totalWeight() /
                                                                static_cast<double>(context.partition.k)));

  context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                               * context.partition.perfect_balance_part_weights[0]);
  context.partition.max_part_weights.push_back(context.partition.max_part_weights[0]);

  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);
  hypergraph->initializeNumCutHyperedges();

  refiner.reset(new TwoWayFMRefinerSimpleStopping(*hypergraph, context));
  ASSERT_DEBUG_DEATH(refiner->initialize(100), ".*");
  ASSERT_DEBUG_DEATH(refiner->computeGain(0), ".*");
}

TEST_F(ATwoWayFMRefiner, KnowsIfAHyperedgeIsFullyActive) {
  hypergraph.reset(new Hypergraph(3, 1, HyperedgeIndexVector { 0,  /*sentinel*/ 3 },
                                  HyperedgeVector { 0, 1, 2 }, 2));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 0);
  hypergraph->initializeNumCutHyperedges();

  refiner.reset(new TwoWayFMRefinerSimpleStopping(*hypergraph, context));
  refiner->initialize(100);

  refiner->_hg.activate(0);
  hypergraph->changeNodePart(0, 0, 1);
  refiner->_hg.mark(0);
  refiner->fullUpdate(0, 1, 0);
  ASSERT_THAT(refiner->_he_fully_active[0], Eq(true));
}
}                // namespace kahypar
