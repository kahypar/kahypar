#pragma once



#ifndef KAHYPAR_USE_MPI
namespace kahypar {
class Exchanger {
  

 public: 
  Exchanger(size_t hypergraph_size) { }
  
  inline void exchangeInitialPopulations(Population& population, const Context& context, Hypergraph& hg, const int& amount_of_individuals_already_generated) { }
  inline void collectBestPartition(Population& population, Hypergraph& hg, const Context& context) { }
  
  inline void sendMessages(const Context& context, Hypergraph& hg, Population& population) { }
  inline void broadcastPopulationSize(Context& context) { }
};
} //namespace kahypar
#else 



#include "kahypar/partition/parallel/partition_buffer.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/utils/randomize.h"

#include <mpi.h>

namespace kahypar {
class Exchanger {
  

 public: 
  Exchanger(size_t hypergraph_size) : 
    _current_best_fitness(std::numeric_limits<int>::max()),
    _maximum_allowed_pushes(),
    _number_of_pushes(),
    _rank(),
    _individual_already_sent_to(),
    _partition_buffer(),
    _MPI_Partition(),
    _m_communicator(MPI_COMM_WORLD) {
    
      MPI_Comm_rank( _m_communicator, &_rank);
      int comm_size;
      MPI_Comm_size( _m_communicator, &comm_size);
      if(comm_size > 2) {
        _maximum_allowed_pushes = ceil(log2(comm_size));
      }
      else {
        _maximum_allowed_pushes = 1;
      }

      _individual_already_sent_to = std::vector<bool>(comm_size);
      
      MPI_Type_contiguous(hypergraph_size, MPI_INT, &_MPI_Partition);
      MPI_Type_commit(&_MPI_Partition);
    }
  Exchanger(const Exchanger&) = delete;
  Exchanger& operator= (const Exchanger&) = delete;

  Exchanger(Exchanger&&) = delete;
  Exchanger& operator= (Exchanger&&) = delete;

  ~Exchanger() {
      MPI_Barrier( _m_communicator );
        
      int flag;
      MPI_Status st;
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _m_communicator, &flag, &st);
      DBG << preface() << "DESTRUCTOR";  
      while(flag) {
        int message_length;
        MPI_Get_count(&st, MPI_INT, &message_length);
        DBG << preface() << "Entering the fray";            
        int* partition_map = new int[message_length];
        MPI_Status rst;
        MPI_Recv( partition_map, message_length, MPI_INT, st.MPI_SOURCE, _rank, _m_communicator, &rst); 
                
        delete[] partition_map;
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _m_communicator, &flag, &st);
      } 
      MPI_Type_free(&_MPI_Partition);
      MPI_Barrier( _m_communicator );
    }          

  
  
    //inline void exchangeInitialPopulationse() {
    //}
  
  inline void exchangeInitialPopulations(Population& population, const Context& context, Hypergraph& hg, const int& amount_of_individuals_already_generated) {
    int number_of_exchanges_required = context.evolutionary.population_size - amount_of_individuals_already_generated;
    if(number_of_exchanges_required < 0) {
      number_of_exchanges_required = 0;
    }
    for(int i = 0; i < number_of_exchanges_required; ++i) {
      exchangeIndividuals(population, context, hg);
      DBG << preface() << "Population " << population <<" exchange individuals";
    }
    
  }
  
  
  
  
  
  
  
  
  
  
  inline void collectBestPartition(Population& population, Hypergraph& hg, const Context& context) {
  
  
    MPI_Barrier(MPI_COMM_WORLD);      
    receiveIndividual(context,hg, population);  
     
    
    MPI_Barrier(MPI_COMM_WORLD);
    DBG << preface() << "After recieve Barrier"; 
    clearBuffer();
    DBG << preface() << "Collect Best Partition";
    std::vector<PartitionID> best_local_partition(population.individualAt(population.best()).partition());
    int best_local_objective = population.individualAt(population.best()).fitness();
    int best_global_objective = 0;
    
    MPI_Allreduce(&best_local_objective, &best_global_objective, 1, MPI_INT, MPI_MIN, _m_communicator);
    
    int broadcast_rank = std::numeric_limits<int>::max();
    int global_broadcaster = 0;
    DBG << preface() << "Best Local Partition: " << best_local_objective << " Best Global Partition: " << best_global_objective;
    if(best_local_objective == best_global_objective) {
      broadcast_rank = _rank;
    }
    
    MPI_Allreduce(&broadcast_rank, &global_broadcaster, 1, MPI_INT, MPI_MIN, _m_communicator);
    DBG << preface() << "Determine Broadcaster (MAXINT if NOT) " << broadcast_rank;
    MPI_Bcast(best_local_partition.data(), hg.initialNumNodes(), MPI_INT, global_broadcaster, _m_communicator);
     
    hg.setPartition(best_local_partition);
    population.insert(Individual(hg, context), context);
  }
  
  inline void sendMessages(const Context& context, Hypergraph& hg, Population& population) {
  
      unsigned messages = ceil(log(context.communicator.size));
     
      for(unsigned i = 0; i < messages; ++i) {

        sendBestIndividual(population);
        clearBuffer();
        receiveIndividual(context,hg, population);
      }

  }
  inline void broadcastPopulationSize(Context& context) {
    MPI_Bcast(&context.evolutionary.population_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

      
      
    MPI_Barrier(MPI_COMM_WORLD);
  }

  
 private: 
   inline void clearBuffer() {
    _partition_buffer.releaseBuffer();
  }
 inline void sendBestIndividual(const Population& population) {

    if(population.individualAt(population.best()).fitness() < _current_best_fitness) {
      
      _current_best_fitness = population.individualAt(population.best()).fitness();
      
      
      for(unsigned i = 0; i < _individual_already_sent_to.size(); ++i) {
        _individual_already_sent_to[i] = false;
      }  
      _individual_already_sent_to[_rank] = true;
      _number_of_pushes = 0;
    }
    
    bool something_todo = false;
    for(unsigned i = 0; i < _individual_already_sent_to.size(); ++i) {
      if(!_individual_already_sent_to[i]) {
        something_todo = true;
        break;
      }
    }
    if(_number_of_pushes > _maximum_allowed_pushes) {
      something_todo = false;
    }
    
    if(something_todo) {
      int new_target = _rank;
      
      
      //Determining the target via randomization
      std::vector<int> randomized_targets(_individual_already_sent_to.size());
      std::iota(std::begin(randomized_targets), std::end(randomized_targets), 0);
      Randomize::instance().shuffleVector(randomized_targets, randomized_targets.size());
      
      for(unsigned i = 0; i < _individual_already_sent_to.size(); ++i) { 
        int current_target = randomized_targets[i];
	      if(!_individual_already_sent_to[current_target]) {
          new_target = current_target;
	        break;
        }
      }
      

      
      DBG << preface() << " sending to " << new_target << "..."<< "fitness " << population.individualAt(population.best()).fitness();
      const std::vector<PartitionID>& partition_vector = population.individualAt(population.best()).partition();

      BufferElement ele = _partition_buffer.acquireBuffer(partition_vector);
      DBG << preface() << " buffersize: " << _partition_buffer.size();
      MPI_Isend(ele.partition, 1, _MPI_Partition, new_target, new_target, _m_communicator, ele.request);
      _number_of_pushes++;
      _individual_already_sent_to[new_target] = true;
      //_partition_buffer.acquireBuffer(request, partition_map);
    }
    //_partition_buffer.releaseBuffer();

  }
 inline void receiveIndividual(const Context& context, Hypergraph& hg, Population& population) {

    int flag; 
    MPI_Status st;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _m_communicator, &flag, &st);
    
    while(flag) {
    int* partition_vector_pointer = new int[hg.initialNumNodes()];
    MPI_Status rst;
    MPI_Recv(partition_vector_pointer, 1, _MPI_Partition, st.MPI_SOURCE, _rank, _m_communicator, &rst); 
    std::vector<PartitionID> result_individual_partition(partition_vector_pointer, partition_vector_pointer + hg.initialNumNodes());
    delete[] partition_vector_pointer;
    hg.reset();
    hg.setPartition(result_individual_partition);
    size_t insertion_value = population.insert(Individual(hg, context), context);
    if(insertion_value == std::numeric_limits<unsigned>::max()) {
      DBG << preface() << "INSERTION DISCARDED";
      break;
    }
    else {
      LOG << preface() << "Population " << population << "receive individual";
    }
    
    int received_fitness = population.individualAt(insertion_value).fitness();
    
    LOG << preface() << "Population " << "received Individual from" << st.MPI_SOURCE << "with fitness" << received_fitness;
    
    LOG << preface() << " buffersize: " << _partition_buffer.size(); 
    if(received_fitness < _current_best_fitness) {
      
      _current_best_fitness = received_fitness;
                       
      for(unsigned i = 0; i < _individual_already_sent_to.size(); ++i) {
        _individual_already_sent_to[i] = false;
      }
      _individual_already_sent_to[_rank] = true;
      _number_of_pushes = 0;
    }
    _individual_already_sent_to[st.MPI_SOURCE] = true;
      
     MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _m_communicator, &flag, &st);
     }
  }
 
  inline void exchangeIndividuals(Population& population, const Context& context, Hypergraph& hg) {

    
    int amount_of_mpi_processes;
    MPI_Comm_size( _m_communicator, &amount_of_mpi_processes);
    std::vector<int> permutation_of_mpi_process_numbers(amount_of_mpi_processes);
    
    
    /*Master Thread generates a degenerate permutation. all processes have to use the same permutation
      in order for the exchange protocol to work*/
    if(_rank == 0) {
      std::iota (std::begin(permutation_of_mpi_process_numbers), std::end(permutation_of_mpi_process_numbers), 0);
      for(unsigned i = 1; i < amount_of_mpi_processes; ++i) {
         int random_int_smaller_than_i = Randomize::instance().getRandomInt(0, i - 1);
         std::swap(permutation_of_mpi_process_numbers[i], permutation_of_mpi_process_numbers[random_int_smaller_than_i]);
         
      }
    }

    MPI_Bcast(permutation_of_mpi_process_numbers.data(), amount_of_mpi_processes, MPI_INT, 0, _m_communicator);

    int sending_to = permutation_of_mpi_process_numbers[_rank];
    int receiving_from = 0;
    for(unsigned i = 0; i < permutation_of_mpi_process_numbers.size(); ++i) {
      if (permutation_of_mpi_process_numbers[i] == _rank) {
        receiving_from = i;
        break;
      }
    }
       
    std::vector<PartitionID> outgoing_partition = population.individualAt(population.randomIndividual()).partition();
    int* received_partition_pointer = new int[outgoing_partition.size()];
    DBG << preface() << "sending to " << sending_to << "quick_start";
    MPI_Status st;
    MPI_Sendrecv( outgoing_partition.data(), 1, _MPI_Partition, sending_to, 0, 
                  received_partition_pointer, 1, _MPI_Partition, receiving_from, 0, _m_communicator, &st); 
                  

    std::vector<int> received_partition_vector(received_partition_pointer, received_partition_pointer + outgoing_partition.size());
    
    
    hg.reset();
    hg.setPartition(received_partition_vector);
    population.insert(Individual(hg, context), context);
    LOG << preface() << ":"  << "Population " << population << "exchange individuals l.227";
    delete[] received_partition_pointer;
  }
  
  HyperedgeWeight _current_best_fitness;
  int _maximum_allowed_pushes;
  int _number_of_pushes;
  int _rank;
  std::vector<bool> _individual_already_sent_to;
  PartitionBuffer _partition_buffer;
  MPI_Datatype _MPI_Partition;
  MPI_Comm _m_communicator;
  
  static constexpr bool debug = true;
  
  inline std::string preface() {
       return "[MPI Rank " + std::to_string(_rank) + "] ";
  }
};
}
 //namespace kahypar
#endif








