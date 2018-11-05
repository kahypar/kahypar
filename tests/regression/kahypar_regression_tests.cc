/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>

#include "gmock/gmock.h"

#include "kahypar/application/command_line_options.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/kahypar.h"
#include "kahypar/partitioner_facade.h"
#include "tests/end_to_end/kahypar_test_fixtures.h"

namespace kahypar {
void test(const std::string& filename, const Hypergraph& hypergraph, const Context& context) {
  std::ifstream previous_commit_results(filename.c_str());
  HyperedgeWeight cut = -1;
  HyperedgeWeight km1 = -1;
  HyperedgeWeight soed = -1;
  double imbalance = -1.0;
  if (previous_commit_results.good()) {
    previous_commit_results >> cut >> km1 >> soed >> imbalance;
    ASSERT_EQ(metrics::hyperedgeCut(hypergraph), cut);
    ASSERT_EQ(metrics::km1(hypergraph), km1);
    ASSERT_EQ(metrics::soed(hypergraph), soed);
    ASSERT_DOUBLE_EQ(metrics::imbalance(hypergraph, context), imbalance);
    std::remove(filename.c_str());
  } else {
    std::ofstream old_commit_results(filename.c_str());
    old_commit_results << metrics::hyperedgeCut(hypergraph) << ' '
                       << metrics::km1(hypergraph) << ' '
                       << metrics::soed(hypergraph) << ' ';
    old_commit_results.precision(std::numeric_limits<double>::max_digits10);
    old_commit_results << std::fixed << metrics::imbalance(hypergraph, context);
  }
}

TEST_F(KaHyParK, ComputesDirectKwayCutPartitioning) {
  parseIniToContext(context, "../../../config/km1_direct_kway_alenex17.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  test("kahypar-k-cut.results", hypergraph, context);
}

TEST_F(KaHyParK, ComputesDirectKwayKm1Partitioning) {
  parseIniToContext(context, "../../../config/km1_direct_kway_alenex17.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  test("kahypar-k-km1.results", hypergraph, context);
}


TEST_F(KaHyParR, ComputesRecursiveBisectionCutPartitioning) {
  parseIniToContext(context, "../../../config/cut_rb_alenex16.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.local_search.algorithm = RefinementAlgorithm::twoway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  test("kahypar-r-cut.results", hypergraph, context);
}

TEST_F(KaHyParR, ComputesRecursiveBisectionKm1Partitioning) {
  parseIniToContext(context, "../../../config/cut_rb_alenex16.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::twoway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  test("kahypar-r-km1.results", hypergraph, context);
}

TEST_F(KaHyParCA, ComputesDirectKwayKm1Partitioning) {
  parseIniToContext(context, "../../../config/km1_direct_kway_sea17.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  Hypergraph verification_hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  test("kahypar-ca-km1.results", hypergraph, context);
}

TEST_F(KaHyParCA, UsesSparsificationAndCommunityDetection) {
  parseIniToContext(context, "../../../config/km1_direct_kway_sea17.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.seed = 10;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;
  context.preprocessing.enable_community_detection = true;
  context.preprocessing.enable_min_hash_sparsifier = true;
  context.coarsening.algorithm = CoarseningAlgorithm::ml_style;
  context.coarsening.rating.heavy_node_penalty_policy = HeavyNodePenaltyPolicy::no_penalty;
  context.coarsening.rating.acceptance_policy = AcceptancePolicy::best_prefer_unmatched;
  context.partition.graph_filename = "test_instances/bundle1.mtx.hgr";

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  Hypergraph verification_hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  test("kahypar-ca-km1-comm-sparse.results", hypergraph, context);
}
}  // namespace kahypar
