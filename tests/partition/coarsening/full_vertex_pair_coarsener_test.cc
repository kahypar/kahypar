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
#include "kahypar/partition/coarsening/full_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "tests/partition/coarsening/vertex_pair_coarsener_test_fixtures.h"

using ::testing::AllOf;
using ::testing::AnyOf;

namespace kahypar {
using CoarsenerType = FullVertexPairCoarsener<HeavyEdgeScore,
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

// accesses private coarsener internals and therefore cannot be extracted easily
TEST_F(ACoarsener, SelectsNodePairToContractBasedOnHighestRating) {
  coarsener.coarsen(6);
  if (hypergraph->nodeIsEnabled(2)) {
    ASSERT_THAT(coarsener._history.back().contraction_memento.u, Eq(2));
    ASSERT_THAT(coarsener._history.back().contraction_memento.v, Eq(0));
  } else {
    ASSERT_THAT(hypergraph->nodeIsEnabled(0), Eq(true));
    ASSERT_THAT(coarsener._history.back().contraction_memento.u, Eq(0));
    ASSERT_THAT(coarsener._history.back().contraction_memento.v, Eq(2));
  }
}

TEST_F(ACoarsener, ReEvaluatesHypernodesWithNoIncidentEdges) {
  Hypergraph hypergraph(3, 1, HyperedgeIndexVector { 0,  /*sentinel*/ 2 },
                        HyperedgeVector { 0, 1 });

  Context context;
  context.coarsening.max_allowed_node_weight = 4;
  CoarsenerType coarsener(hypergraph, context,  /* heaviest_node_weight */ 1);

  coarsener.coarsen(1);

  ASSERT_THAT(true,
              AnyOf(
                AllOf(
                  hypergraph.nodeIsEnabled(0) == true,
                  hypergraph.nodeIsEnabled(1) == false),
                AllOf(
                  hypergraph.nodeIsEnabled(0) == false,
                  hypergraph.nodeIsEnabled(1) == true)));
  ASSERT_THAT(hypergraph.nodeIsEnabled(2), Eq(true));
}

TEST(OurCoarsener, DoesNotObscureNaturalClustersInHypergraphs) {
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  Context context;
  context.coarsening.max_allowed_node_weight = 3;
  kahypar::Randomize::instance().setSeed(context.partition.seed);
  std::string graph_file("../../../../special_instances/bad_for_ec.hgr");
  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  io::readHypergraphFile(graph_file, num_hypernodes, num_hyperedges, index_vector, edge_vector);
  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);
  CoarsenerType coarsener(hypergraph, context, 1);
  coarsener.coarsen(5);
  hypergraph.printGraphState();
  // nodes 5 and 6 correspond to nodes 'E' and 'F' in
  // http://downloads.hindawi.com/journals/vlsi/2000/019436.pdf
  // page 290. These two nodes should not be contracted.
  ASSERT_TRUE(hypergraph.nodeIsEnabled(5));
  ASSERT_THAT(true,
              AnyOf(hypergraph.nodeIsEnabled(6),
                    hypergraph.nodeIsEnabled(7),
                    hypergraph.nodeIsEnabled(8)));
}
}  // namespace kahypar
