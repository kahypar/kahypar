/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/coarsening/HeavyEdgeCoarsener_TestFixtures.h"

using defs::Hypergraph;

namespace partition {
typedef Rater<defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef FullHeavyEdgeCoarsener<FirstWinsRater> CoarsenerType;

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

// accesses private coarsener internals and therefore cannot be extracted easily
TEST_F(ACoarsener, SelectsNodePairToContractBasedOnHighestRating) {
  coarsener.coarsen(6);
  ASSERT_THAT(hypergraph->nodeIsEnabled(2), Eq(false));
  ASSERT_THAT(coarsener._history.top().contraction_memento.u, Eq(0));
  ASSERT_THAT(coarsener._history.top().contraction_memento.v, Eq(2));
}

TEST_F(ACoarsener, ReEvaluatesHypernodesWithNoIncidentEdges) {
  Hypergraph hypergraph(3, 1, HyperedgeIndexVector { 0, /*sentinel*/ 2 },
                        HyperedgeVector { 0, 1 });

  Configuration config;
  config.coarsening.max_allowed_node_weight = 4;
  CoarsenerType coarsener(hypergraph, config);

  coarsener.coarsen(1);

  ASSERT_THAT(hypergraph.nodeIsEnabled(0), Eq(true));
  ASSERT_THAT(hypergraph.nodeIsEnabled(1), Eq(false));
  ASSERT_THAT(hypergraph.nodeIsEnabled(2), Eq(true));
}
} // namespace partition
