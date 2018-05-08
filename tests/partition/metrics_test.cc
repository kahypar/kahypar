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
#include "kahypar/partition/coarsening/full_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/coarsening/vertex_pair_rater.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/multilevel.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

using ::testing::Test;
using ::testing::Eq;
using ::testing::DoubleEq;

namespace kahypar {
namespace metrics {
using FirstWinsCoarsener = FullVertexPairCoarsener<HeavyEdgeScore,
                                                   MultiplicativePenalty,
                                                   UseCommunityStructure,
                                                   NormalPartitionPolicy,
                                                   BestRatingWithTieBreaking<FirstRatingWins>,
                                                   AllowFreeOnFixedFreeOnFreeFixedOnFixed,
                                                   RatingType>;
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
    context(),
    coarsener(nullptr),
    refiner(nullptr) {
    context.partition.k = 2;
    context.partition.objective = Objective::cut;
    context.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    context.coarsening.contraction_limit = 2;
    context.coarsening.max_allowed_node_weight = 5;
    context.partition.graph_filename = "Test";
    context.partition.graph_partition_filename = "Test.hgr.part.2.KaHyPar";
    context.partition.epsilon = 0.15;
    context.partition.perfect_balance_part_weights.push_back(ceil(7.0 / 2));
    context.partition.perfect_balance_part_weights.push_back(ceil(7.0 / 2));
    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[0]);
    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[1]);
    context.coarsening.max_allowed_node_weight = 5;
    context.initial_partitioning.mode = Mode::recursive_bisection;
    context.initial_partitioning.technique = InitialPartitioningTechnique::flat;
    context.initial_partitioning.algo = InitialPartitionerAlgorithm::pool;
    context.initial_partitioning.nruns = 20;
    context.initial_partitioning.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    context.initial_partitioning.local_search.fm.max_number_of_fruitless_moves = 50;
    context.initial_partitioning.local_search.fm.stopping_rule = RefinementStoppingRule::simple;
    coarsener.reset(new FirstWinsCoarsener(hypergraph, context,  /* heaviest_node_weight */ 1));
    refiner.reset(new Refiner(hypergraph, context));
    multilevel::partition(hypergraph, *coarsener, *refiner, context);
  }

  Hypergraph hypergraph;
  Context context;
  Partitioner partitioner;
  std::unique_ptr<ICoarsener> coarsener;
  std::unique_ptr<IRefiner> refiner;
};


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
}  // namespace kahypar
