/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <memory>

#include "gmock/gmock.h"

#include "include/libkahypar.h"

#include "kahypar/macros.h"

#include "tests/io/hypergraph_io_test_fixtures.h"

using ::testing::Eq;
using ::testing::ContainerEq;

namespace kahypar {
TEST(KaHyPar, CanBeCalledViaInterface) {
  kahypar_context_t* context = kahypar_context_new();

  kahypar_configure_context_from_file(context, "../../../config/km1_kKaHyPar_sea20.ini");

  const kahypar_hypernode_id_t num_vertices = 7;
  const kahypar_hyperedge_id_t num_hyperedges = 4;

  std::unique_ptr<kahypar_hyperedge_weight_t[]> hyperedge_weights =
    std::make_unique<kahypar_hyperedge_weight_t[]>(4);

  // force the cut to contain hyperedge 0 and 2
  hyperedge_weights[0] = 1;
  hyperedge_weights[1] = 1000;
  hyperedge_weights[2] = 1;
  hyperedge_weights[3] = 1000;

  std::unique_ptr<size_t[]> hyperedge_indices =
    std::make_unique<size_t[]>(5);

  hyperedge_indices[0] = 0;
  hyperedge_indices[1] = 2;
  hyperedge_indices[2] = 6;
  hyperedge_indices[3] = 9;
  hyperedge_indices[4] = 12;

  std::unique_ptr<kahypar_hypernode_id_t[]> hyperedges =
    std::make_unique<kahypar_hypernode_id_t[]>(12);

  // hypergraph from hMetis manual page 14
  hyperedges[0] = 0;
  hyperedges[1] = 2;
  hyperedges[2] = 0;
  hyperedges[3] = 1;
  hyperedges[4] = 3;
  hyperedges[5] = 4;
  hyperedges[6] = 3;
  hyperedges[7] = 4;
  hyperedges[8] = 6;
  hyperedges[9] = 2;
  hyperedges[10] = 5;
  hyperedges[11] = 6;

  const double imbalance = 0.03;
  const kahypar_partition_id_t k = 2;
  kahypar_hyperedge_weight_t objective = 0;

  std::vector<kahypar_partition_id_t> partition(num_vertices, -1);

  kahypar_partition(num_vertices, num_hyperedges,
                    imbalance, k,
                    /*vertex_weights */ nullptr, hyperedge_weights.get(),
                    hyperedge_indices.get(), hyperedges.get(),
                    &objective, context, partition.data());


  std::vector<kahypar_partition_id_t> correct_solution({ 0, 0, 1, 0, 0, 1, 1 });
  std::vector<kahypar_partition_id_t> correct_solution2({ 1, 1, 0, 1, 1, 0, 0 });
  ASSERT_THAT(partition, AnyOf(::testing::ContainerEq(correct_solution),
                               ::testing::ContainerEq(correct_solution2)));
  ASSERT_EQ(objective, 2);

  kahypar_context_free(context);
}

TEST(KaHyPar, CanHandleFixedVerticesViaInterface) {
  const kahypar_hypernode_id_t num_vertices = 7;
  const kahypar_hyperedge_id_t num_hyperedges = 4;

  std::unique_ptr<size_t[]> hyperedge_indices =
    std::make_unique<size_t[]>(5);

  hyperedge_indices[0] = 0;
  hyperedge_indices[1] = 2;
  hyperedge_indices[2] = 6;
  hyperedge_indices[3] = 9;
  hyperedge_indices[4] = 12;

  std::unique_ptr<kahypar_hypernode_id_t[]> hyperedges =
    std::make_unique<kahypar_hypernode_id_t[]>(12);

  // hypergraph from hMetis manual page 14
  hyperedges[0] = 0;
  hyperedges[1] = 2;
  hyperedges[2] = 0;
  hyperedges[3] = 1;
  hyperedges[4] = 3;
  hyperedges[5] = 4;
  hyperedges[6] = 3;
  hyperedges[7] = 4;
  hyperedges[8] = 6;
  hyperedges[9] = 2;
  hyperedges[10] = 5;
  hyperedges[11] = 6;

  const double imbalance = 0.03;
  const kahypar_partition_id_t k = 2;
  kahypar_hyperedge_weight_t objective = 0;

  kahypar_hypergraph_t* kahypar_hypergraph = kahypar_create_hypergraph(k,
                                                                       num_vertices,
                                                                       num_hyperedges,
                                                                       hyperedge_indices.get(),
                                                                       hyperedges.get(),
                                                                       nullptr,
                                                                       nullptr);

  std::unique_ptr<kahypar_partition_id_t[]> fixed_vertices = std::make_unique<kahypar_partition_id_t[]>(7);
  // vertex 1 and 5 are fixed to blocks 0/1
  fixed_vertices[0] = -1;
  fixed_vertices[1] = 0;
  fixed_vertices[2] = -1;
  fixed_vertices[3] = -1;
  fixed_vertices[4] = -1;
  fixed_vertices[5] = 1;
  fixed_vertices[6] = -1;

  kahypar_set_fixed_vertices(kahypar_hypergraph, fixed_vertices.get());

  Hypergraph& hypergraph = *reinterpret_cast<Hypergraph*>(kahypar_hypergraph);

  ASSERT_FALSE(hypergraph.isFixedVertex(0));
  ASSERT_TRUE(hypergraph.isFixedVertex(1));
  ASSERT_FALSE(hypergraph.isFixedVertex(2));
  ASSERT_FALSE(hypergraph.isFixedVertex(3));
  ASSERT_FALSE(hypergraph.isFixedVertex(4));
  ASSERT_TRUE(hypergraph.isFixedVertex(5));
  ASSERT_FALSE(hypergraph.isFixedVertex(6));

  kahypar_context_t* context = kahypar_context_new();
  kahypar_configure_context_from_file(context, "../../../config/km1_kKaHyPar_sea20.ini");

  std::vector<kahypar_partition_id_t> partition(num_vertices, -1);
  kahypar_partition_hypergraph(kahypar_hypergraph, k, imbalance, &objective, context, partition.data());

  for (kahypar_hypernode_id_t i = 0; i != num_vertices; ++i) {
    LOG << V(i) << V(partition[i]);
  }

  ASSERT_TRUE(hypergraph.partID(1) == 0);
  ASSERT_TRUE(hypergraph.partID(5) == 1);


  // Make partition worse by moving vertex 4/1 to block 0/1 and then improve partition again.
  if (hypergraph.partID(4) == 1) {
    partition[4] = 0;
  } else if (hypergraph.partID(0) == 1) {
    partition[0] = 0;
  }

  hypergraph.reset();

  ASSERT_FALSE(hypergraph.isFixedVertex(0));
  ASSERT_TRUE(hypergraph.isFixedVertex(1));
  ASSERT_FALSE(hypergraph.isFixedVertex(2));
  ASSERT_FALSE(hypergraph.isFixedVertex(3));
  ASSERT_FALSE(hypergraph.isFixedVertex(4));
  ASSERT_TRUE(hypergraph.isFixedVertex(5));
  ASSERT_FALSE(hypergraph.isFixedVertex(6));

  std::vector<kahypar_partition_id_t> improved_partition(num_vertices, -1);
  kahypar_improve_hypergraph_partition(kahypar_hypergraph,
                                       k,
                                       imbalance,
                                       &objective,
                                       context,
                                       partition.data(),
                                       5,
                                       improved_partition.data());

  for (kahypar_hypernode_id_t i = 0; i != num_vertices; ++i) {
    LOG << V(i) << V(improved_partition[i]);
  }

  ASSERT_TRUE(hypergraph.partID(1) == 0);
  ASSERT_TRUE(hypergraph.partID(5) == 1);
  ASSERT_TRUE(hypergraph.partID(4) == 0);

  kahypar_context_free(context);
  kahypar_hypergraph_free(kahypar_hypergraph);
}


TEST(KaHyPar, CanImprovePartitionsViaInterface) {
  kahypar_context_t* context = kahypar_context_new();
  kahypar_configure_context_from_file(context, "../../../config/km1_kKaHyPar_sea20.ini");

  // lower contraction limit to enforce contractions
  reinterpret_cast<kahypar::Context*>(context)->coarsening.contraction_limit_multiplier = 1;

  const kahypar_hypernode_id_t num_vertices = 7;
  const kahypar_hyperedge_id_t num_hyperedges = 4;

  std::unique_ptr<size_t[]> hyperedge_indices =
    std::make_unique<size_t[]>(5);

  hyperedge_indices[0] = 0;
  hyperedge_indices[1] = 2;
  hyperedge_indices[2] = 6;
  hyperedge_indices[3] = 9;
  hyperedge_indices[4] = 12;

  std::unique_ptr<kahypar_hypernode_id_t[]> hyperedges =
    std::make_unique<kahypar_hypernode_id_t[]>(12);

  // hypergraph from hMetis manual page 14
  hyperedges[0] = 0;
  hyperedges[1] = 2;
  hyperedges[2] = 0;
  hyperedges[3] = 1;
  hyperedges[4] = 3;
  hyperedges[5] = 4;
  hyperedges[6] = 3;
  hyperedges[7] = 4;
  hyperedges[8] = 6;
  hyperedges[9] = 2;
  hyperedges[10] = 5;
  hyperedges[11] = 6;

  const double imbalance = 0.03;
  const kahypar_partition_id_t k = 2;
  kahypar_hyperedge_weight_t objective = 0;

  std::unique_ptr<kahypar_hyperedge_weight_t[]> hyperedge_weights =
    std::make_unique<kahypar_hyperedge_weight_t[]>(4);

  // force the the current partition to be bad
  hyperedge_weights[0] = 1000;
  hyperedge_weights[1] = 1;
  hyperedge_weights[2] = 1;
  hyperedge_weights[3] = 1;

  std::vector<kahypar_partition_id_t> input_partition(num_vertices, -1);
  std::vector<kahypar_partition_id_t> improved_partition(num_vertices, -1);

  input_partition[0] = 0;
  input_partition[1] = 0;
  input_partition[2] = 1;
  input_partition[3] = 0;
  input_partition[4] = 0;
  input_partition[5] = 1;
  input_partition[6] = 1;

  kahypar_improve_partition(num_vertices, num_hyperedges,
                            imbalance, k,
                            /*vertex_weights */ nullptr,
                            hyperedge_weights.get(),
                            hyperedge_indices.get(),
                            hyperedges.get(),
                            input_partition.data(),
                            1,
                            &objective,
                            context,
                            improved_partition.data());

  for (kahypar_hypernode_id_t i = 0; i != num_vertices; ++i) {
    LOG << V(i) << V(improved_partition[i]);
  }

  std::vector<kahypar_partition_id_t> correct_solution({ 1, 0, 1, 0, 0, 1, 1 });
  std::vector<kahypar_partition_id_t> correct_solution2({ 0, 0, 0, 1, 1, 1, 1 });


  ASSERT_THAT(improved_partition, AnyOf(::testing::ContainerEq(correct_solution),
                                        ::testing::ContainerEq(correct_solution2)));
  ASSERT_EQ(objective, 2);

  kahypar_context_free(context);
}

TEST(KaHyPar, CanCreateHypergraphsViaInterface) {
  const kahypar_hypernode_id_t num_vertices = 7;
  const kahypar_hyperedge_id_t num_hyperedges = 4;

  std::unique_ptr<size_t[]> hyperedge_indices =
    std::make_unique<size_t[]>(5);

  hyperedge_indices[0] = 0;
  hyperedge_indices[1] = 2;
  hyperedge_indices[2] = 6;
  hyperedge_indices[3] = 9;
  hyperedge_indices[4] = 12;

  std::unique_ptr<kahypar_hypernode_id_t[]> hyperedges =
    std::make_unique<kahypar_hypernode_id_t[]>(12);

  // hypergraph from hMetis manual page 14
  hyperedges[0] = 0;
  hyperedges[1] = 2;
  hyperedges[2] = 0;
  hyperedges[3] = 1;
  hyperedges[4] = 3;
  hyperedges[5] = 4;
  hyperedges[6] = 3;
  hyperedges[7] = 4;
  hyperedges[8] = 6;
  hyperedges[9] = 2;
  hyperedges[10] = 5;
  hyperedges[11] = 6;

  const double imbalance = 0.03;
  const kahypar_partition_id_t k = 2;
  kahypar_hyperedge_weight_t objective = 0;

  std::unique_ptr<kahypar_hyperedge_weight_t[]> hyperedge_weights =
    std::make_unique<kahypar_hyperedge_weight_t[]>(4);

  hyperedge_weights[0] = 1000;
  hyperedge_weights[1] = 1;
  hyperedge_weights[2] = 1;
  hyperedge_weights[3] = 1;


  Hypergraph verification_hypergraph(num_vertices,
                                     num_hyperedges,
                                     hyperedge_indices.get(),
                                     hyperedges.get(),
                                     k,
                                     hyperedge_weights.get(),
                                     nullptr);

  kahypar_hypergraph_t* kahypar_hypergraph = kahypar_create_hypergraph(k,
                                                                       num_vertices,
                                                                       num_hyperedges,
                                                                       hyperedge_indices.get(),
                                                                       hyperedges.get(),
                                                                       hyperedge_weights.get(),
                                                                       nullptr);

  Hypergraph& hypergraph = *reinterpret_cast<Hypergraph*>(kahypar_hypergraph);

  ASSERT_TRUE(verifyEquivalenceWithoutPartitionInfo(hypergraph, verification_hypergraph));

  kahypar_hypergraph_free(kahypar_hypergraph);
}


TEST(KaHyPar, SupportsIndividualBlockWeightsViaInterface) {
  kahypar_context_t* context = kahypar_context_new();
  kahypar_configure_context_from_file(context, "../../../config/km1_kKaHyPar_sea20.ini");

  reinterpret_cast<kahypar::Context*>(context)->preprocessing.enable_community_detection = false;
  reinterpret_cast<kahypar::Context*>(context)->initial_partitioning.enable_early_restart = false;
  reinterpret_cast<kahypar::Context*>(context)->initial_partitioning.enable_late_restart = false;

  HypernodeID num_hypernodes = 7;
  HyperedgeID num_hyperedges = 4;

  std::unique_ptr<size_t[]> hyperedge_indices =
    std::make_unique<size_t[]>(5);

  hyperedge_indices[0] = 0;
  hyperedge_indices[1] = 2;
  hyperedge_indices[2] = 6;
  hyperedge_indices[3] = 9;
  hyperedge_indices[4] = 12;

  std::unique_ptr<kahypar_hypernode_id_t[]> hyperedges =
    std::make_unique<kahypar_hypernode_id_t[]>(12);

  // hypergraph from hMetis manual page 14
  hyperedges[0] = 0;
  hyperedges[1] = 2;
  hyperedges[2] = 0;
  hyperedges[3] = 1;
  hyperedges[4] = 3;
  hyperedges[5] = 4;
  hyperedges[6] = 3;
  hyperedges[7] = 4;
  hyperedges[8] = 6;
  hyperedges[9] = 2;
  hyperedges[10] = 5;
  hyperedges[11] = 6;

  kahypar_hyperedge_weight_t* hyperedge_weights_ptr = nullptr;

  std::unique_ptr<kahypar_hypernode_weight_t[]> vertex_weights =
    std::make_unique<kahypar_hypernode_weight_t[]>(7);

  // force a 4-way partition
  vertex_weights[0] = 300;
  vertex_weights[1] = 1000;
  vertex_weights[2] = 300;
  vertex_weights[3] = 250;
  vertex_weights[4] = 250;
  vertex_weights[5] = 1000;
  vertex_weights[6] = 250;


  const double imbalance = 0.0;
  const kahypar_partition_id_t num_blocks = 4;
  const std::array<kahypar_hypernode_weight_t, num_blocks> max_part_weights = { 1000, 1000, 600, 750 };

  kahypar_hyperedge_weight_t objective = 0;
  std::vector<kahypar_partition_id_t> partition(num_hypernodes, -1);

  kahypar_set_custom_target_block_weights(num_blocks, max_part_weights.data(), context);

  kahypar_partition(num_hypernodes,
                    num_hyperedges,
                    imbalance,
                    num_blocks,
                    vertex_weights.get(),
                    /*hyperedge_weights */ nullptr,
                    hyperedge_indices.get(),
                    hyperedges.get(),
                    &objective,
                    context,
                    partition.data());

  LOG << V(objective);

  Hypergraph verification_hypergraph(num_hypernodes,
                                     num_hyperedges,
                                     hyperedge_indices.get(),
                                     hyperedges.get(),
                                     num_blocks,
                                     hyperedge_weights_ptr,
                                     vertex_weights.get());

  for (const HypernodeID& hn : verification_hypergraph.nodes()) {
    LOG << V(hn) << V(partition[hn]);
    verification_hypergraph.setNodePart(hn, partition[hn]);
  }

  ASSERT_LE(verification_hypergraph.partWeight(0), max_part_weights[0]);
  ASSERT_LE(verification_hypergraph.partWeight(1), max_part_weights[1]);
  ASSERT_LE(verification_hypergraph.partWeight(2), max_part_weights[2]);
  ASSERT_LE(verification_hypergraph.partWeight(3), max_part_weights[3]);

  ASSERT_EQ(objective, metrics::km1(verification_hypergraph));

  kahypar_context_free(context);
}

namespace io {
TEST_F(AnUnweightedHypergraphFile, CanBeParsedIntoAHypergraph) {
  HypernodeID num_hypernodes = 0;
  HyperedgeID num_hyperedges = 0;

  size_t* index_ptr = nullptr;
  kahypar_hypernode_id_t* hyperedges_ptr = nullptr;

  kahypar_hyperedge_weight_t* hyperedge_weights_ptr = nullptr;
  kahypar_hypernode_weight_t* vertex_weights_ptr = nullptr;

  kahypar_read_hypergraph_from_file(_filename.c_str(),
                                    &num_hypernodes,
                                    &num_hyperedges,
                                    &index_ptr,
                                    &hyperedges_ptr,
                                    &hyperedge_weights_ptr,
                                    &vertex_weights_ptr);


  ASSERT_TRUE(verifyEquivalenceWithoutPartitionInfo(Hypergraph(_control_num_hypernodes,
                                                               _control_num_hyperedges,
                                                               _control_index_vector,
                                                               _control_edge_vector),
                                                    Hypergraph(num_hypernodes,
                                                               num_hyperedges,
                                                               index_ptr,
                                                               hyperedges_ptr)));
  delete[] index_ptr;
  delete[] hyperedges_ptr;
}

TEST_F(AHypergraphFileWithHyperedgeWeights, CanBeParsedIntoAHypergraph) {
  HypernodeID num_hypernodes = 0;
  HyperedgeID num_hyperedges = 0;

  size_t* index_ptr = nullptr;
  kahypar_hypernode_id_t* hyperedges_ptr = nullptr;

  kahypar_hyperedge_weight_t* hyperedge_weights_ptr = nullptr;
  kahypar_hypernode_weight_t* vertex_weights_ptr = nullptr;

  kahypar_read_hypergraph_from_file(_filename.c_str(),
                                    &num_hypernodes,
                                    &num_hyperedges,
                                    &index_ptr,
                                    &hyperedges_ptr,
                                    &hyperedge_weights_ptr,
                                    &vertex_weights_ptr);


  ASSERT_TRUE(verifyEquivalenceWithoutPartitionInfo(Hypergraph(_control_num_hypernodes,
                                                               _control_num_hyperedges,
                                                               _control_index_vector,
                                                               _control_edge_vector,
                                                               2,
                                                               &_control_hyperedge_weights),
                                                    Hypergraph(num_hypernodes,
                                                               num_hyperedges,
                                                               index_ptr,
                                                               hyperedges_ptr,
                                                               2,
                                                               hyperedge_weights_ptr)));
  delete[] index_ptr;
  delete[] hyperedges_ptr;
  delete[] hyperedge_weights_ptr;
}

TEST_F(AHypergraphFileWithHypernodeWeights, CanBeParsedIntoAHypergraph) {
  HypernodeID num_hypernodes = 0;
  HyperedgeID num_hyperedges = 0;

  size_t* index_ptr = nullptr;
  kahypar_hypernode_id_t* hyperedges_ptr = nullptr;

  kahypar_hyperedge_weight_t* hyperedge_weights_ptr = nullptr;
  kahypar_hypernode_weight_t* vertex_weights_ptr = nullptr;

  kahypar_read_hypergraph_from_file(_filename.c_str(),
                                    &num_hypernodes,
                                    &num_hyperedges,
                                    &index_ptr,
                                    &hyperedges_ptr,
                                    &hyperedge_weights_ptr,
                                    &vertex_weights_ptr);


  ASSERT_TRUE(verifyEquivalenceWithoutPartitionInfo(Hypergraph(_control_num_hypernodes,
                                                               _control_num_hyperedges,
                                                               _control_index_vector,
                                                               _control_edge_vector,
                                                               2,
                                                               nullptr,
                                                               &_control_hypernode_weights),
                                                    Hypergraph(num_hypernodes,
                                                               num_hyperedges,
                                                               index_ptr,
                                                               hyperedges_ptr,
                                                               2,
                                                               nullptr,
                                                               vertex_weights_ptr)));
  delete[] index_ptr;
  delete[] hyperedges_ptr;
  delete[] vertex_weights_ptr;
}

TEST_F(AHypergraphFileWithHypernodeAndHyperedgeWeights, CanBeParsedIntoAHypergraph) {
  HypernodeID num_hypernodes = 0;
  HyperedgeID num_hyperedges = 0;

  size_t* index_ptr = nullptr;
  kahypar_hypernode_id_t* hyperedges_ptr = nullptr;

  kahypar_hyperedge_weight_t* hyperedge_weights_ptr = nullptr;
  kahypar_hypernode_weight_t* vertex_weights_ptr = nullptr;

  kahypar_read_hypergraph_from_file(_filename.c_str(),
                                    &num_hypernodes,
                                    &num_hyperedges,
                                    &index_ptr,
                                    &hyperedges_ptr,
                                    &hyperedge_weights_ptr,
                                    &vertex_weights_ptr);


  ASSERT_TRUE(verifyEquivalenceWithoutPartitionInfo(Hypergraph(_control_num_hypernodes,
                                                               _control_num_hyperedges,
                                                               _control_index_vector,
                                                               _control_edge_vector,
                                                               2,
                                                               &_control_hyperedge_weights,
                                                               &_control_hypernode_weights),
                                                    Hypergraph(num_hypernodes,
                                                               num_hyperedges,
                                                               index_ptr,
                                                               hyperedges_ptr,
                                                               2,
                                                               hyperedge_weights_ptr,
                                                               vertex_weights_ptr)));
  delete[] index_ptr;
  delete[] hyperedges_ptr;
  delete[] hyperedge_weights_ptr;
  delete[] vertex_weights_ptr;
}

TEST_F(AHypergraphFileWithHypernodeAndHyperedgeWeights, CanBeParsedIntoKaHyParAHypergraph) {
  kahypar_partition_id_t num_blocks = 8;

  kahypar_hypergraph_t* kahypar_hypergraph = kahypar_create_hypergraph_from_file(_filename.c_str(),
                                                                                 num_blocks);

  Hypergraph& hypergraph = *reinterpret_cast<Hypergraph*>(kahypar_hypergraph);

  ASSERT_TRUE(verifyEquivalenceWithoutPartitionInfo(Hypergraph(_control_num_hypernodes,
                                                               _control_num_hyperedges,
                                                               _control_index_vector,
                                                               _control_edge_vector,
                                                               8,
                                                               &_control_hyperedge_weights,
                                                               &_control_hypernode_weights),
                                                    hypergraph));
  kahypar_hypergraph_free(kahypar_hypergraph);
}
}  // namespace io
}  // namespace kahypar
