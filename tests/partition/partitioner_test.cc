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
#include "kahypar/kahypar.h"
#include "kahypar/macros.h"
#include "kahypar/partition/coarsening/full_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/configuration.h"
#include "kahypar/partition/partitioner.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

using::testing::Test;
using::testing::Eq;

namespace kahypar {
using FirstWinsRater = HeavyEdgeRater<RatingType, FirstRatingWins>;
using FirstWinsCoarsener = FullVertexPairCoarsener<FirstWinsRater>;
using Refiner = TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;

class APartitioner : public Test {
 public:
  APartitioner() :
    hypergraph(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    config(),
    partitioner(),
    coarsener(new FirstWinsCoarsener(*hypergraph, config,  /* heaviest_node_weight */ 1)),
    refiner(new Refiner(*hypergraph, config)) {
    config.coarsening.contraction_limit = 2;
    config.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    config.coarsening.max_allowed_node_weight = 5;
    config.initial_partitioning.tool = InitialPartitioner::KaHyPar;
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
    kahypar::Randomize::instance().setSeed(config.partition.seed);
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

TEST_F(APartitioner, UsesKaHyParPartitioningOnCoarsestHypergraph) {
  partitioner.partition(*hypergraph, *coarsener, *refiner, config, 0, (config.partition.k - 1));
  ASSERT_THAT(hypergraph->partID(1), Eq(1));
  ASSERT_THAT(hypergraph->partID(3), Eq(0));
}

TEST_F(APartitioner, UncoarsensTheInitiallyPartitionedHypergraph) {
  partitioner.partition(*hypergraph, *coarsener, *refiner, config,
                        0, (config.partition.k - 1));
  hypergraph->printGraphState();
  ASSERT_THAT(hypergraph->partID(0), Eq(1));
  ASSERT_THAT(hypergraph->partID(1), Eq(1));
  ASSERT_THAT(hypergraph->partID(2), Eq(1));
  ASSERT_THAT(hypergraph->partID(3), Eq(0));
  ASSERT_THAT(hypergraph->partID(4), Eq(0));
  ASSERT_THAT(hypergraph->partID(5), Eq(1));
  ASSERT_THAT(hypergraph->partID(6), Eq(0));
}

TEST_F(APartitioner, CalculatesPinCountsOfAHyperedgesAfterInitialPartitioning) {
  ASSERT_THAT(hypergraph->pinCountInPart(0, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 1), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 1), Eq(0));
  partitioner.partition(*hypergraph, *coarsener, *refiner, config,
                        0, (config.partition.k - 1));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 1), Eq(2));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 0), Eq(3));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 1), Eq(0));
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
}  // namespace kahypar
