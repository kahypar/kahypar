/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <gmock/gmock.h>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/Partitioner.h"
#include "partition/coarsening/HeuristicVertexPairCoarsener.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using::testing::Test;
using::testing::Eq;
using::testing::DoubleEq;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeight;

using partition::Rater;
using partition::FirstRatingWins;
using partition::ICoarsener;
using partition::IRefiner;
using partition::HeuristicVertexPairCoarsener;
using partition::Configuration;
using partition::Partitioner;
using partition::TwoWayFMRefiner;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::InitialPartitioner;

namespace metrics {
using FirstWinsRater = Rater<defs::RatingType, FirstRatingWins>;
using FirstWinsCoarsener = HeuristicVertexPairCoarsener<FirstWinsRater>;
using Refiner = TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;

class AnUnPartitionedHypergraph : public Test {
 public:
  AnUnPartitionedHypergraph() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }

  Hypergraph hypergraph;
};

class TheDemoHypergraph : public AnUnPartitionedHypergraph {
 public:
  TheDemoHypergraph() :
    AnUnPartitionedHypergraph() { }
};

class APartitionedHypergraph : public Test {
 public:
  APartitionedHypergraph() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    config(),
    partitioner(),
    coarsener(new FirstWinsCoarsener(hypergraph, config,  /* heaviest_node_weight */ 1)),
    refiner(new Refiner(hypergraph, config)) {
    config.partition.k = 2;
    config.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    config.coarsening.contraction_limit = 2;
    config.partition.total_graph_weight = 7;
    config.coarsening.max_allowed_node_weight = 5;
    config.partition.graph_filename = "Test";
    config.initial_partitioning.tool = InitialPartitioner::hMetis;
    config.initial_partitioning.tool_path = "/software/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1";
    config.partition.graph_partition_filename = "Test.hgr.part.2.KaHyPar";
    config.partition.coarse_graph_filename = "test_coarse.hgr";
    config.partition.coarse_graph_partition_filename = "test_coarse.hgr.part.2";
    config.partition.epsilon = 0.15;
    config.partition.perfect_balance_part_weights[0] = ceil(7.0 / 2);
    config.partition.perfect_balance_part_weights[1] = ceil(7.0 / 2);
    config.partition.max_part_weights[0] = (1 + config.partition.epsilon)
                                           * config.partition.perfect_balance_part_weights[0];
    config.partition.max_part_weights[1] = (1 + config.partition.epsilon)
                                           * config.partition.perfect_balance_part_weights[1];
    double exp = 1.0 / log2(config.partition.k);
    config.initial_partitioning.hmetis_ub_factor =
      50.0 * (2 * pow((1 + config.partition.epsilon), exp)
              * pow(ceil(static_cast<double>(config.partition.total_graph_weight)
                         / config.partition.k) / config.partition.total_graph_weight, exp) - 1);
    partitioner.partition(hypergraph, *coarsener, *refiner, config, 0, (config.partition.k - 1));
  }

  Hypergraph hypergraph;
  Configuration config;
  Partitioner partitioner;
  std::unique_ptr<ICoarsener> coarsener;
  std::unique_ptr<IRefiner> refiner;
};

class TheHyperedgeCutCalculationForInitialPartitioning : public AnUnPartitionedHypergraph {
 public:
  TheHyperedgeCutCalculationForInitialPartitioning() :
    AnUnPartitionedHypergraph(),
    config(),
    coarsener(hypergraph, config,  /* heaviest_node_weight */ 1),
    hg_to_hmetis(),
    partition() {
    config.coarsening.contraction_limit = 2;
    config.coarsening.max_allowed_node_weight = 5;
    config.partition.graph_filename = "cutCalc_test.hgr";
    config.partition.graph_partition_filename = "cutCalc_test.hgr.part.2.KaHyPar";
    config.partition.coarse_graph_filename = "cutCalc_test_coarse.hgr";
    config.partition.coarse_graph_partition_filename = "cutCalc_test_coarse.hgr.part.2";
    config.partition.epsilon = 0.15;
    hg_to_hmetis[1] = 0;
    hg_to_hmetis[3] = 1;
    partition.push_back(1);
    partition.push_back(0);
  }

  Configuration config;
  FirstWinsCoarsener coarsener;
  std::unordered_map<HypernodeID, HypernodeID> hg_to_hmetis;
  std::vector<PartitionID> partition;
};

TEST_F(TheHyperedgeCutCalculationForInitialPartitioning, ReturnsCorrectResult) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.nodeDegree(1), Eq(1));
  ASSERT_THAT(hypergraph.nodeDegree(3), Eq(1));
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(3, 1);

  ASSERT_THAT(hyperedgeCut(hypergraph, hg_to_hmetis, partition), Eq(hyperedgeCut(hypergraph)));
}

TEST_F(AnUnPartitionedHypergraph, HasHyperedgeCutZero) {
  ASSERT_THAT(hyperedgeCut(hypergraph), Eq(0));
}

TEST_F(APartitionedHypergraph, HasCorrectHyperedgeCut) {
  ASSERT_THAT(hyperedgeCut(hypergraph), Eq(2));
}

TEST_F(TheDemoHypergraph, HasAvgHyperedgeDegree3) {
  ASSERT_THAT(avgHyperedgeDegree(hypergraph), DoubleEq(3.0));
}

TEST_F(TheDemoHypergraph, HasAvgHypernodeDegree12Div7) {
  ASSERT_THAT(avgHypernodeDegree(hypergraph), DoubleEq(12.0 / 7));
}
}  // namespace metrics
