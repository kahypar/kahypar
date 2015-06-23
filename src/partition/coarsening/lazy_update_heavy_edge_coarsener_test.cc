/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "partition/coarsening/HeavyEdgeCoarsener_TestFixtures.h"
#include "partition/coarsening/LazyUpdateHeavyEdgeCoarsener.h"


using defs::Hypergraph;

namespace partition {
using FirstWinsRater = Rater<defs::RatingType, FirstRatingWins>;
using CoarsenerType = LazyUpdateHeavyEdgeCoarsener<FirstWinsRater>;

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
  doesNotCoarsenUntilCoarseningLimit(coarsener, hypergraph, config);
}

TEST(ALazyUpdateCoarsener, InvalidatesAdjacentHypernodesInsteadOfReratingThem) {
  Hypergraph hypergraph(5, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 7 },
                        HyperedgeVector { 0, 1, 0, 1, 2, 3, 4 });
  Configuration config;
  config.coarsening.max_allowed_node_weight = 5;
  CoarsenerType coarsener(hypergraph, config,  /* heaviest_node_weight */ 1);

  coarsener.coarsen(4);
  hypergraph.printGraphState();

  // representative should have new up-to-date rating
  ASSERT_THAT(coarsener._outdated_rating[0], Eq(false));
  ASSERT_THAT(coarsener._outdated_rating[2], Eq(true));
  ASSERT_THAT(coarsener._outdated_rating[3], Eq(true));
  ASSERT_THAT(coarsener._outdated_rating[4], Eq(true));
}
}  // namespace partition
