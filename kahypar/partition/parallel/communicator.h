#pragma once



#ifndef KAHYPAR_USE_MPI
namespace kahypar {
class Communicator {
  public :
  
  explicit Communicator() :
    _rank(0),
    _size(1),
    _population_size(MPIPopulationSize::as_usual) { 
    /* These values are fixed, if there is no MPI, the rank should be 0 (the main process) 
    the size should be 1 (only one process) and the method for generating the population size
    should be the method not pertaining information about the mpi status. */  
  }

  inline void init(int argc, char* argv[]) {
  }
  inline void finalize() {

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
  inline void setPopulationSize(const MPIPopulationSize &pop_size) {
    /* This method is intentionally left blank, there is no reason to set any other
    MPI Population Size since this is compiled without MPI*/
  }
  private : 

  const int _rank;
  const int _size;
  const MPIPopulationSize _population_size;
};

} //namespace kahypar

#else


#include <mpi.h>

namespace kahypar {
class Communicator {
  public :
  
  explicit Communicator() :
    _rank(),
    _size(),
    _population_size() {
     
    }

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
  inline void setPopulationSize(const MPIPopulationSize &pop_size) {
    _population_size = pop_size;
  }
  inline MPI_Comm getCommunicator() const{
    return communicator;
  }
  private : 
  
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
} //namespace kahpyar
#endif


