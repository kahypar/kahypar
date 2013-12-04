#include "gmock/gmock.h"

#include "Coarsener.h"
#include "../lib/datastructure/Hypergraph.h"

using ::testing::Test;
using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::AnyOf;

using defs::hMetisHyperEdgeIndexVector;
using defs::hMetisHyperEdgeVector;

namespace partition {

typedef datastructure::HypergraphType HypergraphType;
typedef Coarsener<HypergraphType, defs::RatingType, FirstRatingWins> CoarsenerType;
typedef Coarsener<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsCoarsener;
typedef Coarsener<HypergraphType, defs::RatingType, LastRatingWins> LastWinsCoarsener;
typedef Coarsener<HypergraphType, defs::RatingType, RandomRatingWins> RandomWinsCoarsener;
typedef HypergraphType::HypernodeID HypernodeID;

class ACoarsener : public Test {
 public:
  ACoarsener() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}, nullptr, nullptr),
      coarsening_limit(2),
      threshold_node_weight(5) {}
  
  HypergraphType hypergraph;
  int coarsening_limit;
  int threshold_node_weight;
};

TEST_F(ACoarsener, TakesAHypergraphAContractionLimitAndAThresholdForNodeWeightAsInput) {
  CoarsenerType coarsener(hypergraph, coarsening_limit, threshold_node_weight);
}

TEST_F(ACoarsener, UsesACoarseningHistoryToRememberAndUndoContractions) {
  RandomWinsCoarsener coarsener (hypergraph, coarsening_limit, threshold_node_weight);
  coarsener.coarsen();
  coarsener.uncoarsen();
}

TEST_F(ACoarsener, CalculatesHeavyEdgeRating) {
  CoarsenerType coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  ASSERT_THAT(coarsener.rate(0).value, Eq(1));
  ASSERT_THAT(coarsener.rate(0).target, Eq(2));
  ASSERT_THAT(coarsener.rate(3).value, DoubleEq(5.0/6));
  ASSERT_THAT(coarsener.rate(3).target, Eq(4));
}

TEST_F(ACoarsener, WithFirstWinsPolicyUsesFirstRatingEntryOfEqualRatings) {
  FirstWinsCoarsener coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  ASSERT_THAT(coarsener.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(6).target, Eq(5));
  ASSERT_THAT(coarsener.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(5).target, Eq(6));
}

TEST_F(ACoarsener, WithLastWinsPolicyUsesLastRatingEntryOfEqualRatings) {
  LastWinsCoarsener coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  ASSERT_THAT(coarsener.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(6).target, Eq(3));
  ASSERT_THAT(coarsener.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(5).target, Eq(2));
}

TEST_F(ACoarsener, WithRandomWinsPolicyUsesRandomRatingEntryOfEqualRatings) {
  RandomWinsCoarsener coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  ASSERT_THAT(coarsener.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(6).target, AnyOf(5,2,4,3));
  ASSERT_THAT(coarsener.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(5).target, AnyOf(2,6));
}

TEST_F(ACoarsener, DoesNotRateNodePairsViolatingThresholdNodeWeight) {
  FirstWinsCoarsener coarsener(hypergraph, coarsening_limit, /*threshold*/ 2);
  ASSERT_THAT(coarsener.rate(0).target, Eq(2));
  ASSERT_THAT(coarsener.rate(0).value, Eq(1));
  ASSERT_THAT(coarsener.rate(0).valid, Eq(true));
  
  hypergraph.contract(0,2);
  
  ASSERT_THAT(coarsener.rate(0).target, Eq(std::numeric_limits<HypernodeID>::max()));
  ASSERT_THAT(coarsener.rate(0).value, Eq(std::numeric_limits<defs::RatingType>::min()));
  ASSERT_THAT(coarsener.rate(0).valid, Eq(false));
}

TEST_F(ACoarsener, DISABLED_SelectsNodePairToContractBasedOnHighestRating) {
  FirstWinsCoarsener coarsener(hypergraph, coarsening_limit, /*threshold*/ 5);
  coarsener.coarsen();
  ASSERT_THAT(hypergraph.nodeIsValid(2), Eq(false));
}

} // namespace partition
