/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "lib/macros.h"
#include "partition/Configuration.h"
#include "partition/Partitioner.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMQueueSelectionPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;

using partition::EligibleTopGain;
using partition::RemoveOnlyTheCloggingEntry;

namespace partition {
typedef Rater<defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef HeuristicHeavyEdgeCoarsener<FirstWinsRater> FirstWinsCoarsener;
typedef TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch,
                        EligibleTopGain, RemoveOnlyTheCloggingEntry> Refiner;

class APartitioner : public Test {
  public:
  APartitioner(Hypergraph* graph =
                 new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
                                HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })) :
    hypergraph(graph),
    config(),
    partitioner(config),
    coarsener(new FirstWinsCoarsener(*hypergraph, config)),
    refiner(new Refiner(*hypergraph, config)) {
    config.coarsening.contraction_limit = 2;
    config.coarsening.max_allowed_node_weight = 5;
    config.partition.graph_filename = "PartitionerTest.hgr";
    config.partition.graph_partition_filename = "PartitionerTest.hgr.part.2.KaHyPar";
    config.partition.coarse_graph_filename = "PartitionerTest_coarse.hgr";
    config.partition.coarse_graph_partition_filename = "PartitionerTest_coarse.hgr.part.2";
    config.partition.epsilon = 0.15;
    config.partition.k = 2;
    config.partition.total_graph_weight = 7;
    config.partition.max_part_weight = (1 + config.partition.epsilon)
                                       * ceil(7 / static_cast<double>(config.partition.k));
    double exp = 1.0 / log2(config.partition.k);
    config.partition.hmetis_ub_factor =
      50.0 * (2 * pow((1 + config.partition.epsilon), exp)
              * pow(ceil(static_cast<double>(config.partition.total_graph_weight)
                         / config.partition.k) / config.partition.total_graph_weight, exp) - 1);
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Configuration config;
  Partitioner partitioner;
  std::unique_ptr<ICoarsener> coarsener;
  std::unique_ptr<IRefiner> refiner;
};

class APartitionerWithHyperedgeSizeThreshold : public APartitioner {
  public:
  APartitionerWithHyperedgeSizeThreshold() :
    APartitioner() {
    config.partition.hyperedge_size_threshold = 3;
  }
};

TEST_F(APartitioner, UseshMetisPartitioningOnCoarsestHypergraph) {
  partitioner.partition(*hypergraph, *coarsener, *refiner);
  ASSERT_THAT(hypergraph->partID(1), Eq(0));
  ASSERT_THAT(hypergraph->partID(3), Eq(1));
}

TEST_F(APartitioner, UncoarsensTheInitiallyPartitionedHypergraph) {
  partitioner.partition(*hypergraph, *coarsener, *refiner);
  ASSERT_THAT(hypergraph->partID(0), Eq(0));
  ASSERT_THAT(hypergraph->partID(1), Eq(0));
  ASSERT_THAT(hypergraph->partID(2), Eq(0));
  ASSERT_THAT(hypergraph->partID(3), Eq(1));
  ASSERT_THAT(hypergraph->partID(4), Eq(1));
  ASSERT_THAT(hypergraph->partID(5), Eq(0));
  ASSERT_THAT(hypergraph->partID(6), Eq(1));
}

TEST_F(APartitioner, CalculatesPinCountsOfAHyperedgesAfterInitialPartitioning) {
  ASSERT_THAT(hypergraph->pinCountInPart(0, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 1), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 1), Eq(0));
  partitioner.partition(*hypergraph, *coarsener, *refiner);
  ASSERT_THAT(hypergraph->pinCountInPart(0, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 1), Eq(2));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 0), Eq(3));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 1), Eq(0));
}

TEST_F(APartitionerWithHyperedgeSizeThreshold,
       RemovesHyperedgesExceedingThreshold) {
  std::vector<HyperedgeID> removed_hyperedges;
  partitioner.removeLargeHyperedges(*hypergraph, removed_hyperedges);

  ASSERT_THAT(hypergraph->edgeIsEnabled(1), Eq(false));
}

TEST_F(APartitionerWithHyperedgeSizeThreshold,
       RestoresHyperedgesExceedingThreshold) {
  std::vector<HyperedgeID> removed_hyperedges;
  partitioner.removeLargeHyperedges(*hypergraph, removed_hyperedges);
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(2, 1);
  hypergraph->setNodePart(3, 0);
  hypergraph->setNodePart(4, 0);
  hypergraph->setNodePart(5, 1);
  hypergraph->setNodePart(6, 1);

  partitioner.restoreLargeHyperedges(*hypergraph, removed_hyperedges);
  ASSERT_THAT(hypergraph->edgeIsEnabled(1), Eq(true));
}

TEST_F(APartitionerWithHyperedgeSizeThreshold,
       TriesToMinimizesCutIfNoPinOfRemainingHyperedgeIsPartitioned) {
  hypergraph.reset(new Hypergraph(7, 3, HyperedgeIndexVector { 0, 2, 4, /*sentinel*/ 7 },
                                  HyperedgeVector { 0, 1, 2, 3, 4, 5, 6 }));

  std::vector<HyperedgeID> removed_hyperedges { 1, 2 };

  for (auto removed_edge : removed_hyperedges) {
    partitioner.partitionUnpartitionedPins(removed_edge, *hypergraph);
  }

  ASSERT_THAT(hypergraph->partID(2), Eq(0));
  ASSERT_THAT(hypergraph->partID(3), Eq(0));
  ASSERT_THAT(hypergraph->partID(4), Eq(1));
  ASSERT_THAT(hypergraph->partID(5), Eq(1));
  ASSERT_THAT(hypergraph->partID(6), Eq(1));
}

TEST_F(APartitionerWithHyperedgeSizeThreshold,
       TriesToMinimizesCutIfOnlyOnePartitionIsUsed) {
  hypergraph.reset(new Hypergraph(7, 3, HyperedgeIndexVector { 0, 2, 4, /*sentinel*/ 7 },
                                  HyperedgeVector { 0, 1, 2, 3, 4, 5, 6 }));

  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);
  hypergraph->setNodePart(2, 0);
  hypergraph->setNodePart(4, 1);
  hypergraph->setNodePart(5, 1);
  hypergraph->setNodePart(6, 1);

  std::vector<HyperedgeID> removed_hyperedges { 1 };

  for (auto removed_edge : removed_hyperedges) {
    partitioner.partitionUnpartitionedPins(removed_edge, *hypergraph);
  }

  ASSERT_THAT(hypergraph->partID(3), Eq(0));
}

TEST_F(APartitionerWithHyperedgeSizeThreshold,
       DistributesAllRemainingHypernodesToMinimizeImbalaceIfCutCannotBeMinimized) {
  hypergraph.reset(new Hypergraph(7, 2, HyperedgeIndexVector { 0, 2, /*sentinel*/ 8 },
                                  HyperedgeVector { 0, 1, 0, 2, 3, 4, 5, 6 }));
  std::vector<HyperedgeID> removed_hyperedges { 1 };
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);

  for (auto removed_edge : removed_hyperedges) {
    partitioner.partitionUnpartitionedPins(removed_edge, *hypergraph);
  }

  ASSERT_THAT(hypergraph->partID(2), Eq(0));
  ASSERT_THAT(hypergraph->partID(3), Eq(1));
  ASSERT_THAT(hypergraph->partID(4), Eq(0));
  ASSERT_THAT(hypergraph->partID(5), Eq(1));
  ASSERT_THAT(hypergraph->partID(6), Eq(0));
}

TEST_F(APartitioner, CanUseVcyclesAsGlobalSearchStrategy) {
  //simulate the first vcycle by explicitly setting a partitioning
  config.partition.global_search_iterations = 2;
  DBG(true, metrics::hyperedgeCut(*hypergraph));
  partitioner.partition(*hypergraph, *coarsener, *refiner);
  hypergraph->printGraphState();
  DBG(true, metrics::hyperedgeCut(*hypergraph));
  metrics::hyperedgeCut(*hypergraph);
}
} // namespace partition
