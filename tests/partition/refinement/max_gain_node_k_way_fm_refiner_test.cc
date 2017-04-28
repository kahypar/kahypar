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
#include "kahypar/partition/refinement/kway_fm_max_gain_node_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
using KWayFMRefinerSimpleStopping = MaxGainNodeKWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;

class AMaxGainNodeKWayFMRefiner : public Test {
 public:
  AMaxGainNodeKWayFMRefiner() :
    context(),
    // example hypergraph with one additional HN and one additional HE
    hypergraph(new Hypergraph(8, 5, HyperedgeIndexVector { 0, 2, 6, 9, 12,  /*sentinel*/ 14 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6, 2, 7 }, 4)),
    refiner() {
    context.partition.k = 4;
    context.partition.rb_lower_k = 0;
    context.partition.rb_upper_k = context.partition.k - 1;
    hypergraph->setNodePart(0, 0);
    hypergraph->setNodePart(1, 0);
    hypergraph->setNodePart(2, 1);
    hypergraph->setNodePart(3, 2);
    hypergraph->setNodePart(4, 2);
    hypergraph->setNodePart(5, 3);
    hypergraph->setNodePart(6, 3);
    hypergraph->setNodePart(7, 1);
    hypergraph->initializeNumCutHyperedges();
    context.local_search.fm.max_number_of_fruitless_moves = 50;
    context.partition.total_graph_weight = 8;
    refiner = std::make_unique<KWayFMRefinerSimpleStopping>(*hypergraph, context);
    refiner->initialize(100);
  }

  Context context;
  std::unique_ptr<Hypergraph> hypergraph;
  std::unique_ptr<KWayFMRefinerSimpleStopping> refiner;
};

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
  hypergraph.reset(new Hypergraph(9, 5, HyperedgeIndexVector { 0, 2, 4, 8, 10, 13 },
                                  HyperedgeVector { 0, 1, 1, 2, 2, 3, 4, 5, 5, 6, 6, 7, 8 }, 4));
  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, context));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);
  hypergraph->setNodePart(2, 0);
  hypergraph->setNodePart(3, 3);
  hypergraph->setNodePart(4, 3);
  hypergraph->setNodePart(5, 2);
  hypergraph->setNodePart(6, 2);
  hypergraph->setNodePart(7, 2);
  hypergraph->setNodePart(8, 1);
  hypergraph->initializeNumCutHyperedges();
  refiner->initialize(100);

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
  hypergraph->initializeNumCutHyperedges();
  context.partition.k = 4;
  context.partition.rb_lower_k = 0;
  context.partition.rb_upper_k = context.partition.k - 1;
  context.partition.epsilon = 1.0;

  context.partition.perfect_balance_part_weights[0] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.perfect_balance_part_weights[1] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.max_part_weights[0] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[0];
  context.partition.max_part_weights[1] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[1];

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, context));
  refiner->initialize(100);

  refiner->moveHypernode(7, 3, 2);
  ASSERT_THAT(hypergraph->partID(7), Eq(2));
}

/*TEST_F(AMaxGainNodeKWayFMRefiner, PerformsCompleteRollbackIfNoImprovementCouldBeFound) {
  hypergraph.reset(new Hypergraph(8, 6, HyperedgeIndexVector { 0, 2, 5, 7, 9, 11, 13 },
                                  HyperedgeVector { 0, 1, 0, 1, 6, 1, 6, 2, 3, 4, 5, 6, 7 }, 4));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 1);
  hypergraph->setNodePart(3, 1);
  hypergraph->setNodePart(4, 2);
  hypergraph->setNodePart(5, 2);
  hypergraph->setNodePart(6, 3);
  hypergraph->setNodePart(7, 3);
  hypergraph->initializeNumCutHyperedges();

  Hypergraph orig_hgr(8, 6, HyperedgeIndexVector { 0, 2, 5, 7, 9, 11,  sentinel 13 },
                      HyperedgeVector { 0, 1, 0, 1, 6, 1, 6, 2, 3, 4, 5, 6, 7 }, 4);
  orig_hgr.setNodePart(0, 0);
  orig_hgr.setNodePart(1, 0);
  orig_hgr.setNodePart(2, 1);
  orig_hgr.setNodePart(3, 1);
  orig_hgr.setNodePart(4, 2);
  orig_hgr.setNodePart(5, 2);
  orig_hgr.setNodePart(6, 3);
  orig_hgr.setNodePart(7, 3);
  orig_hgr.initializeNumCutHyperedges();

  context.partition.k = 4;
  context.partition.rb_lower_k = 0;
  context.partition.rb_upper_k = context.partition.k - 1;
  context.partition.epsilon = 1.0;
  context.partition.perfect_balance_part_weights[0] = ceil(8.0 / 4);
  context.partition.perfect_balance_part_weights[1] = ceil(8.0 / 4);
  context.partition.max_part_weights[0] = 0;
  context.partition.max_part_weights[1] = 0;

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, context));
#ifdef USE_BUCKET_QUEUE
  refiner->initialize(100);
#else
  refiner->initialize();
#endif

  double old_imbalance = metrics::imbalance(*hypergraph, context);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(*hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 0, 1 };

  refiner->refine(refinement_nodes, 2, context.partition.max_part_weights, { 0, 0 }, old_cut, old_imbalance);

  ASSERT_THAT(verifyEquivalenceWithPartitionInfo(orig_hgr, *hypergraph), Eq(true));
}*/

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
  hypergraph->initializeNumCutHyperedges();

  context.partition.k = 4;
  context.partition.rb_lower_k = 0;
  context.partition.rb_upper_k = context.partition.k - 1;
  context.partition.epsilon = 1.0;
  context.partition.perfect_balance_part_weights[0] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.perfect_balance_part_weights[1] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.max_part_weights[0] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[0];
  context.partition.max_part_weights[1] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[1];

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, context));
  refiner->initialize(100);

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
  hypergraph->initializeNumCutHyperedges();
  hypergraph->printGraphState();
  context.partition.k = 4;
  context.partition.rb_lower_k = 0;
  context.partition.rb_upper_k = context.partition.k - 1;
  context.partition.epsilon = 1.0;
  context.partition.perfect_balance_part_weights[0] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.perfect_balance_part_weights[1] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.max_part_weights[0] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[0];
  context.partition.max_part_weights[1] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[1];

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, context));
  refiner->initialize(100);

  ASSERT_THAT(refiner->computeMaxGainMove(0).first, Eq(0));
}

TEST_F(AMaxGainNodeKWayFMRefiner, ChoosesMaxGainMoveHNWithHighesConnectivityDecrease) {
  // hypergraph with zero gain moves and different connectivity decrease values
  // Move of HN 3 to part 1 has decrease -1 and move to part 2 has decrease +1
  hypergraph.reset(new Hypergraph(10, 4, HyperedgeIndexVector { 0, 4, 7, 10,  /*sentinel*/ 13 },
                                  HyperedgeVector { 0, 1, 2, 3, 3, 4, 5, 3, 8, 9, 3, 6, 7 }, 3));

  context.partition.k = 3;
  context.partition.rb_lower_k = 0;
  context.partition.rb_upper_k = context.partition.k - 1;
  context.partition.epsilon = 1.0;
  context.partition.perfect_balance_part_weights[0] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.perfect_balance_part_weights[1] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.max_part_weights[0] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[0];
  context.partition.max_part_weights[1] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[1];

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
  hypergraph->initializeNumCutHyperedges();

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, context));
  refiner->initialize(100);

  ASSERT_THAT(refiner->computeMaxGainMove(3).first, Eq(0));
  ASSERT_THAT(refiner->computeMaxGainMove(3).second, Eq(2));
}

TEST_F(AMaxGainNodeKWayFMRefiner, ConsidersSingleNodeHEsDuringGainComputation) {
  hypergraph.reset(new Hypergraph(2, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 3 },
                                  HyperedgeVector { 0, 1, 0 }, 2));

  context.partition.k = 2;
  context.partition.rb_lower_k = 0;
  context.partition.rb_upper_k = context.partition.k - 1;
  context.partition.epsilon = 1.0;
  context.partition.perfect_balance_part_weights[0] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.perfect_balance_part_weights[1] = ceil(hypergraph->initialNumNodes()
                                                           / static_cast<double>(context.partition.k));
  context.partition.max_part_weights[0] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[0];
  context.partition.max_part_weights[1] =
    (1 + context.partition.epsilon)
    * context.partition.perfect_balance_part_weights[1];

  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);
  hypergraph->initializeNumCutHyperedges();

  refiner.reset(new KWayFMRefinerSimpleStopping(*hypergraph, context));
  refiner->initialize(100);

  ASSERT_THAT(refiner->computeMaxGainMove(0).first, Eq(1));
  ASSERT_THAT(refiner->computeMaxGainMove(0).second, Eq(1));
}
}  // namespace kahypar
