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
#include "kahypar/partition/context.h"
#include "kahypar/partition/multilevel.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
using FirstWinsCoarsener = FullVertexPairCoarsener<HeavyEdgeScore,
                                                   MultiplicativePenalty,
                                                   UseCommunityStructure,
                                                   BestRatingWithTieBreaking<FirstRatingWins>,
                                                   RatingType>;
using Refiner = TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;

class MultilevelPartitioning : public Test {
 public:
  static constexpr bool debug = false;

  MultilevelPartitioning() :
    hypergraph(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    context(),
    coarsener(new FirstWinsCoarsener(*hypergraph, context,  /* heaviest_node_weight */ 1)),
    refiner(new Refiner(*hypergraph, context)) {
    context.coarsening.contraction_limit = 2;
    context.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    context.coarsening.max_allowed_node_weight = 5;
    context.partition.graph_filename = "PartitionerTest.hgr";
    context.partition.graph_partition_filename = "PartitionerTest.hgr.part.2.KaHyPar";
    context.partition.epsilon = 0.15;
    context.partition.k = 2;
    context.partition.rb_lower_k = 0;
    context.partition.rb_upper_k = context.partition.k - 1;
    context.partition.total_graph_weight = 7;
    context.partition.perfect_balance_part_weights[0] = ceil(
      7 / static_cast<double>(context.partition.k));
    context.partition.perfect_balance_part_weights[1] = ceil(
      7 / static_cast<double>(context.partition.k));

    context.partition.max_part_weights[0] = (1 + context.partition.epsilon)
                                            * context.partition.perfect_balance_part_weights[0];
    context.partition.max_part_weights[1] = (1 + context.partition.epsilon)
                                            * context.partition.perfect_balance_part_weights[1];
    kahypar::Randomize::instance().setSeed(context.partition.seed);
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Context context;
  std::unique_ptr<ICoarsener> coarsener;
  std::unique_ptr<IRefiner> refiner;
};

TEST_F(MultilevelPartitioning, UsesKaHyParPartitioningOnCoarsestHypergraph) {
  multilevel::partition(*hypergraph, *coarsener, *refiner, context);
  ASSERT_THAT(hypergraph->partID(1), Eq(1));
  ASSERT_THAT(hypergraph->partID(3), Eq(0));
}

TEST_F(MultilevelPartitioning, UncoarsensTheInitiallyPartitionedHypergraph) {
  multilevel::partition(*hypergraph, *coarsener, *refiner, context);
  hypergraph->printGraphState();
  ASSERT_THAT(hypergraph->partID(0), Eq(1));
  ASSERT_THAT(hypergraph->partID(1), Eq(1));
  ASSERT_THAT(hypergraph->partID(2), Eq(1));
  ASSERT_THAT(hypergraph->partID(3), Eq(0));
  ASSERT_THAT(hypergraph->partID(4), Eq(0));
  ASSERT_THAT(hypergraph->partID(5), Eq(1));
  ASSERT_THAT(hypergraph->partID(6), Eq(0));
}

TEST_F(MultilevelPartitioning, CalculatesPinCountsOfAHyperedgesAfterInitialPartitioning) {
  ASSERT_THAT(hypergraph->pinCountInPart(0, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 1), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 1), Eq(0));
  multilevel::partition(*hypergraph, *coarsener, *refiner, context);
  ASSERT_THAT(hypergraph->pinCountInPart(0, 0), Eq(0));
  ASSERT_THAT(hypergraph->pinCountInPart(0, 1), Eq(2));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 0), Eq(3));
  ASSERT_THAT(hypergraph->pinCountInPart(2, 1), Eq(0));
}

TEST_F(MultilevelPartitioning, CanUseVcyclesAsGlobalSearchStrategy) {
  // simulate the first vcycle by explicitly setting a partitioning
  context.partition.global_search_iterations = 2;
  DBG1 << metrics::hyperedgeCut(*hypergraph);
  multilevel::partition(*hypergraph, *coarsener, *refiner, context);
  hypergraph->printGraphState();
  DBG1 << metrics::hyperedgeCut(*hypergraph);
  metrics::hyperedgeCut(*hypergraph);
}
}  // namespace kahypar
