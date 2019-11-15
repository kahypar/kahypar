#pragma once



#ifndef KAHYPAR_USE_MPI
namespace kahypar {
class Communicator {
  public :
  
  explicit Communicator() { }

  inline void init(int argc, char* argv[]) {
  }
  inline void finalize() {

  }
  int rank;
  int size;
  MPIPopulationSize population_size;
  private : 

};

} //namespace kahypar

#else


#include <mpi.h>

namespace kahypar {
class Communicator {
  public :
  
  explicit Communicator() :
    rank(),
    size(),
    population_size() {
    }

  inline void init(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    MPI_Comm communicator = MPI_COMM_WORLD;
    MPI_Comm_rank(communicator, &rank);
    MPI_Comm_size(communicator, &size);
  }
  inline void finalize() {
    MPI_Finalize();
  }

  int rank;
  int size;
  MPIPopulationSize population_size;


  private : 

};
} //namespace kahpyar
#endif


