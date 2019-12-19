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
  
      unsigned messages = ceil(log(context.communicator.getSize()));
     
      for(unsigned i = 0; i < messages; ++i) {

        sendBestIndividual(population);
        receiveIndividual(context,hg, population);
      }

  }
  inline void broadcastPopulationSize(Context& context) {
    MPI_Bcast(&context.evolutionary.population_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

      
      
    MPI_Barrier(MPI_COMM_WORLD);
  }

  
 private: 
   FRIEND_TEST(TheBiggerExchanger, ProperlyExchangesIndividuals);
   FRIEND_TEST(TheBiggerExchanger, ProperlyExchangesInitialPopulations);
   FRIEND_TEST(TheBiggerExchanger, DoesNotCollapseIfMultipleProcessesAct);

   inline void sendBestIndividual(const Population& population) {

    if(population.individualAt(population.best()).fitness() < _current_best_fitness) {
      
      _current_best_fitness = population.individualAt(population.best()).fitness();
      openAllTargets();
      resetSendQuota();
    }

    if(hasOpenTargets() && isWithinSendQuota()) {
    
      int new_target = getRandomOpenTarget();
     
      DBG << preface() << " sending to " << new_target << "..."<< "fitness " << population.individualAt(population.best()).fitness();
      const std::vector<PartitionID>& partition_vector = population.individualAt(population.best()).partition();

      MPI_Request request;
      MPI_Isend(&partition_vector[0], 1, _MPI_Partition, new_target, new_target, _m_communicator, &request);
      incrementSendQuota();
      closeTarget(new_target);

    }
  }
  
  inline void receiveIndividual(const Context& context, Hypergraph& hg, Population& population) {

    int flag; 
    MPI_Status st;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _m_communicator, &flag, &st);
    DBG << preface() << "Receiving";
    while(flag) {
    
      std::vector<PartitionID> receive_vector;
      receive_vector.resize(hg.initialNumNodes());
      MPI_Status rst;
      MPI_Recv(&receive_vector[0], 1, _MPI_Partition, st.MPI_SOURCE, _rank, _m_communicator, &rst); 
      hg.reset();
      hg.setPartition(receive_vector);
    
      size_t insertion_value = population.insert(Individual(hg, context), context);
      
      if(insertion_value == std::numeric_limits<unsigned>::max()) {
        DBG << preface() << "INSERTION DISCARDED";
        break;
      }
      DBG << preface() << insertion_value;
    
      int received_fitness = population.individualAt(insertion_value).fitness();
       
      if(received_fitness < _current_best_fitness) {
      
        _current_best_fitness = received_fitness;   
        openAllTargets();              
        resetSendQuota();
      }
      closeTarget(st.MPI_SOURCE); //We do not want to send the partition back
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _m_communicator, &flag, &st);
    }
  }
  inline void exchangeIndividuals(Population& population, const Context& context, Hypergraph& hg) {

    std::vector<int> permutation_of_mpi_process_numbers(context.communicator.getSize());
    
    /*Master Thread generates a degenerate permutation. all processes have to use the same permutation
      in order for the exchange protocol to work*/
    if(_rank == 0) {
      std::iota (std::begin(permutation_of_mpi_process_numbers), std::end(permutation_of_mpi_process_numbers), 0);
      degenerate(permutation_of_mpi_process_numbers);
      
    }
    MPI_Bcast(permutation_of_mpi_process_numbers.data(), context.communicator.getSize(), MPI_INT, 0, _m_communicator);
    

    int sending_to = permutation_of_mpi_process_numbers[_rank];
    int receiving_from = findCorrespondingSender(permutation_of_mpi_process_numbers);

    std::vector<PartitionID> outgoing_partition = population.individualAt(population.randomIndividual()).partition();
    std::vector<PartitionID> received_partition_vector;
    received_partition_vector.resize(hg.initialNumNodes());
    DBG << preface() << "receiving from " << receiving_from << " sending to " << sending_to << "quick_start";
    MPI_Status st;
    
    MPI_Sendrecv( outgoing_partition.data(), 1, _MPI_Partition, sending_to, 0, 
                  &received_partition_vector[0], 1, _MPI_Partition, receiving_from, 0, _m_communicator, &st); 
     


    
    
    hg.reset();
    hg.setPartition(received_partition_vector);
    population.insert(Individual(hg, context), context);
    DBG << preface() << ":"  << "Population " << population << "exchange individuals";

  }  
  
  
  
  
// HELPER METHODS BELOW
  
  
  
  
  
  
  
 /*
 This method resets the possible targets, since it is not clever to send to oneself 
 the value of sending to oneself is set to true
 */
 inline void openAllTargets() {
   for(unsigned i = 0; i < _individual_already_sent_to.size(); ++i) {
     _individual_already_sent_to[i] = false;
   }  
   closeTarget(_rank);
 }
 inline void resetSendQuota() {
   _number_of_pushes = 0;
 }
 inline bool hasOpenTargets() {
   bool something_todo = false;
   for(unsigned i = 0; i < _individual_already_sent_to.size(); ++i) {
     if(!_individual_already_sent_to[i]) {
       something_todo = true;
       break;
     }
   }
   return something_todo;
 
 }
 inline bool isWithinSendQuota() {
   bool within_quota = true;
   if(_number_of_pushes > _maximum_allowed_pushes) {
     within_quota = false;
   }
   return within_quota;
 }
 
 
 inline int getRandomOpenTarget() {
 
 
   std::vector<int> randomized_targets(_individual_already_sent_to.size());
   std::iota(std::begin(randomized_targets), std::end(randomized_targets), 0);
   Randomize::instance().shuffleVector(randomized_targets, randomized_targets.size());
   
   for(unsigned i = 0; i < _individual_already_sent_to.size(); ++i) { 
     int current_target = randomized_targets[i];
	   if(!_individual_already_sent_to[current_target]) {
        return current_target;
       }
     }
   return -1;
 }
 inline void closeTarget(const int target) {
   _individual_already_sent_to[target] = true;
 }
 inline void incrementSendQuota() {
   _number_of_pushes++;
 }
 

  //Permutates the vector so that no element is left on the original position in the vector. 
  inline void degenerate(std::vector<int>& vector) {
    for(unsigned i = 1; i < vector.size(); ++i) {
         
         int random_int_smaller_than_i = Randomize::instance().getRandomInt(0, i - 1);
         std::swap(vector[i], vector[random_int_smaller_than_i]);
    }
  }
  inline int findCorrespondingSender(const std::vector<int>& vector) {

    for(unsigned i = 0; i < vector.size(); ++i) {
      if (vector[i] == _rank) {
        return i;
        
      }
    }
    return -1;
  }
  
 
  HyperedgeWeight _current_best_fitness;
  int _maximum_allowed_pushes;
  int _number_of_pushes;
  int _rank;
  std::vector<bool> _individual_already_sent_to;
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








