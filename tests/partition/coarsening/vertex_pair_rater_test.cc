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

#include <memory>

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "kahypar/partition/coarsening/vertex_pair_rater.h"

using ::testing::Test;
using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::AnyOf;

namespace kahypar {
using FirstWinsRater = VertexPairRater<HeavyEdgeScore,
                                       MultiplicativePenalty,
                                       UseCommunityStructure,
                                       NormalPartitionPolicy,
                                       BestRatingWithTieBreaking<FirstRatingWins>,
                                       AllowFreeOnFixedFreeOnFreeFixedOnFixed,
                                       RatingType>;
using LastWinsRater = VertexPairRater<HeavyEdgeScore,
                                      MultiplicativePenalty,
                                      UseCommunityStructure,
                                      NormalPartitionPolicy,
                                      BestRatingWithTieBreaking<LastRatingWins>,
                                      AllowFreeOnFixedFreeOnFreeFixedOnFixed,
                                      RatingType>;
using RandomWinsRater = VertexPairRater<HeavyEdgeScore,
                                        MultiplicativePenalty,
                                        UseCommunityStructure,
                                        NormalPartitionPolicy,
                                        BestRatingWithTieBreaking<RandomRatingWins>,
                                        AllowFreeOnFixedFreeOnFreeFixedOnFixed,
                                        RatingType>;

class ARater : public Test {
 public:
  explicit ARater(Hypergraph* graph = nullptr) :
    hypergraph(graph),
    context() {
    context.coarsening.max_allowed_node_weight = 2;
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Context context;
};

class AFirstWinsRater : public ARater {
 public:
  AFirstWinsRater() :
    ARater(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                          HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    rater(*hypergraph, context) { }

  FirstWinsRater rater;
};

class ALastWinsRater : public ARater {
 public:
  ALastWinsRater() :
    ARater(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                          HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    rater(*hypergraph, context) { }

  LastWinsRater rater;
};

class ARandomWinsRater : public ARater {
 public:
  ARandomWinsRater() :
    ARater(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                          HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    rater(*hypergraph, context) { }

  RandomWinsRater rater;
};

class AHyperedgeRater : public ARater {
 public:
  AHyperedgeRater() :
    ARater(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                          HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })) { }
};

TEST_F(AFirstWinsRater, UsesHeavyEdgeRatingToRateHypernodes) {
  ASSERT_THAT(rater.rate(0).value, Eq(1));
  ASSERT_THAT(rater.rate(0).target, Eq(2));
  ASSERT_THAT(rater.rate(3).value, DoubleEq(5.0 / 6));
  ASSERT_THAT(rater.rate(3).target, Eq(4));
}

TEST_F(AFirstWinsRater, UsesFirstRatingEntryOfEqualRatings) {
  ASSERT_THAT(rater.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(rater.rate(6).target, Eq(5));
  ASSERT_THAT(rater.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(rater.rate(5).target, Eq(6));
}

TEST_F(ALastWinsRater, UsesLastRatingEntryOfEqualRatings) {
  ASSERT_THAT(rater.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(rater.rate(6).target, Eq(3));
  ASSERT_THAT(rater.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(rater.rate(5).target, Eq(2));
}


TEST_F(ARandomWinsRater, UsesRandomRatingEntryOfEqualRatings) {
  ASSERT_THAT(rater.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(rater.rate(6).target, AnyOf(5, 2, 4, 3));
  ASSERT_THAT(rater.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(rater.rate(5).target, AnyOf(2, 6));
}

TEST_F(AFirstWinsRater, DoesNotRateNodePairsViolatingThresholdNodeWeight) {
  ASSERT_THAT(rater.rate(0).target, Eq(2));
  ASSERT_THAT(rater.rate(0).value, Eq(1));
  ASSERT_THAT(rater.rate(0).valid, Eq(true));

  hypergraph->contract(0, 2);
  hypergraph->removeEdge(0);  // since we cannot rate single-node HEs

  ASSERT_THAT(rater.rate(0).target, Eq(std::numeric_limits<HypernodeID>::max()));
  ASSERT_THAT(rater.rate(0).value, Eq(std::numeric_limits<RatingType>::min()));
  ASSERT_THAT(rater.rate(0).valid, Eq(false));
}

TEST_F(ARater, ReturnsInvalidRatingIfTargetNotIsNotInSamePartition) {
  hypergraph.reset(new Hypergraph(2, 1, HyperedgeIndexVector { 0, 2 },
                                  HyperedgeVector { 0, 1 }));

  FirstWinsRater rater(*hypergraph, context);

  ASSERT_THAT(rater.rate(0).target, Eq(1));
  ASSERT_THAT(rater.rate(0).value, Eq(1));
  ASSERT_THAT(rater.rate(0).valid, Eq(true));

  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);

  ASSERT_THAT(rater.rate(0).target, Eq(std::numeric_limits<HypernodeID>::max()));
  ASSERT_THAT(rater.rate(0).value, Eq(std::numeric_limits<RatingType>::min()));
  ASSERT_THAT(rater.rate(0).valid, Eq(false));
}
}  // namespace kahypar
