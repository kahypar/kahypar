/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016-2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/application/command_line_options.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/kahypar.h"
#include "kahypar/partitioner_facade.h"
#include "tests/end_to_end/kahypar_test_fixtures.h"

#include "include/libkahypar.h"

namespace kahypar {
TEST_F(KaHyParK, ComputesDirectKwayCutPartitioning) {
  parseIniToContext(context, "../../../config/old_reference_configs/km1_direct_kway_alenex17.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
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
  parseIniToContext(context, "../../../config/old_reference_configs/km1_direct_kway_alenex17.ini");
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

  for (const HypernodeID& hn : hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, hypergraph.partID(hn));
  }

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), metrics::hyperedgeCut(verification_hypergraph));
  ASSERT_EQ(metrics::soed(hypergraph), metrics::soed(verification_hypergraph));
  ASSERT_EQ(metrics::km1(hypergraph), metrics::km1(verification_hypergraph));
}


TEST_F(KaHyParR, ComputesRecursiveBisectionCutPartitioning) {
  parseIniToContext(context, "../../../config/old_reference_configs/cut_rb_alenex16.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.local_search.algorithm = RefinementAlgorithm::twoway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
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
  parseIniToContext(context, "../../../config/old_reference_configs/cut_rb_alenex16.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::twoway_fm;

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);
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
  parseIniToContext(context, "../../../config/old_reference_configs/km1_direct_kway_sea17.ini");
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

  for (const HypernodeID& hn : hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, hypergraph.partID(hn));
  }

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), metrics::hyperedgeCut(verification_hypergraph));
  ASSERT_EQ(metrics::soed(hypergraph), metrics::soed(verification_hypergraph));
  ASSERT_EQ(metrics::km1(hypergraph), metrics::km1(verification_hypergraph));
}

TEST_F(KaHyParCA, HandlesIndividualBlockWeights) {
  parseIniToContext(context, "../../../config/old_reference_configs/km1_direct_kway_sea17.ini");
  context.partition.k = 6;

  context.partition.epsilon = 0;
  context.partition.mode = Mode::direct_kway;
  context.partition.objective = Objective::km1;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;
  context.partition.use_individual_part_weights = true;
  context.initial_partitioning.enable_early_restart = false;
  context.initial_partitioning.enable_late_restart = false;
  context.partition.verbose_output = true;
  context.partition.max_part_weights = { 2750, 1000, 3675, 2550, 2550, 250 };

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);

  kahypar::io::printPartitioningResults(hypergraph, context, std::chrono::duration<double>(0.0));

  ASSERT_LE(hypergraph.partWeight(0), 2750);
  ASSERT_LE(hypergraph.partWeight(1), 1000);
  ASSERT_LE(hypergraph.partWeight(2), 3675);
  ASSERT_LE(hypergraph.partWeight(3), 2550);
  ASSERT_LE(hypergraph.partWeight(4), 2550);
  ASSERT_LE(hypergraph.partWeight(5), 250);

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

TEST_F(KaHyParBP, ComputesBalancedSolutionWithNodeWeights) {
  parseIniToContext(context, "../../../config/old_reference_configs/km1_direct_kway_sea17.ini");
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

  for (const HypernodeID& hn : hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, hypergraph.partID(hn));
  }

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), metrics::hyperedgeCut(verification_hypergraph));
  ASSERT_EQ(metrics::soed(hypergraph), metrics::soed(verification_hypergraph));
  ASSERT_EQ(metrics::km1(hypergraph), metrics::km1(verification_hypergraph));
  ASSERT_EQ(metrics::imbalance(hypergraph, context), metrics::imbalance(verification_hypergraph, context));
  ASSERT_LE(metrics::imbalance(hypergraph, context), 0.03);
}

TEST_F(KaHyParE, ComputesDirectKwayKm1Partitioning) {
  parseIniToContext(context, "configs/test.ini");
  context.partition.k = 3;
  context.partition.quiet_mode = true;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.partition.mode = Mode::direct_kway;
  context.initial_partitioning.bp_algo = BinPackingAlgorithm::worst_fit;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
  context.partition.quiet_mode = true;
  context.partition_evolutionary = true;
  context.partition.graph_filename = "../../../tests/partition/evolutionary/TestHypergraph";
  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  PartitionerFacade().partition(hypergraph, context);

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

TEST(KaHyPar, SupportsIndividualBlockWeightsViaInterface) {
  kahypar_context_t* context = kahypar_context_new();
  kahypar_configure_context_from_file(context, "../../../config/old_reference_configs/km1_direct_kway_sea17.ini");

  reinterpret_cast<kahypar::Context*>(context)->preprocessing.enable_community_detection = false;
  reinterpret_cast<kahypar::Context*>(context)->initial_partitioning.bp_algo = BinPackingAlgorithm::worst_fit;
  reinterpret_cast<kahypar::Context*>(context)->initial_partitioning.use_heuristic_prepacking = false;
  reinterpret_cast<kahypar::Context*>(context)->initial_partitioning.enable_early_restart = false;
  reinterpret_cast<kahypar::Context*>(context)->initial_partitioning.enable_late_restart = false;

  HypernodeID num_hypernodes = 0;
  HyperedgeID num_hyperedges = 0;

  size_t* index_ptr = nullptr;
  kahypar_hypernode_id_t* hyperedges_ptr = nullptr;

  kahypar_hyperedge_weight_t* hyperedge_weights_ptr = nullptr;
  kahypar_hypernode_weight_t* vertex_weights_ptr = nullptr;

  const std::string filename("test_instances/ISPD98_ibm01.hgr");
  kahypar_read_hypergraph_from_file(filename.c_str(),
                                    &num_hypernodes,
                                    &num_hyperedges,
                                    &index_ptr,
                                    &hyperedges_ptr,
                                    &hyperedge_weights_ptr,
                                    &vertex_weights_ptr);

  const double imbalance = 0.0;
  const kahypar_partition_id_t num_blocks = 6;
  const std::array<kahypar_hypernode_weight_t, num_blocks> max_part_weights = { 2750, 1000, 3675, 2550, 2550, 250 };

  kahypar_hyperedge_weight_t objective = 0;
  std::vector<kahypar_partition_id_t> partition(num_hypernodes, -1);

  kahypar_set_custom_target_block_weights(num_blocks, max_part_weights.data(), context);

  kahypar_partition(num_hypernodes,
                    num_hyperedges,
                    imbalance,
                    num_blocks,
                    /*vertex_weights */ nullptr,
                    /*hyperedge_weights */ nullptr,
                    index_ptr,
                    hyperedges_ptr,
                    &objective,
                    context,
                    partition.data());

  Hypergraph verification_hypergraph(kahypar::io::createHypergraphFromFile(filename, num_blocks));

  for (const HypernodeID& hn : verification_hypergraph.nodes()) {
    verification_hypergraph.setNodePart(hn, partition[hn]);
  }

  ASSERT_LE(verification_hypergraph.partWeight(0), max_part_weights[0]);
  ASSERT_LE(verification_hypergraph.partWeight(1), max_part_weights[1]);
  ASSERT_LE(verification_hypergraph.partWeight(2), max_part_weights[2]);
  ASSERT_LE(verification_hypergraph.partWeight(3), max_part_weights[3]);
  ASSERT_LE(verification_hypergraph.partWeight(4), max_part_weights[4]);
  ASSERT_LE(verification_hypergraph.partWeight(5), max_part_weights[5]);

  ASSERT_EQ(objective, metrics::km1(verification_hypergraph));

  kahypar_context_free(context);
}
}  // namespace kahypar
