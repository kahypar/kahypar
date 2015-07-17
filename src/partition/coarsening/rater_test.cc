/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <memory>

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "partition/coarsening/HyperedgeRatingPolicies.h"
#include "partition/coarsening/Rater.h"

using::testing::Test;
using::testing::Eq;
using::testing::DoubleEq;
using::testing::AnyOf;

using defs::Hypergraph;
using defs::HypernodeWeight;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HypernodeID;
using defs::PartitionID;

namespace partition {
using FirstWinsRater = Rater<defs::RatingType, FirstRatingWins>;
using LastWinsRater = Rater<defs::RatingType, LastRatingWins>;
using RandomWinsRater = Rater<defs::RatingType, RandomRatingWins>;

class ARater : public Test {
 public:
  explicit ARater(Hypergraph* graph = nullptr) :
    hypergraph(graph),
    config() {
    config.coarsening.max_allowed_node_weight = 2;
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Configuration config;
};

class AFirstWinsRater : public ARater {
 public:
  AFirstWinsRater() :
    ARater(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                          HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    rater(*hypergraph, config) { }

  FirstWinsRater rater;
};

class ALastWinsRater : public ARater {
 public:
  ALastWinsRater() :
    ARater(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                          HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    rater(*hypergraph, config) { }

  LastWinsRater rater;
};

class ARandomWinsRater : public ARater {
 public:
  ARandomWinsRater() :
    ARater(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                          HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    rater(*hypergraph, config) { }

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
  hypergraph->removeEdge(0, false); // since we cannot rate single-node HEs

  ASSERT_THAT(rater.rate(0).target, Eq(std::numeric_limits<HypernodeID>::max()));
  ASSERT_THAT(rater.rate(0).value, Eq(std::numeric_limits<defs::RatingType>::min()));
  ASSERT_THAT(rater.rate(0).valid, Eq(false));
}

TEST_F(ARater, ReturnsInvalidRatingIfTargetNotIsNotInSamePartition) {
  hypergraph.reset(new Hypergraph(2, 1, HyperedgeIndexVector { 0, 2 },
                                  HyperedgeVector { 0, 1 }));

  FirstWinsRater rater(*hypergraph, config);

  ASSERT_THAT(rater.rate(0).target, Eq(1));
  ASSERT_THAT(rater.rate(0).value, Eq(1));
  ASSERT_THAT(rater.rate(0).valid, Eq(true));

  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 1);

  ASSERT_THAT(rater.rate(0).target, Eq(std::numeric_limits<HypernodeID>::max()));
  ASSERT_THAT(rater.rate(0).value, Eq(std::numeric_limits<defs::RatingType>::min()));
  ASSERT_THAT(rater.rate(0).valid, Eq(false));
}

TEST_F(AHyperedgeRater, ReturnsCorrectHyperedgeRatings) {
  ASSERT_THAT(EdgeWeightDivMultPinWeight::rate(0, *hypergraph,
                                               config.coarsening.max_allowed_node_weight).value,
              DoubleEq(1.0));

  config.coarsening.max_allowed_node_weight = 10;
  hypergraph->setNodeWeight(1, 2);
  hypergraph->setNodeWeight(3, 3);
  hypergraph->setNodeWeight(4, 4);

  ASSERT_THAT(EdgeWeightDivMultPinWeight::rate(1, *hypergraph,
                                               config.coarsening.max_allowed_node_weight).value,
              DoubleEq(1.0 / (2 * 3 * 4)));
}

TEST_F(AHyperedgeRater, ReturnsInvalidRatingIfContractionWouldViolateThreshold) {
  config.coarsening.max_allowed_node_weight = 3;

  ASSERT_THAT(EdgeWeightDivMultPinWeight::rate(1, *hypergraph,
                                               config.coarsening.max_allowed_node_weight).valid,
              Eq(false));
}

TEST_F(AHyperedgeRater, ReturnsInvalidRatingIfHyperedgeIsCutHyperedge) {
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(2, 1);
  ASSERT_THAT(EdgeWeightDivMultPinWeight::rate(0, *hypergraph,
                                               config.coarsening.max_allowed_node_weight).valid,
              Eq(false));
}
}  // namespace partition
