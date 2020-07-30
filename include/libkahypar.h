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

#ifndef LIBKAHYPAR_H
#define LIBKAHYPAR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef KAHYPAR_API
#  ifdef _WIN32
#     if defined(KAHYPAR_BUILD_SHARED)  /* build dll */
#         define KAHYPAR_API __declspec(dllexport)
#     elif !defined(KAHYPAR_BUILD_STATIC)  /* use dll */
#         define KAHYPAR_API __declspec(dllimport)
#     else  /* static library */
#         define KAHYPAR_API
#     endif
#  else
#     if __GNUC__ >= 4
#         define KAHYPAR_API __attribute__ ((visibility("default")))
#     else
#         define KAHYPAR_API
#     endif
#  endif
#endif

struct kahypar_context_s;
typedef struct kahypar_context_s kahypar_context_t;
typedef struct kahypar_hypergraph_s kahypar_hypergraph_t;

typedef unsigned int kahypar_hypernode_id_t;
typedef unsigned int kahypar_hyperedge_id_t;
typedef int kahypar_hypernode_weight_t;
typedef int kahypar_hyperedge_weight_t;
typedef int kahypar_partition_id_t;

KAHYPAR_API kahypar_context_t* kahypar_context_new();
KAHYPAR_API void kahypar_context_free(kahypar_context_t* kahypar_context);
KAHYPAR_API void kahypar_configure_context_from_file(kahypar_context_t* kahypar_context,
                                                     const char* ini_file_name);

KAHYPAR_API void kahypar_hypergraph_free(kahypar_hypergraph_t* hypergraph);

KAHYPAR_API void kahypar_set_custom_target_block_weights(const kahypar_partition_id_t num_blocks,
                                                         const kahypar_hypernode_weight_t* block_weights,
                                                         kahypar_context_t* kahypar_context);

KAHYPAR_API void kahypar_set_fixed_vertices(kahypar_hypergraph_t* hypergraph,
                                            const kahypar_partition_id_t* fixed_vertex_blocks);

KAHYPAR_API kahypar_hypergraph_t* kahypar_create_hypergraph_from_file(const char* file_name,
                                                                      const kahypar_partition_id_t num_blocks);

KAHYPAR_API kahypar_hypergraph_t* kahypar_create_hypergraph(const kahypar_partition_id_t num_blocks,
                                                            const kahypar_hypernode_id_t num_vertices,
                                                            const kahypar_hyperedge_id_t num_hyperedges,
                                                            const size_t* hyperedge_indices,
                                                            const kahypar_hyperedge_id_t* hyperedges,
                                                            const kahypar_hyperedge_weight_t* hyperedge_weights,
                                                            const kahypar_hypernode_weight_t* vertex_weights);

KAHYPAR_API void kahypar_partition_hypergraph(kahypar_hypergraph_t* kahypar_hypergraph,
                                              const kahypar_partition_id_t num_blocks,
                                              const double epsilon,
                                              kahypar_hyperedge_weight_t* objective,
                                              kahypar_context_t* kahypar_context,
                                              kahypar_partition_id_t* partition);

KAHYPAR_API void kahypar_improve_hypergraph_partition(kahypar_hypergraph_t* kahypar_hypergraph,
                                                      const kahypar_partition_id_t num_blocks,
                                                      const double epsilon,
                                                      kahypar_hyperedge_weight_t* objective,
                                                      kahypar_context_t* kahypar_context,
                                                      const kahypar_partition_id_t* input_partition,
                                                      const size_t num_improvement_iterations,
                                                      kahypar_partition_id_t* improved_partition);


KAHYPAR_API void kahypar_hypergraph_free(kahypar_hypergraph_t* kahypar_hypergraph);

KAHYPAR_API void kahypar_read_hypergraph_from_file(const char* file_name,
                                                   kahypar_hypernode_id_t* num_vertices,
                                                   kahypar_hyperedge_id_t* num_hyperedges,
                                                   size_t** hyperedge_indices,
                                                   kahypar_hyperedge_id_t** hyperedges,
                                                   kahypar_hyperedge_weight_t** hyperedge_weights,
                                                   kahypar_hypernode_weight_t** vertex_weights);

KAHYPAR_API void kahypar_partition(const kahypar_hypernode_id_t num_vertices,
                                   const kahypar_hyperedge_id_t num_hyperedges,
                                   const double epsilon,
                                   const kahypar_partition_id_t num_blocks,
                                   const kahypar_hypernode_weight_t* vertex_weights,
                                   const kahypar_hyperedge_weight_t* hyperedge_weights,
                                   const size_t* hyperedge_indices,
                                   const kahypar_hyperedge_id_t* hyperedges,
                                   kahypar_hyperedge_weight_t* objective,
                                   kahypar_context_t* kahypar_context,
                                   kahypar_partition_id_t* partition);


KAHYPAR_API void kahypar_improve_partition(const kahypar_hypernode_id_t num_vertices,
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
                                           kahypar_partition_id_t* improved_partition);

#ifdef __cplusplus
}
#endif

#endif    // LIBKAHYPAR_H
