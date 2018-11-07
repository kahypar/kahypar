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

typedef unsigned int kahypar_hypernode_id_t;
typedef unsigned int kahypar_hyperedge_id_t;
typedef int kahypar_hypernode_weight_t;
typedef int kahypar_hyperedge_weight_t;
typedef unsigned int kahypar_partition_id_t;

KAHYPAR_API kahypar_context_t* kahypar_context_new();
KAHYPAR_API void kahypar_context_free(kahypar_context_t* kahypar_context);
KAHYPAR_API void kahypar_configure_context_from_file(kahypar_context_t* kahypar_context,
                                                     const char* ini_file_name);

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

#ifdef __cplusplus
}
#endif

#endif    // LIBKAHYPAR_H
