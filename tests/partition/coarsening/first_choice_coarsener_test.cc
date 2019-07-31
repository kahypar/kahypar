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
#include "kahypar/partition/coarsening/first_choice_coarsener.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/level_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "tests/partition/coarsening/vertex_pair_coarsener_test_fixtures.h"

using ::testing::AllOf;
using ::testing::AnyOf;

namespace kahypar {
using CoarsenerType = FirstChoiceCoarsener<HeavyEdgeScore,
                                           MultiplicativePenalty,
                                           UseCommunityStructure,
                                           NormalPartitionPolicy,
                                           BestRatingWithTieBreaking<FirstRatingWins>,
                                           AllowFreeOnFixedFreeOnFreeFixedOnFixed,
                                           nLevel,
                                           RatingType>;

class AFirstChoiceCoarsener : public ACoarsenerBase<CoarsenerType>{
 public:
  explicit AFirstChoiceCoarsener() :
    ACoarsenerBase() { 
    coarsener.deactivateRandomization();
  }

  using Base = ACoarsenerBase<CoarsenerType>;
  using Base::coarsener;
};

TEST_F(AFirstChoiceCoarsener, ComputesCorrectContractionTargets) {
  // This test does not test any functionality specific to the first
  // choice coarsener, but is an indicator how contraction are performed
  // during coarsening
  std::vector<HypernodeID> current_hns;
  coarsener.computeContractionTargetForAllHypernodes(current_hns);
  EXPECT_EQ(coarsener._target[0], 2);
  EXPECT_EQ(coarsener._target[1], 4);
  EXPECT_EQ(coarsener._target[2], 0);
  EXPECT_EQ(coarsener._target[3], 4);
  EXPECT_EQ(coarsener._target[4], 3);
  EXPECT_EQ(coarsener._target[5], 6);
  EXPECT_EQ(coarsener._target[6], 5);
}

TEST_F(AFirstChoiceCoarsener, ComputesContractionForAHypernode) {
  std::vector<HypernodeID> current_hns;
  coarsener.computeContractionTargetForAllHypernodes(current_hns);

  CoarsenerType::Contraction contraction = coarsener.computeContractionForHypernode(0);
  EXPECT_THAT(contraction.representive_node, Eq(0));
  EXPECT_THAT(contraction.contraction_partner, Eq(2));
}

TEST_F(AFirstChoiceCoarsener, ComputesContractionForSeveralHypernodes) {
  std::vector<HypernodeID> current_hns;
  coarsener.computeContractionTargetForAllHypernodes(current_hns);

  coarsener.computeContractionForHypernode(3);
  CoarsenerType::Contraction contraction = coarsener.computeContractionForHypernode(1);
  EXPECT_THAT(contraction.representive_node, Eq(3));
  EXPECT_THAT(contraction.contraction_partner, Eq(1));
}

TEST_F(AFirstChoiceCoarsener, RemovesHyperedgesOfSizeOneDuringCoarsening) {
  removesHyperedgesOfSizeOneDuringCoarsening(coarsener, hypergraph, {0, 3});
}

TEST_F(AFirstChoiceCoarsener, DecreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne) {
  decreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne(coarsener, hypergraph);
}

TEST_F(AFirstChoiceCoarsener, ReAddsHyperedgesOfSizeOneDuringUncoarsening) {
  reAddsHyperedgesOfSizeOneDuringUncoarsening(coarsener, hypergraph, refiner, {0, 3}, {0}, {1});
}

TEST_F(AFirstChoiceCoarsener, RemovesParallelHyperedgesDuringCoarsening) {
  removesParallelHyperedgesDuringCoarsening(coarsener, hypergraph, {1});
}

TEST_F(AFirstChoiceCoarsener, UpdatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval) {
  updatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval(coarsener, hypergraph, {{2, 2}});
}

TEST_F(AFirstChoiceCoarsener, DecreasesNumberOfHyperedgesOnParallelHyperedgeRemoval) {
  decreasesNumberOfHyperedgesOnParallelHyperedgeRemoval(coarsener, hypergraph);
}

TEST_F(AFirstChoiceCoarsener, DecreasesNumberOfPinsOnParallelHyperedgeRemoval) {
  decreasesNumberOfPinsOnParallelHyperedgeRemoval(coarsener, hypergraph);
}

TEST_F(AFirstChoiceCoarsener, RestoresParallelHyperedgesDuringUncoarsening) {
  restoresParallelHyperedgesDuringUncoarsening(coarsener, hypergraph, refiner, {1}, {0}, {1});
}

TEST(AnUncoarseningOperation, RestoresParallelHyperedgesInReverseOrder) {
  restoresParallelHyperedgesInReverseOrder<CoarsenerType>();
}

TEST(AnUncoarseningOperation, RestoresSingleNodeHyperedgesInReverseOrder) {
  restoresSingleNodeHyperedgesInReverseOrder<CoarsenerType>();
}

TEST_F(AFirstChoiceCoarsener, DoesNotCoarsenUntilCoarseningLimit) {
  doesNotCoarsenUntilCoarseningLimit(coarsener, hypergraph, context);
}


}  // namespace kahypar
