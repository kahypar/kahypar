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
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/coarsening/lazy_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "tests/partition/coarsening/vertex_pair_coarsener_test_fixtures.h"

namespace kahypar {
using CoarsenerType = LazyVertexPairCoarsener<HeavyEdgeScore,
                                              MultiplicativePenalty,
                                              UseCommunityStructure,
                                              NormalPartitionPolicy,
                                              BestRatingWithTieBreaking<FirstRatingWins>,
                                              AllowFreeOnFixedFreeOnFreeFixedOnFixed,
                                              RatingType>;

class ACoarsener : public ACoarsenerBase<CoarsenerType>{
 public:
  explicit ACoarsener() :
    ACoarsenerBase() { }
};

TEST_F(ACoarsener, RemovesHyperedgesOfSizeOneDuringCoarsening) {
  removesHyperedgesOfSizeOneDuringCoarsening(coarsener, hypergraph);
}

TEST_F(ACoarsener, DecreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne) {
  decreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne(coarsener, hypergraph);
}

TEST_F(ACoarsener, ReAddsHyperedgesOfSizeOneDuringUncoarsening) {
  reAddsHyperedgesOfSizeOneDuringUncoarsening(coarsener, hypergraph, refiner);
}

TEST_F(ACoarsener, RemovesParallelHyperedgesDuringCoarsening) {
  removesParallelHyperedgesDuringCoarsening(coarsener, hypergraph);
}

TEST_F(ACoarsener, UpdatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval) {
  updatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval(coarsener, hypergraph);
}
TEST_F(ACoarsener, DecreasesNumberOfHyperedgesOnParallelHyperedgeRemoval) {
  decreasesNumberOfHyperedgesOnParallelHyperedgeRemoval(coarsener, hypergraph);
}

TEST_F(ACoarsener, DecreasesNumberOfPinsOnParallelHyperedgeRemoval) {
  decreasesNumberOfPinsOnParallelHyperedgeRemoval(coarsener, hypergraph);
}

TEST_F(ACoarsener, RestoresParallelHyperedgesDuringUncoarsening) {
  restoresParallelHyperedgesDuringUncoarsening(coarsener, hypergraph, refiner);
}

TEST(AnUncoarseningOperation, RestoresParallelHyperedgesInReverseOrder) {
  restoresParallelHyperedgesInReverseOrder<CoarsenerType>();
}

TEST(AnUncoarseningOperation, RestoresSingleNodeHyperedgesInReverseOrder) {
  restoresSingleNodeHyperedgesInReverseOrder<CoarsenerType>();
}

TEST_F(ACoarsener, DoesNotCoarsenUntilCoarseningLimit) {
  doesNotCoarsenUntilCoarseningLimit(coarsener, hypergraph, context);
}

TEST(ALazyUpdateCoarsener, InvalidatesAdjacentHypernodesInsteadOfReratingThem) {
  Hypergraph hypergraph(5, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 7 },
                        HyperedgeVector { 0, 1, 0, 1, 2, 3, 4 });
  Context context;
  context.coarsening.max_allowed_node_weight = 5;
  CoarsenerType coarsener(hypergraph, context,  /* heaviest_node_weight */ 1);

  coarsener.coarsen(4);
  hypergraph.printGraphState();

  // representative should have new up-to-date rating
  ASSERT_THAT(coarsener._outdated_rating[0], Eq(false));
  ASSERT_THAT(coarsener._outdated_rating[2], Eq(true));
  ASSERT_THAT(coarsener._outdated_rating[3], Eq(true));
  ASSERT_THAT(coarsener._outdated_rating[4], Eq(true));
}


// By adding the cmaxnet parameter for ml-style coarsening, we introduced
// the a bug in the lazy coarsener, which violated it's main invariant
// that for any two possible contraction partners both are in the priority queue.
// Using cmaxnet can lead to a coarsening process that violates
// this invariant in the following way:
// Assume that v0 is a vertex incident two two nets. Let n0 be a net smaller
// than cmaxnet and n1 a net just above the cmaxnet threshold. Further
// assume that n1 contains only one pin v1 (also incident to n0)
// light enough to be contracted with v0. All other pins of n1 are to heavy,
// while all other pins of n0 are light enough to be contracted
// with v0/v1.
// Because of this setup, the only possible contraction is (v0,v1), since
// all other pins of n1 are to heavy and the pins of n0 are ignored during
// rating because |n0| > cmaxnet.
// Now, the LazyCoarsener starts by contracting (v0,v1).
// After contracting (v0,v1), |n0| <= cmaxnet.
// This leads to new possible contractions of v0 with pins of n0.
// However, since these pins have been ignored previously, none of the is in
// the priority queue. Therefore assertion
// ASSERT(_pq.contains(contracted_node), V(contracted_node)); fails!

TEST(ALazyUpdateCoarsener, HandlesHyperedgeSizeRestrictionsCorrectlyDuringCoarsening) {
  Hypergraph hypergraph(12, 2, HyperedgeIndexVector { 0, 10,  /*sentinel*/ 14 },
                        HyperedgeVector { 0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3 });

  hypergraph.setNodeWeight(0, 1);
  hypergraph.setNodeWeight(1, 1);
  hypergraph.setNodeWeight(2, 100);
  hypergraph.setNodeWeight(3, 100);
  hypergraph.setNodeWeight(4, 1);
  hypergraph.setNodeWeight(5, 1);
  hypergraph.setNodeWeight(6, 1);
  hypergraph.setNodeWeight(7, 1);
  hypergraph.setNodeWeight(8, 1);
  hypergraph.setNodeWeight(9, 1);
  hypergraph.setNodeWeight(10, 1);
  hypergraph.setNodeWeight(11, 1);

  Context context;
  context.coarsening.max_allowed_node_weight = 5;
  context.partition.hyperedge_size_threshold = 9;

  CoarsenerType coarsener(hypergraph, context,  /* heaviest_node_weight */ 1);
  coarsener.coarsen(4);
}
}  // namespace kahypar
