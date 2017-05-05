/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "kahypar/kahypar.h"
#include "tests/end_to_end/kahypar_test_fixtures.h"

namespace kahypar {
TEST_F(KaHyParK, ComputesDirectKwayCutPartitioning) {
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  Partitioner partitioner;
  partitioner.partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  Hypergraph verification_hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  for (const HypernodeID& hn : hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, hypergraph.partID(hn));
  }

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), metrics::hyperedgeCut(verification_hypergraph));
  ASSERT_EQ(metrics::soed(hypergraph), metrics::soed(verification_hypergraph));
  ASSERT_EQ(metrics::km1(hypergraph), metrics::km1(verification_hypergraph));
}

TEST_F(KaHyParK, ComputesDirectKwayKm1Partitioning) {
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  Partitioner partitioner;
  partitioner.partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  Hypergraph verification_hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  for (const HypernodeID& hn : hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, hypergraph.partID(hn));
  }

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), metrics::hyperedgeCut(verification_hypergraph));
  ASSERT_EQ(metrics::soed(hypergraph), metrics::soed(verification_hypergraph));
  ASSERT_EQ(metrics::km1(hypergraph), metrics::km1(verification_hypergraph));
}


TEST_F(KaHyParR, ComputesRecursiveBisectionCutPartitioning) {
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.local_search.algorithm = RefinementAlgorithm::twoway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  Partitioner partitioner;
  partitioner.partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  Hypergraph verification_hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  for (const HypernodeID& hn : hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, hypergraph.partID(hn));
  }

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), metrics::hyperedgeCut(verification_hypergraph));
  ASSERT_EQ(metrics::soed(hypergraph), metrics::soed(verification_hypergraph));
  ASSERT_EQ(metrics::km1(hypergraph), metrics::km1(verification_hypergraph));
}

TEST_F(KaHyParR, ComputesRecursiveBisectionKm1Partitioning) {
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::twoway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  Partitioner partitioner;
  partitioner.partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  Hypergraph verification_hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  for (const HypernodeID& hn : hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, hypergraph.partID(hn));
  }

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), metrics::hyperedgeCut(verification_hypergraph));
  ASSERT_EQ(metrics::soed(hypergraph), metrics::soed(verification_hypergraph));
  ASSERT_EQ(metrics::km1(hypergraph), metrics::km1(verification_hypergraph));
}

TEST_F(KaHyParCA, ComputesDirectKwayKm1Partitioning) {
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  Partitioner partitioner;
  partitioner.partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  Hypergraph verification_hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  for (const HypernodeID& hn : hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, hypergraph.partID(hn));
  }

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), metrics::hyperedgeCut(verification_hypergraph));
  ASSERT_EQ(metrics::soed(hypergraph), metrics::soed(verification_hypergraph));
  ASSERT_EQ(metrics::km1(hypergraph), metrics::km1(verification_hypergraph));
}
}  // namespace kahypar
