/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/coarsening/HeavyEdgeCoarsener_TestFixtures.h"

using defs::Hypergraph;

namespace partition {
using FirstWinsRater = Rater<defs::RatingType, FirstRatingWins>;
using CoarsenerType = FullHeavyEdgeCoarsener<FirstWinsRater>;

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
  ASSERT_THAT(coarsener._history.back().contraction_memento.u, Eq(0));
  ASSERT_THAT(coarsener._history.back().contraction_memento.v, Eq(2));
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

TEST(OurCoarsener, DoesNotObscureNaturalClustersInHypergraphs) {
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  Configuration config;
  config.coarsening.max_allowed_node_weight = 5;
  config.coarsening.max_allowed_node_weight = 3;
  std::string graph_file("../../../../benchmark_instances/special_instances/bad_for_ec.hgr");
  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  io::readHypergraphFile(graph_file, num_hypernodes, num_hyperedges, index_vector, edge_vector);
  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);
  CoarsenerType coarsener(hypergraph, config);
  coarsener.coarsen(5);
  hypergraph.printGraphState();
  ASSERT_THAT(hypergraph.nodeWeight(0), Eq(2));
  ASSERT_THAT(hypergraph.nodeWeight(3), Eq(1));
  ASSERT_THAT(hypergraph.nodeWeight(4), Eq(2));
  ASSERT_THAT(hypergraph.nodeWeight(7), Eq(2));
  ASSERT_THAT(hypergraph.nodeWeight(8), Eq(3));
  ASSERT_THAT(hypergraph.edgeWeight(0), Eq(1));
  ASSERT_THAT(hypergraph.edgeWeight(3), Eq(1));
  ASSERT_THAT(hypergraph.edgeWeight(4), Eq(1));
  ASSERT_THAT(hypergraph.edgeWeight(5), Eq(2));
  ASSERT_THAT(hypergraph.edgeWeight(7), Eq(1));
  ASSERT_THAT(hypergraph.edgeWeight(10), Eq(3));
}
} // namespace partition
