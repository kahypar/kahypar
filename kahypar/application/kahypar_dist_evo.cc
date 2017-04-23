/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/definitions.h"
#include "kahypar/kahypar.h"
#include "kahypar/partition/evo/dist_evo_partitioner.h"


int main(int argc, char* argv[]) {
  MPI_Init(&argc, &argv);

  int rank = -1;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  LOG << V(rank);
  return 0;
}
