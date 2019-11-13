
#pragma once
#include <mpi.h>

#include "kahypar/datastructure/hypergraph.h"

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
  inline void setSeed() {
    
  }

  
  
  
  
  
  
  inline void broadcastPopulationSize() {
  
  }
  

  //MPIParameters params;
  int rank;
  int size;
  MPIPopulationSize population_size;

  //Exchanger exchanger;
  private : 

};

}
