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

typedef hgr::HypergraphType HypergraphType;
typedef Coarsener<defs::RatingType, FirstRatingWins> CoarsenerType;
typedef Coarsener<defs::RatingType, FirstRatingWins> FirstWinsCoarsenerType;
typedef Coarsener<defs::RatingType, LastRatingWins> LastWinsCoarsenerType;
typedef Coarsener<defs::RatingType, RandomRatingWins> RandomWinsCoarsenerType;

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
  RandomWinsCoarsenerType coarsener (hypergraph, coarsening_limit, threshold_node_weight);
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
  FirstWinsCoarsenerType coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  ASSERT_THAT(coarsener.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(6).target, Eq(5));
  ASSERT_THAT(coarsener.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(5).target, Eq(6));
}

TEST_F(ACoarsener, WithLastWinsPolicyUsesLastRatingEntryOfEqualRatings) {
  LastWinsCoarsenerType coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  ASSERT_THAT(coarsener.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(6).target, Eq(3));
  ASSERT_THAT(coarsener.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(5).target, Eq(2));
}

TEST_F(ACoarsener, WithRandomWinsPolicyUsesRandomRatingEntryOfEqualRatings) {
  RandomWinsCoarsenerType coarsener(hypergraph, coarsening_limit, threshold_node_weight);
  ASSERT_THAT(coarsener.rate(6).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(6).target, AnyOf(5,2,4,3));
  ASSERT_THAT(coarsener.rate(5).value, DoubleEq(0.5));
  ASSERT_THAT(coarsener.rate(5).target, AnyOf(2,6));
}

} // namespace partition
