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
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;

namespace partition {
using FirstWinsRater = Rater<defs::RatingType, FirstRatingWins>;
using FirstWinsCoarsener = HeuristicHeavyEdgeCoarsener<FirstWinsRater>;
using Refiner = TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;

class APartitioner : public Test {
 public:
  APartitioner(Hypergraph* graph =
                 new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                                HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })) :
    hypergraph(graph),
    config(),
    partitioner(),
    coarsener(new FirstWinsCoarsener(*hypergraph, config,  /* heaviest_node_weight */ 1)),
    refiner(new Refiner(*hypergraph, config)) {
    config.coarsening.contraction_limit = 2;
    config.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    config.coarsening.max_allowed_node_weight = 5;
    config.initial_partitioning.tool = InitialPartitioner::hMetis;
    config.initial_partitioning.tool_path = "/software/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1";
    config.partition.graph_filename = "PartitionerTest.hgr";
    config.partition.graph_partition_filename = "PartitionerTest.hgr.part.2.KaHyPar";
    config.partition.coarse_graph_filename = "PartitionerTest_coarse.hgr";
    config.partition.coarse_graph_partition_filename = "PartitionerTest_coarse.hgr.part.2";
    config.partition.epsilon = 0.15;
    config.partition.k = 2;
    config.partition.rb_lower_k = 0;
    config.partition.rb_upper_k = config.partition.k - 1;
    config.partition.total_graph_weight = 7;
    config.partition.perfect_balance_part_weights[0] = ceil(
      7 / static_cast<double>(config.partition.k));
    config.partition.perfect_balance_part_weights[1] = ceil(
      7 / static_cast<double>(config.partition.k));

    config.partition.max_part_weights[0] = (1 + config.partition.epsilon)
                                           * config.partition.perfect_balance_part_weights[0];
    config.partition.max_part_weights[1] = (1 + config.partition.epsilon)
                                           * config.partition.perfect_balance_part_weights[1];
    double exp = 1.0 / log2(config.partition.k);
    config.initial_partitioning.hmetis_ub_factor =
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
  partitioner.partition(*hypergraph, *coarsener, *refiner, config, 0, (config.partition.k - 1));
  ASSERT_THAT(hypergraph->partID(1), Eq(0));
  ASSERT_THAT(hypergraph->partID(3), Eq(1));
}

TEST_F(APartitioner, UncoarsensTheInitiallyPartitionedHypergraph) {
  partitioner.partition(*hypergraph, *coarsener, *refiner, config,
                        0, (config.partition.k - 1));
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
  partitioner.partition(*hypergraph, *coarsener, *refiner, config,
                        0, (config.partition.k - 1));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 0), Eq(2));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 1), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 1), Eq(3));
}

TEST_F(APartitioner, CanUseVcyclesAsGlobalSearchStrategy) {
  // simulate the first vcycle by explicitly setting a partitioning
  config.partition.global_search_iterations = 2;
  DBG(true, metrics::hyperedgeCut(*hypergraph));
  partitioner.partition(*hypergraph, *coarsener, *refiner, config,
                        0, (config.partition.k - 1));
  hypergraph->printGraphState();
  DBG(true, metrics::hyperedgeCut(*hypergraph));
  metrics::hyperedgeCut(*hypergraph);
}
}  // namespace partition
