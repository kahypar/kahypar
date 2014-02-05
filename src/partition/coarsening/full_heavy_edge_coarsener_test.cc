/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph.h"
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/refinement/IRefiner.h"

using::testing::AnyOf;
using::testing::DoubleEq;
using::testing::Eq;
using::testing::Le;
using::testing::Test;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeWeightVector;
using datastructure::HypernodeWeightVector;
using datastructure::HyperedgeVector;
using datastructure::HypernodeID;
using datastructure::HypernodeWeight;
using datastructure::HyperedgeWeight;

namespace partition {
typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef FullHeavyEdgeCoarsener<HypergraphType, FirstWinsRater> CoarsenerType;

template <typename Hypergraph>
class DummyRefiner : public IRefiner<Hypergraph>{
  void refine(HypernodeID, HypernodeID, HyperedgeWeight&,
              double, double&) { }
};

class ACoarsener : public Test {
  public:
  ACoarsener() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    config(),
    coarsener(hypergraph, config),
    refiner(new DummyRefiner<HypergraphType>()) {
    config.coarsening.threshold_node_weight = 5;
  }

  HypergraphType hypergraph;
  Configuration<HypergraphType> config;
  CoarsenerType coarsener;
  std::unique_ptr<IRefiner<HypergraphType> > refiner;

  private:
  DISALLOW_COPY_AND_ASSIGN(ACoarsener);
};

class ACoarsenerWithThresholdWeight3 : public Test {
  public:
  ACoarsenerWithThresholdWeight3() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    config(),
    coarsener(hypergraph, config),
    refiner(new DummyRefiner<HypergraphType>()) {
    config.coarsening.threshold_node_weight = 3;
  }

  HypergraphType hypergraph;
  Configuration<HypergraphType> config;
  CoarsenerType coarsener;
  std::unique_ptr<IRefiner<HypergraphType> > refiner;

  private:
  DISALLOW_COPY_AND_ASSIGN(ACoarsenerWithThresholdWeight3);
};

TEST_F(ACoarsener, RemovesHyperedgesOfSizeOneDuringCoarsening) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(false));
}

TEST_F(ACoarsener, DecreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne) {
  coarsener.coarsen(6);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(false));

  ASSERT_THAT(hypergraph.numPins(), Eq(10));
}

TEST_F(ACoarsener, ReAddsHyperedgesOfSizeOneDuringUncoarsening) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(false));
  coarsener.uncoarsen(*refiner);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(true));
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(true));
  ASSERT_THAT(hypergraph.edgeSize(1), Eq(4));
  ASSERT_THAT(hypergraph.edgeSize(3), Eq(3));
}

TEST_F(ACoarsener, SelectsNodePairToContractBasedOnHighestRating) {
  coarsener.coarsen(6);
  ASSERT_THAT(hypergraph.nodeIsEnabled(2), Eq(false));
  ASSERT_THAT(coarsener._history.top().contraction_memento.u, Eq(0));
  ASSERT_THAT(coarsener._history.top().contraction_memento.v, Eq(2));
}

TEST_F(ACoarsener, RemovesParallelHyperedgesDuringCoarsening) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(3), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(1), Eq(true));
}

TEST_F(ACoarsener, UpdatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeWeight(1), Eq(2));
}
TEST_F(ACoarsener, DecreasesNumberOfHyperedgesOnParallelHyperedgeRemoval) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.numEdges(), Eq(1));
}

TEST_F(ACoarsener, DecreasesNumberOfPinsOnParallelHyperedgeRemoval) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.numPins(), Eq(2));
}

TEST_F(ACoarsener, RestoresParallelHyperedgesDuringUncoarsening) {
  coarsener.coarsen(2);
  coarsener.uncoarsen(*refiner);
  ASSERT_THAT(hypergraph.edgeSize(1), Eq(4));
  ASSERT_THAT(hypergraph.edgeSize(3), Eq(3));
  ASSERT_THAT(hypergraph.edgeWeight(1), Eq(1));
  ASSERT_THAT(hypergraph.edgeWeight(3), Eq(1));
}

TEST(AnUncoarseningOperation, RestoresParallelHyperedgesInReverseOrder) {
  // Artificially constructed hypergraph that enforces the successive removal of
  // two successive parallel hyperedges.
  HyperedgeWeightVector edge_weights { 1, 1, 1, 1 };
  HypernodeWeightVector node_weights { 50, 1, 1 };
  HypergraphType hypergraph(3, 4, HyperedgeIndexVector { 0, 2, 4, 6, /*sentinel*/ 8 },
                            HyperedgeVector { 0, 1, 0, 1, 0, 2, 1, 2 }, &edge_weights,
                            &node_weights);

  Configuration<HypergraphType> config;
  config.coarsening.threshold_node_weight = 4;
  CoarsenerType coarsener(hypergraph, config);
  std::unique_ptr<IRefiner<HypergraphType> > refiner(new DummyRefiner<HypergraphType>());

  coarsener.coarsen(2);
  // The following assertion is thrown if parallel hyperedges are restored in the order in which
  // they were removed: Assertion `_incidence_array[hypernode(pin).firstInvalidEntry() - 1] == e`
  // failed: Incorrect restore of HE 1. In order to correctly restore the hypergraph during un-
  // coarsening, we have to restore the parallel hyperedges in reverse order!
  coarsener.uncoarsen(*refiner);
}

TEST(AnUncoarseningOperation, RestoresSingleNodeHyperedgesInReverseOrder) {
  // Artificially constructed hypergraph that enforces the successive removal of
  // three single-node hyperedges.
  HyperedgeWeightVector edge_weights { 1, 1, 1 };
  HypernodeWeightVector node_weights { 1, 1 };
  HypergraphType hypergraph(2, 3, HyperedgeIndexVector { 0, 2, 4, /*sentinel*/ 6 },
                            HyperedgeVector { 0, 1, 0, 1, 0, 1 }, &edge_weights,
                            &node_weights);

  Configuration<HypergraphType> config;
  config.coarsening.threshold_node_weight = 4;
  CoarsenerType coarsener(hypergraph, config);
  std::unique_ptr<IRefiner<HypergraphType> > refiner(new DummyRefiner<HypergraphType>());

  coarsener.coarsen(1);
  // The following assertion is thrown if parallel hyperedges are restored in the order in which
  // they were removed: Assertion `_incidence_array[hypernode(pin).firstInvalidEntry() - 1] == e`
  // failed: Incorrect restore of HE 0. In order to correctly restore the hypergraph during un-
  // coarsening, we have to restore the single-node hyperedges in reverse order!
  coarsener.uncoarsen(*refiner);
}

TEST_F(ACoarsenerWithThresholdWeight3, DoesNotCoarsenUntilCoarseningLimit) {
  coarsener.coarsen(2);
  forall_hypernodes(hn, hypergraph) {
    ASSERT_THAT(hypergraph.nodeWeight(*hn), Le(3));
  } endfor
    ASSERT_THAT(hypergraph.numNodes(), Eq(3));
}
} // namespace partition
