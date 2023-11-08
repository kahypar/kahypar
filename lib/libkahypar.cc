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

#include "libkahypar.h"

#include "kahypar/application/command_line_options.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar-resources/macros.h"
#include "kahypar/partition/context.h"
#include "kahypar/partitioner_facade.h"
#include "kahypar-resources/utils/randomize.h"
#include "kahypar/utils/validate.h"


kahypar_context_t* kahypar_context_new() {
  return reinterpret_cast<kahypar_context_t*>(new kahypar::Context());
}

void kahypar_set_custom_target_block_weights(const kahypar_partition_id_t num_blocks,
                                             const kahypar_hypernode_weight_t* block_weights,
                                             kahypar_context_t* kahypar_context) {
  ASSERT(block_weights != nullptr);

  kahypar::Context& context = *reinterpret_cast<kahypar::Context*>(kahypar_context);
  context.partition.use_individual_part_weights = true;

  for (kahypar_partition_id_t i = 0; i != num_blocks; ++i) {
    context.partition.max_part_weights.push_back(block_weights[i]);
  }
}

void kahypar_context_free(kahypar_context_t* kahypar_context) {
  if (kahypar_context == nullptr) {
    return;
  }
  delete reinterpret_cast<kahypar::Context*>(kahypar_context);
}

void kahypar_hypergraph_free(kahypar_hypergraph_t* kahypar_hypergraph) {
  if (kahypar_hypergraph == nullptr) {
    return;
  }
  delete reinterpret_cast<kahypar::Hypergraph*>(kahypar_hypergraph);
}

void kahypar_set_seed(kahypar_context_t* kahypar_context, const int seed) {
  kahypar::Context& context = *reinterpret_cast<kahypar::Context*>(kahypar_context);
  context.partition.seed =seed;
}

void kahypar_supress_output(kahypar_context_t* kahypar_context, const bool decision) {
  kahypar::Context& context = *reinterpret_cast<kahypar::Context*>(kahypar_context);
  context.partition.quiet_mode = decision;
}

void kahypar_configure_context_from_file(kahypar_context_t* kahypar_context,
                                         const char* ini_file_name) {
  kahypar::parseIniToContext(*reinterpret_cast<kahypar::Context*>(kahypar_context),
                             ini_file_name);
}

void kahypar_set_fixed_vertices(kahypar_hypergraph_t* kahypar_hypergraph,
                                const kahypar_partition_id_t* fixed_vertex_blocks) {
  kahypar::Hypergraph& hypergraph = *reinterpret_cast<kahypar::Hypergraph*>(kahypar_hypergraph);
  for (const auto hn : hypergraph.nodes()) {
    if (fixed_vertex_blocks[hn] != -1) {
      hypergraph.setFixedVertex(hn, fixed_vertex_blocks[hn]);
    }
  }
}

kahypar_hypergraph_t* kahypar_create_hypergraph_from_file(const char* file_name, const kahypar_partition_id_t num_blocks) {
  kahypar::Hypergraph* hypergraph = new kahypar::Hypergraph();
  *hypergraph = kahypar::io::createHypergraphFromFile(file_name, num_blocks, VALIDATE_INPUT, PROMOTE_WARNINGS_TO_ERRORS);
  return reinterpret_cast<kahypar_hypergraph_t*>(hypergraph);
}

kahypar_hypergraph_t* kahypar_create_hypergraph(const kahypar_partition_id_t num_blocks,
                                                const kahypar_hypernode_id_t num_vertices,
                                                const kahypar_hyperedge_id_t num_hyperedges,
                                                const size_t* hyperedge_indices,
                                                const kahypar_hyperedge_id_t* hyperedges,
                                                const kahypar_hyperedge_weight_t* hyperedge_weights,
                                                const kahypar_hypernode_weight_t* vertex_weights) {
  if (VALIDATE_INPUT) {
    std::vector<kahypar_hyperedge_id_t> ignored_hes;
    std::vector<size_t> ignored_pins;
    kahypar::io::validateAndPrintErrors(num_vertices, num_hyperedges, hyperedge_indices, hyperedges,
                                        hyperedge_weights, vertex_weights, {},
                                        ignored_hes, ignored_pins, PROMOTE_WARNINGS_TO_ERRORS);
  return reinterpret_cast<kahypar_hypergraph_t*>(new kahypar::Hypergraph(num_vertices,
                                                                         num_hyperedges,
                                                                         hyperedge_indices,
                                                                         hyperedges,
                                                                         num_blocks,
                                                                         hyperedge_weights,
                                                                         vertex_weights,
                                                                         ignored_hes,
                                                                         ignored_pins));
  }
  return reinterpret_cast<kahypar_hypergraph_t*>(new kahypar::Hypergraph(num_vertices,
                                                                         num_hyperedges,
                                                                         hyperedge_indices,
                                                                         hyperedges,
                                                                         num_blocks,
                                                                         hyperedge_weights,
                                                                         vertex_weights));
}

bool kahypar_validate_input(const kahypar_hypernode_id_t num_vertices,
                            const kahypar_hyperedge_id_t num_hyperedges,
                            const size_t* hyperedge_indices,
                            const kahypar_hyperedge_id_t* hyperedges,
                            const kahypar_hyperedge_weight_t* hyperedge_weights,
                            const kahypar_hypernode_weight_t* vertex_weights,
                            const bool print_errors) {
  // The C interface provides no API for constructing a hypergraph from input with duplicate pins if input
  // validation is disabled. Thus we always treat this (and other warnings) as an error.
  const bool promote_warnings_to_errors = true;

  std::vector<kahypar::validate::InputError> errors;
  bool found_error = kahypar::validate::validateInput(num_vertices, num_hyperedges, hyperedge_indices, hyperedges,
                                                      hyperedge_weights, vertex_weights, &errors);
  if (print_errors && found_error) {
    kahypar::validate::printErrors(num_hyperedges, errors, {}, promote_warnings_to_errors);
  }
  return !found_error;
}

void kahypar_partition_hypergraph(kahypar_hypergraph_t* kahypar_hypergraph,
                                  const kahypar_partition_id_t num_blocks,
                                  const double epsilon,
                                  kahypar_hyperedge_weight_t* objective,
                                  kahypar_context_t* kahypar_context,
                                  kahypar_partition_id_t* partition) {
  kahypar::Context& context = *reinterpret_cast<kahypar::Context*>(kahypar_context);
  kahypar::Hypergraph& hypergraph = *reinterpret_cast<kahypar::Hypergraph*>(kahypar_hypergraph);
  ASSERT(!context.partition.use_individual_part_weights ||
         !context.partition.max_part_weights.empty());
  ASSERT(partition != nullptr);

  context.partition.k = num_blocks;
  context.partition.epsilon = epsilon;
  context.partition.write_partition_file = false;

  if (context.partition.vcycle_refinement_for_input_partition) {
    for (const auto hn : hypergraph.nodes()) {
      hypergraph.setNodePart(hn, partition[hn]);
    }
  }

  kahypar::PartitionerFacade().partition(hypergraph, context);

  *objective = kahypar::metrics::correctMetric(hypergraph, context);

  for (const auto hn : hypergraph.nodes()) {
    partition[hn] = hypergraph.partID(hn);
  }

  context.partition.perfect_balance_part_weights.clear();
  context.partition.max_part_weights.clear();
  context.evolutionary.communities.clear();
}


void kahypar_improve_hypergraph_partition(kahypar_hypergraph_t* kahypar_hypergraph,
                                          const kahypar_partition_id_t num_blocks,
                                          const double epsilon,
                                          kahypar_hyperedge_weight_t* objective,
                                          kahypar_context_t* kahypar_context,
                                          const kahypar_partition_id_t* input_partition,
                                          const size_t num_improvement_iterations,
                                          kahypar_partition_id_t* improved_partition) {
  kahypar::Context& context = *reinterpret_cast<kahypar::Context*>(kahypar_context);
  kahypar::Hypergraph& hypergraph = *reinterpret_cast<kahypar::Hypergraph*>(kahypar_hypergraph);
  ALWAYS_ASSERT(context.partition.mode == kahypar::Mode::direct_kway,
                "V-cycle refinement of input partitions is only possible in direct k-way mode");
  ASSERT(*std::max_element(input_partition, input_partition + hypergraph.initialNumNodes()) == num_blocks - 1);
  ASSERT([&]() {
    std::unordered_set<kahypar_partition_id_t> set(input_partition, input_partition + hypergraph.initialNumNodes());
    LOG << V(set.size());
    for (kahypar_partition_id_t i = 0; i < num_blocks; ++i) {
      if (set.find(i) == set.end()) {
        return false;
      }
    }
    return true;
  } (), "Partition is corrupted.");

  // toggle v-cycle refinement
  context.partition.vcycle_refinement_for_input_partition = true;
  // perform one v-cycle
  context.partition.global_search_iterations = num_improvement_iterations;
  // sparsifier has to be disabled for v-cycle refinement
  context.preprocessing.enable_min_hash_sparsifier = false;

  // use improved_partition as temporary_input_partition
  std::memcpy(improved_partition, input_partition, hypergraph.initialNumNodes() * sizeof(kahypar_partition_id_t));

  kahypar_partition_hypergraph(kahypar_hypergraph,
                               num_blocks,
                               epsilon,
                               objective,
                               kahypar_context,
                               improved_partition);
}

void kahypar_read_hypergraph_from_file(const char* file_name,
                                       kahypar_hypernode_id_t* num_vertices,
                                       kahypar_hyperedge_id_t* num_hyperedges,
                                       size_t** hyperedge_indices,
                                       kahypar_hyperedge_id_t** hyperedges,
                                       kahypar_hyperedge_weight_t** hyperedge_weights,
                                       kahypar_hypernode_weight_t** vertex_weights) {
  std::unique_ptr<size_t[]> indices_ptr(nullptr);
  std::unique_ptr<kahypar_hyperedge_id_t[]> hyperedges_ptr(nullptr);

  std::unique_ptr<kahypar_hyperedge_weight_t[]> hyperedge_weights_ptr(nullptr);
  std::unique_ptr<kahypar_hypernode_weight_t[]> vertex_weights_ptr(nullptr);

  kahypar::io::readHypergraphFile(file_name, *num_vertices, *num_hyperedges,
                                  indices_ptr, hyperedges_ptr, hyperedge_weights_ptr,
                                  vertex_weights_ptr);

  *hyperedge_indices = indices_ptr.release();
  *hyperedges = hyperedges_ptr.release();
  *hyperedge_weights = hyperedge_weights_ptr.release();
  *vertex_weights = vertex_weights_ptr.release();
}


void kahypar_partition(const kahypar_hypernode_id_t num_vertices,
                       const kahypar_hyperedge_id_t num_hyperedges,
                       const double epsilon,
                       const kahypar_partition_id_t num_blocks,
                       const kahypar_hypernode_weight_t* vertex_weights,
                       const kahypar_hyperedge_weight_t* hyperedge_weights,
                       const size_t* hyperedge_indices,
                       const kahypar_hyperedge_id_t* hyperedges,
                       kahypar_hyperedge_weight_t* objective,
                       kahypar_context_t* kahypar_context,
                       kahypar_partition_id_t* partition) {
  kahypar_hypergraph_t* hypergraph = kahypar_create_hypergraph(num_blocks, num_vertices, num_hyperedges,
                                                               hyperedge_indices, hyperedges,
                                                               hyperedge_weights, vertex_weights);
  kahypar_partition_hypergraph(hypergraph, num_blocks, epsilon, objective, kahypar_context, partition);
  kahypar_hypergraph_free(hypergraph);
}


void kahypar_improve_partition(const kahypar_hypernode_id_t num_vertices,
                               const kahypar_hyperedge_id_t num_hyperedges,
                               const double epsilon,
                               const kahypar_partition_id_t num_blocks,
                               const kahypar_hypernode_weight_t* vertex_weights,
                               const kahypar_hyperedge_weight_t* hyperedge_weights,
                               const size_t* hyperedge_indices,
                               const kahypar_hyperedge_id_t* hyperedges,
                               const kahypar_partition_id_t* input_partition,
                               const size_t num_improvement_iterations,
                               kahypar_hyperedge_weight_t* objective,
                               kahypar_context_t* kahypar_context,
                               kahypar_partition_id_t* improved_partition) {
  kahypar::Context& context = *reinterpret_cast<kahypar::Context*>(kahypar_context);
  ALWAYS_ASSERT(context.partition.mode == kahypar::Mode::direct_kway,
                "V-cycle refinement of input partitions is only possible in direct k-way mode");
  ASSERT(*std::max_element(input_partition, input_partition + num_vertices) == num_blocks - 1);
  ASSERT([&]() {
    std::unordered_set<kahypar_partition_id_t> set(input_partition, input_partition + num_vertices);
    LOG << V(set.size());
    for (kahypar_partition_id_t i = 0; i < num_blocks; ++i) {
      if (set.find(i) == set.end()) {
        return false;
      }
    }
    return true;
  } (), "Partition is corrupted.");

  // toggle v-cycle refinement
  context.partition.vcycle_refinement_for_input_partition = true;
  // perform one v-cycle
  context.partition.global_search_iterations = num_improvement_iterations;
  // sparsifier has to be disabled for v-cycle refinement
  context.preprocessing.enable_min_hash_sparsifier = false;

  // use improved_partition as temporary_input_partition
  std::memcpy(improved_partition, input_partition, num_vertices * sizeof(kahypar_partition_id_t));

  kahypar_partition(num_vertices,
                    num_hyperedges,
                    epsilon,
                    num_blocks,
                    vertex_weights,
                    hyperedge_weights,
                    hyperedge_indices,
                    hyperedges,
                    objective,
                    kahypar_context,
                    improved_partition);
}
