/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/coarsening/parallel_ml_community_coarsener.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "tests/partition/coarsening/vertex_pair_coarsener_test_fixtures.h"

using ::testing::AllOf;
using ::testing::AnyOf;

namespace kahypar {
using CoarsenerType = ParallelMLCommunityCoarsener<HeavyEdgeScore,
                                                   MultiplicativePenalty,
                                                   UseCommunityStructure,
                                                   NormalPartitionPolicy,
                                                   BestRatingWithTieBreaking<FirstRatingWins>,
                                                   AllowFreeOnFixedFreeOnFreeFixedOnFixed,
                                                   RatingType>;

class AParallelCoarsener : public ::testing::TestWithParam<size_t> {
 public:
  explicit AParallelCoarsener(Hypergraph* graph =
                                new Hypergraph(12, 9,
                                  HyperedgeIndexVector { 0, 2, 5, 8, 10, 13, 16, 18, 21, /*sentinel*/ 24 },
                                  HyperedgeVector { 0, 1, 0, 1, 3, 1, 2, 3, 4, 5, 4, 5, 7, 5, 6, 7,
                                                    8, 9, 8, 9, 11, 9, 10, 11 })) :
    hypergraph(graph),
    context(),
    coarsener(*hypergraph, context,  /* heaviest_node_weight */ 1),
    refiner(new DoNothingRefiner()) {
    refiner->initialize(999999);
    context.partition.k = 2;
    context.partition.objective = Objective::cut;
    context.partition.epsilon = 0.3;
    context.partition.perfect_balance_part_weights.push_back(6);
    context.partition.perfect_balance_part_weights.push_back(6);
    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[0]);
    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[1]);
    kahypar::Randomize::instance().setSeed(context.partition.seed);
    context.coarsening.max_allowed_node_weight = 5;
    context.coarsening.contraction_limit = 6;
    context.shared_memory.num_threads = GetParam();
    context.shared_memory.pool = std::make_unique<kahypar::parallel::ThreadPool>(GetParam());

    hypergraph->setNumCommunities(3);
    hypergraph->setNodeCommunity(0,  0);
    hypergraph->setNodeCommunity(1,  0);
    hypergraph->setNodeCommunity(2,  0);
    hypergraph->setNodeCommunity(3,  0);
    hypergraph->setNodeCommunity(4,  1);
    hypergraph->setNodeCommunity(5,  1);
    hypergraph->setNodeCommunity(6,  1);
    hypergraph->setNodeCommunity(7,  1);
    hypergraph->setNodeCommunity(8,  2);
    hypergraph->setNodeCommunity(9,  2);
    hypergraph->setNodeCommunity(10, 2);
    hypergraph->setNodeCommunity(11, 2);

    coarsener.disableRandomization();
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Context context;
  CoarsenerType coarsener;
  std::unique_ptr<IRefiner> refiner;
};

INSTANTIATE_TEST_CASE_P(NumThreads,
                        AParallelCoarsener,
                        ::testing::Values(1, 2, 3));

TEST_P(AParallelCoarsener, RemovesHyperedgesOfSizeOneDuringCoarsening) {
  removesHyperedgesOfSizeOneDuringCoarsening(coarsener, hypergraph, 6, {0, 3, 6});
}

TEST_P(AParallelCoarsener, DecreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne) {
  decreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne(coarsener, hypergraph, 9, 15);
}

TEST_P(AParallelCoarsener, ReAddsHyperedgesOfSizeOneDuringUncoarsening) {
  reAddsHyperedgesOfSizeOneDuringUncoarsening(coarsener, hypergraph, refiner, 6,
                                              { 0, 3, 6 }, { 0, 2, 4 }, { 6, 8, 10 });
}

TEST_P(AParallelCoarsener, RemovesParallelHyperedgesDuringCoarsening) {
  removesParallelHyperedgesDuringCoarsening(coarsener, hypergraph, 6, {2, 5, 8});
}

TEST_P(AParallelCoarsener, UpdatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval) {
  updatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval(coarsener, hypergraph, 6,
                                                                       { {1, 2}, {4, 2}, {7, 2} });
}

TEST_P(AParallelCoarsener, DecreasesNumberOfHyperedgesOnParallelHyperedgeRemoval) {
  decreasesNumberOfHyperedgesOnParallelHyperedgeRemoval(coarsener, hypergraph, 6, 3);
}

TEST_P(AParallelCoarsener, DecreasesNumberOfPinsOnParallelHyperedgeRemoval) {
  decreasesNumberOfPinsOnParallelHyperedgeRemoval(coarsener, hypergraph, 6, 6);
}

TEST_P(AParallelCoarsener, RestoresParallelHyperedgesDuringUncoarsening) {
  restoresParallelHyperedgesDuringUncoarsening(coarsener, hypergraph, refiner, 6,
                                               {2, 5, 8}, { 0, 2, 4 }, { 6, 8, 10 });
}

TEST_P(AParallelCoarsener, DoesNotCoarsenUntilCoarseningLimit) {
  doesNotCoarsenUntilCoarseningLimit(coarsener, hypergraph, context, 6, 6);
}
}  // namespace kahypar
