/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Robin Andre <robinandre1995@web.de>
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
#pragma once


#ifdef KAHYPAR_USE_MPI
#include <mpi.h>
#endif

namespace kahypar {
#ifndef KAHYPAR_USE_MPI
class Communicator {
 public:
  explicit Communicator() :
    _rank(0),
    _size(1),
    _population_size(MPIPopulationSize::dynamic_percentage_of_total_time) {
    /* These values are fixed, if there is no MPI, the rank should be 0 (the main process)
    the size should be 1 (only one process) and the method for generating the population size
    should be the method not pertaining information about the mpi status. */
  }

  inline void init(int argc, char* argv[]) { }
  inline void finalize() { }
  inline int getRank() const {
    return _rank;
  }
  inline int getSize() const {
    return _size;
  }
  inline MPIPopulationSize getPopulationSize() const {
    return _population_size;
  }
  inline void setPopulationSize(const MPIPopulationSize& pop_size) {
    /* This method is intentionally left blank, there is no reason to set any other
    MPI Population Size since this is compiled without MPI*/
  }
  inline std::string preface() const{
    return "";
  }
 private:
  const int _rank;
  const int _size;
  const MPIPopulationSize _population_size;
};

#else
class Communicator {
 public:
  explicit Communicator() :
    _rank(),
    _size(),
    _population_size() { }

  inline void init(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    communicator = MPI_COMM_WORLD;
    MPI_Comm_rank(communicator, &_rank);
    MPI_Comm_size(communicator, &_size);
  }
  inline void finalize() {
    MPI_Finalize();
  }

  inline int getRank() const {
    return _rank;
  }
  inline int getSize() const {
    return _size;
  }
  inline MPIPopulationSize getPopulationSize() const {
    return _population_size;
  }
  inline void setPopulationSize(const MPIPopulationSize& pop_size) {
    _population_size = pop_size;
  }
  inline MPI_Comm getCommunicator() const {
    return communicator;
  }
  inline std::string preface() const {
    return "[MPI Rank " + std::to_string(_rank) + "] ";
  }
 private:
  inline void setSize(int size) {
    _size = size;
  }
  int _rank;
  int _size;
  MPIPopulationSize _population_size;
  MPI_Comm communicator;
  friend class TheEvoPartitioner;
  FRIEND_TEST(TheEvoPartitioner, ProperlyGeneratesTheInitialPopulation);
  FRIEND_TEST(TheEvoPartitioner, RespectsTheTimeLimit);
  FRIEND_TEST(TheEvoPartitioner, CalculatesTheRightPopulationSize);
};
#endif
}  // namespace kahpyar
