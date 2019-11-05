/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <mpi.h>

#include "kahypar/application/command_line_options.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partitioner_facade.h"

int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);
  kahypar::Context context;


  kahypar::processCommandLineInput(context, argc, argv);
  context.mpi.communicator = MPI_COMM_WORLD;
  MPI_Comm_rank(context.mpi.communicator, &context.mpi.rank);
  MPI_Comm_size(context.mpi.communicator, &context.mpi.size);
  context.partition.seed = context.partition.seed + context.mpi.rank;

  kahypar::Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  kahypar::PartitionerFacade().partition(hypergraph, context);


  return 0;
}

