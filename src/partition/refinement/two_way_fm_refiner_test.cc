/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "partition/Metrics.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeight;
using defs::HypernodeID;

namespace partition {
using TwoWayFMRefinerSimpleStopping = TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch,
                                                      /*global rebalancing */ true>;

class ATwoWayFMRefiner : public Test {
 public:
  ATwoWayFMRefiner() :
    hypergraph(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    config(),
    refiner(nullptr) {
    hypergraph->setNodePart(0, 0);
    hypergraph->setNodePart(1, 1);
    hypergraph->setNodePart(2, 1);
    hypergraph->setNodePart(3, 0);
    hypergraph->setNodePart(4, 0);
    hypergraph->setNodePart(5, 1);
    hypergraph->setNodePart(6, 1);
    hypergraph->initializeNumCutHyperedges();
    config.partition.epsilon = 0.15;
    config.partition.total_graph_weight = 7;
    config.partition.perfect_balance_part_weights[0] = ceil(config.partition.total_graph_weight /
                                                            static_cast<double>(config.partition.k));
    config.partition.perfect_balance_part_weights[1] = ceil(config.partition.total_graph_weight /
                                                            static_cast<double>(config.partition.k));

    config.partition.max_part_weights[0] = (1 + config.partition.epsilon)
                                           * config.partition.perfect_balance_part_weights[0];
    config.partition.max_part_weights[1] = config.partition.max_part_weights[0];
    config.fm_local_search.max_number_of_fruitless_moves = 50;
    refiner = std::make_unique<TwoWayFMRefinerSimpleStopping>(*hypergraph, config);
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Configuration config;
  std::unique_ptr<TwoWayFMRefinerSimpleStopping> refiner;
};

class AGainUpdateMethod : public Test {
 public:
  AGainUpdateMethod() :
    config() {
    config.fm_local_search.max_number_of_fruitless_moves = 50;
  }

  Configuration config;
};

using ATwoWayFMRefinerDeathTest = ATwoWayFMRefiner;

TEST_F(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement) {
  #ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif
  ASSERT_THAT(refiner->computeGain(6), Eq(0));
  ASSERT_THAT(refiner->computeGain(1), Eq(1));
  ASSERT_THAT(refiner->computeGain(5), Eq(-1));
}

TEST_F(ATwoWayFMRefiner, ActivatesBorderNodes) {
  #ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif
  refiner->activate(1,  /* dummy max-part-weight */ { 42, 42 });
  HypernodeID hn;
  HyperedgeWeight gain;
  PartitionID to_part;
  refiner->_pq.deleteMax(hn, gain, to_part);
  ASSERT_THAT(hn, Eq(1));
  ASSERT_THAT(gain, Eq(1));
}

TEST_F(ATwoWayFMRefiner, CalculatesNodeCountsInBothPartitions) {
  #ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif
  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(3));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(4));
}

TEST_F(ATwoWayFMRefiner, DoesNotViolateTheBalanceConstraint) {
  double old_imbalance = metrics::imbalance(*hypergraph, config);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(*hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  #ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif
  refiner->refine(refinement_nodes, 2, { 42, 42 }, { 0, 0 }, old_cut, old_imbalance);

  EXPECT_PRED_FORMAT2(::testing::DoubleLE, metrics::imbalance(*hypergraph, config),
                      old_imbalance);
}


TEST_F(ATwoWayFMRefiner, UpdatesNodeCountsOnNodeMovements) {
  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(3));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(4));

  #ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif
  refiner->moveHypernode(1, 1, 0);

  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(4));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(3));
}

TEST_F(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack) {
  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(3));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(4));
  double old_imbalance = metrics::imbalance(*hypergraph, config);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(*hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

#ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif
  refiner->refine(refinement_nodes, 2, { 42, 42 }, { 0, 0 }, old_cut, old_imbalance);

  ASSERT_THAT(refiner->_hg.partWeight(0), Eq(3));
  ASSERT_THAT(refiner->_hg.partWeight(1), Eq(4));
}

TEST_F(ATwoWayFMRefiner, PerformsCompleteRollBackIfNoImprovementCouldBeFound) {
  refiner.reset(new TwoWayFMRefinerSimpleStopping(*hypergraph, config));
  hypergraph->changeNodePart(1, 1, 0);
  ASSERT_THAT(hypergraph->partID(6), Eq(1));
  ASSERT_THAT(hypergraph->partID(2), Eq(1));

#ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif  // already done above

  double old_imbalance = metrics::imbalance(*hypergraph, config);
  HyperedgeWeight old_cut = metrics::hyperedgeCut(*hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

  refiner->refine(refinement_nodes, 2, { 42, 42 }, { 0, 0 }, old_cut, old_imbalance);

  ASSERT_THAT(hypergraph->partID(6), Eq(1));
  ASSERT_THAT(hypergraph->partID(2), Eq(1));
}

TEST_F(ATwoWayFMRefiner, RollsBackAllNodeMovementsIfCutCouldNotBeImproved) {
  double old_imbalance = metrics::imbalance(*hypergraph, config);
  HyperedgeWeight cut = metrics::hyperedgeCut(*hypergraph);
  std::vector<HypernodeID> refinement_nodes = { 1, 6 };

#ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif
  refiner->refine(refinement_nodes, 2, { 42, 42 }, { 0, 0 }, cut, old_imbalance);

  ASSERT_THAT(cut, Eq(metrics::hyperedgeCut(*hypergraph)));
  ASSERT_THAT(hypergraph->partID(1), Eq(0));
  ASSERT_THAT(hypergraph->partID(5), Eq(1));
}

// Ugly: We could seriously need Mocks here!
TEST_F(AGainUpdateMethod, RespectsPositiveGainUpdateSpecialCaseForHyperedgesOfSize2) {
  Hypergraph hypergraph(2, 1, HyperedgeIndexVector { 0, 2 }, HyperedgeVector { 0, 1 });
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.initializeNumCutHyperedges();

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._hg.activate(0);
  refiner._hg.activate(1);
  refiner._rebalance_pqs[1].deleteNode(0);
  refiner._disabled_rebalance_hns.set(0, 0);
  refiner._rebalance_pqs[1].deleteNode(1);
  refiner._disabled_rebalance_hns.set(1, 1);
  refiner._gain_cache[0] = refiner.computeGain(0);
  refiner._gain_cache[1] = refiner.computeGain(1);
  refiner._pq.insert(0, 1, refiner._gain_cache[0]);
  refiner._pq.insert(1, 1, refiner._gain_cache[0]);
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._hg.activate(0);
  refiner._hg.activate(1);
  refiner._hg.activate(2);
  refiner._hg.activate(3);
  refiner._rebalance_pqs[1].deleteNode(0);
  refiner._disabled_rebalance_hns.set(0, 0);
  refiner._rebalance_pqs[1].deleteNode(1);
  refiner._disabled_rebalance_hns.set(1, 1);
  refiner._rebalance_pqs[1].deleteNode(2);
  refiner._disabled_rebalance_hns.set(2, 2);
  refiner._rebalance_pqs[1].deleteNode(3);
  refiner._disabled_rebalance_hns.set(3, 3);
  refiner._gain_cache[0] = refiner.computeGain(0);
  refiner._gain_cache[1] = refiner.computeGain(1);
  refiner._gain_cache[2] = refiner.computeGain(2);
  refiner._gain_cache[3] = refiner.computeGain(3);
  refiner._pq.insert(0, 1, refiner._gain_cache[0]);
  refiner._pq.insert(1, 1, refiner._gain_cache[1]);
  refiner._pq.insert(2, 1, refiner._gain_cache[2]);
  refiner._pq.insert(3, 1, refiner._gain_cache[3]);
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._gain_cache[0] = refiner.computeGain(0);
  refiner._gain_cache[1] = refiner.computeGain(1);
  refiner._gain_cache[2] = refiner.computeGain(2);
  refiner._gain_cache[3] = refiner.computeGain(3);
  refiner._gain_cache[4] = refiner.computeGain(4);
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
  // bypassing activate since neither 0 nor 1 is actually a border node
  refiner._gain_cache[0] = refiner.computeGain(0);
  refiner._gain_cache[1] = refiner.computeGain(1);
  refiner._pq.insert(0, 1, refiner._gain_cache[0]);
  refiner._pq.insert(1, 1, refiner._gain_cache[1]);
  refiner._hg.activate(0);
  refiner._hg.activate(1);
  refiner._rebalance_pqs[1].deleteNode(0);
  refiner._disabled_rebalance_hns.set(0, 0);
  refiner._rebalance_pqs[1].deleteNode(1);
  refiner._disabled_rebalance_hns.set(1, 1);
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

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif

  // bypassing activate
  refiner._pq.insert(2, 1, refiner.computeGain(2));
  refiner._gain_cache[2] = refiner.computeGain(2);
  refiner._hg.activate(2);
  refiner._rebalance_pqs[1].deleteNode(2);
  refiner._disabled_rebalance_hns.set(2, 2);
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

  Configuration config;
  config.partition.epsilon = 0.02;
  config.partition.perfect_balance_part_weights[0] = ceil(hypergraph.initialNumNodes() /
                                                          static_cast<double>(config.partition.k));
  config.partition.perfect_balance_part_weights[1] = ceil(hypergraph.initialNumNodes() /
                                                          static_cast<double>(config.partition.k));

  config.partition.max_part_weights[0] = (1 + config.partition.epsilon)
                                         * config.partition.perfect_balance_part_weights[0];
  config.partition.max_part_weights[1] = config.partition.max_part_weights[0];

  TwoWayFMRefinerSimpleStopping refiner(hypergraph, config);
  #ifdef USE_BUCKET_PQ
  refiner.initialize(100);
#else
  refiner.initialize();
#endif
  ASSERT_THAT(refiner.moveIsFeasible(1, 0, 1), Eq(true));
  ASSERT_THAT(refiner.moveIsFeasible(3, 1, 0), Eq(false));
}

TEST_F(ATwoWayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInitialGainComputation) {
  hypergraph.reset(new Hypergraph(2, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 3 },
                                  HyperedgeVector { 0, 1, 0 }, 2));

  config.fm_local_search.max_number_of_fruitless_moves = 50;
  config.partition.total_graph_weight = 2;
  config.partition.k = 2;
  config.partition.rb_lower_k = 0;
  config.partition.rb_upper_k = config.partition.k - 1;
  config.partition.epsilon = 1.0;
  config.partition.perfect_balance_part_weights[0] = ceil(config.partition.total_graph_weight /
                                                          static_cast<double>(config.partition.k));
  config.partition.perfect_balance_part_weights[1] = ceil(config.partition.total_graph_weight /
                                                          static_cast<double>(config.partition.k));

  config.partition.max_part_weights[0] = (1 + config.partition.epsilon)
                                         * config.partition.perfect_balance_part_weights[0];
  config.partition.max_part_weights[1] = config.partition.max_part_weights[0];

  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);
  hypergraph->initializeNumCutHyperedges();

  refiner.reset(new TwoWayFMRefinerSimpleStopping(*hypergraph, config));
#ifdef USE_BUCKET_PQ
  ASSERT_DEBUG_DEATH(refiner->initialize(100), ".*");
#else
  ASSERT_DEBUG_DEATH(refiner->initialize(), ".*");
#endif

  ASSERT_DEBUG_DEATH(refiner->computeGain(0), ".*");
}

TEST_F(ATwoWayFMRefiner, KnowsIfAHyperedgeIsFullyActive) {
  hypergraph.reset(new Hypergraph(3, 1, HyperedgeIndexVector { 0,  /*sentinel*/ 3 },
                                  HyperedgeVector { 0, 1, 2 }, 2));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 0);
  hypergraph->initializeNumCutHyperedges();

  refiner.reset(new TwoWayFMRefinerSimpleStopping(*hypergraph, config));

#ifdef USE_BUCKET_PQ
  refiner->initialize(100);
#else
  refiner->initialize();
#endif

  refiner->_hg.activate(0);
  refiner->_rebalance_pqs[1].deleteNode(0);
  refiner->_disabled_rebalance_hns.set(0, 0);
  hypergraph->changeNodePart(0, 0, 1);
  refiner->_hg.mark(0);
  refiner->fullUpdate(0, 1, 0);
  ASSERT_THAT(refiner->_he_fully_active[0], Eq(true));
}
}                // namespace partition
