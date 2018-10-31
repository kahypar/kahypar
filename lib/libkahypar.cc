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
#include "kahypar/macros.h"
#include "kahypar/partition/context.h"
#include "kahypar/partitioner_facade.h"
#include "kahypar/utils/randomize.h"


kahypar_context_t* kahypar_context_new() {
  return reinterpret_cast<kahypar_context_t*>(new kahypar::Context());
}

void kahypar_context_free(kahypar_context_t* kahypar_context) {
  if (kahypar_context == nullptr) {
    return;
  }
  delete reinterpret_cast<kahypar::Context*>(kahypar_context);
}


void kahypar_configure_context_from_file(kahypar_context_t* kahypar_context,
                                         const char* ini_file_name) {
  kahypar::parseIniToContext(*reinterpret_cast<kahypar::Context*>(kahypar_context),
                             ini_file_name);
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
  kahypar::Context& context = *reinterpret_cast<kahypar::Context*>(kahypar_context);

  context.partition.k = num_blocks;
  context.partition.epsilon = epsilon;
  context.partition.quiet_mode = true;
  context.partition.write_partition_file = false;

  kahypar::Hypergraph hypergraph(num_vertices,
                                 num_hyperedges,
                                 hyperedge_indices,
                                 hyperedges,
                                 context.partition.k,
                                 hyperedge_weights,
                                 vertex_weights);

  kahypar::PartitionerFacade partitioner;
  partitioner.partition(hypergraph, context);

  *objective = kahypar::metrics::correctMetric(hypergraph, context);

  ASSERT(partition != nullptr);
  for (const auto hn : hypergraph.nodes()) {
    partition[hn] = hypergraph.partID(hn);
  }
}
