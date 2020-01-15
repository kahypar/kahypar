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

#include <algorithm>

#ifdef KAHYPAR_USE_MPI
#include <mpi.h>

#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/utils/randomize.h"
#endif

namespace kahypar {
#ifndef KAHYPAR_USE_MPI

class Exchanger {
 public:
  Exchanger(size_t hypergraph_size) { }

  inline void exchangeInitialPopulations(Population& population, const Context& context, Hypergraph& hg, const int& amount_of_individuals_already_generated) { }
  inline void collectBestPartition(Population& population, Hypergraph& hg, const Context& context) { }

  inline void sendMessages(const Context& context, Hypergraph& hg, Population& population) { }
  inline void broadcastPopulationSize(Context& context) { }
};

#else

class Exchanger {
 public:
  Exchanger(size_t hypergraph_size) :
    _current_best_fitness(std::numeric_limits<int>::max()),
    _maximum_allowed_pushes(),
    _number_of_pushes(),
    _rank(),
    _individual_already_sent_to(),
    _MPI_Partition(),
    _communicator(MPI_COMM_WORLD){
    MPI_Comm_rank(_communicator, &_rank);
    int comm_size;
    MPI_Comm_size(_communicator, &comm_size);
    if (comm_size > 2) {
      _maximum_allowed_pushes = ceil(log2(comm_size));
    } else {
      _maximum_allowed_pushes = 1;
    }

    _individual_already_sent_to = std::vector<bool>(comm_size);
    _receive_vector.resize(hypergraph_size);
    MPI_Type_contiguous(hypergraph_size, MPI_INT, &_MPI_Partition);
    MPI_Type_commit(&_MPI_Partition);
  }
  Exchanger(const Exchanger&) = delete;
  Exchanger& operator= (const Exchanger&) = delete;

  Exchanger(Exchanger&&) = delete;
  Exchanger& operator= (Exchanger&&) = delete;

  ~Exchanger() {
    MPI_Barrier(_communicator);

    int flag;
    MPI_Status st;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _communicator, &flag, &st);
    DBG << preface() << "DESTRUCTOR";
    while (flag) {
      int message_length;
      MPI_Get_count(&st, MPI_INT, &message_length);
      DBG << preface() << "Entering the fray";
      int* partition_map = new int[message_length];
      MPI_Status rst;
      MPI_Recv(partition_map, message_length, MPI_INT, st.MPI_SOURCE, _rank, _communicator, &rst);

      delete[] partition_map;
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _communicator, &flag, &st);
    }
    MPI_Type_free(&_MPI_Partition);
    MPI_Barrier(_communicator);
  }


  inline void exchangeInitialPopulations(Population& population, const Context& context, Hypergraph& hg, const int& amount_of_individuals_already_generated) {
    const size_t number_of_exchanges_required = std::max(context.evolutionary.population_size - amount_of_individuals_already_generated, static_cast<size_t>(0));
    for (int i = 0; i < number_of_exchanges_required; ++i) {
      exchangeIndividuals(population, context, hg);
      DBG << preface() << "Population " << population << " exchange individuals";
    }
  }


  inline void collectBestPartition(Population& population, Hypergraph& hg, const Context& context) {
      DBG << preface() << "Starting Collect Best Partition";
    MPI_Barrier(MPI_COMM_WORLD);
    receiveIndividuals(context, hg, population);


    MPI_Barrier(MPI_COMM_WORLD);
    DBG << preface() << " Second Collect Best Partition";
    std::vector<PartitionID> best_local_partition(population.individualAt(population.best()).partition());
    int best_local_objective = population.individualAt(population.best()).fitness();
    int best_global_objective = 0;

    MPI_Allreduce(&best_local_objective, &best_global_objective, 1, MPI_INT, MPI_MIN, _communicator);

    int broadcast_rank = std::numeric_limits<int>::max();
    int global_broadcaster = 0;
    DBG << preface() << "Best Local Partition: " << best_local_objective << " Best Global Partition: " << best_global_objective;
    if (best_local_objective == best_global_objective) {
      broadcast_rank = _rank;
    }

    MPI_Allreduce(&broadcast_rank, &global_broadcaster, 1, MPI_INT, MPI_MIN, _communicator);
    DBG << preface() << "Determine Broadcaster (MAXINT if NOT) " << broadcast_rank;
    MPI_Bcast(best_local_partition.data(), hg.initialNumNodes(), MPI_INT, global_broadcaster, _communicator);

    hg.setPartition(best_local_partition);
    population.insert(Individual(hg, context), context);
  }

  inline void sendMessages(const Context& context, Hypergraph& hg, Population& population) {
    const size_t messages = ceil(log(context.communicator.getSize()));

    for (size_t i = 0; i < messages; ++i) {
      DBG << preface() << "Iteration Number: " <<i;
      sendBestIndividual(population);

      receiveIndividuals(context, hg, population);
    }
  }
  inline void broadcastPopulationSize(Context& context) {
    MPI_Bcast(&context.evolutionary.population_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }

 private:
  FRIEND_TEST(TheBiggerExchanger, ProperlyExchangesIndividuals);
  FRIEND_TEST(TheBiggerExchanger, ProperlyExchangesInitialPopulations);
  FRIEND_TEST(TheBiggerExchanger, DoesNotCollapseIfMultipleProcessesAct);
  std::vector<std::vector<PartitionID>> _send_partition_buffer;

  inline void sendBestIndividual(const Population& population) {
    if (population.individualAt(population.best()).fitness() < _current_best_fitness) {
      _current_best_fitness = population.individualAt(population.best()).fitness();
      openAllTargets();
      resetSendQuota();
      updatePartitionBuffer(population);
      DBG << preface() << " Improved Partition | SendbuffSize: " << _send_partition_buffer.size();

    }

    if (hasOpenTargets() && isWithinSendQuota()) {
      int new_target = getRandomOpenTarget();


      DBG << preface() << " sending to " << new_target << "..." << "fitness " << population.individualAt(population.best()).fitness() << " position "  << population.best()   << " my pointer: "  << _send_partition_buffer.back().data() << " my size: " << _send_partition_buffer.size();

      MPI_Request request;
      MPI_Isend(_send_partition_buffer.back().data(), 1, _MPI_Partition, new_target, new_target, _communicator, &request);
      incrementSendQuotaCounter();
      closeTarget(new_target);
    }
  }

  inline void receiveIndividuals(const Context& context, Hypergraph& hg, Population& population) {
    int flag;
    MPI_Status st;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _communicator, &flag, &st);
    while (flag) {
      MPI_Status rst;
      MPI_Recv(_receive_vector.data(), 1, _MPI_Partition, st.MPI_SOURCE, _rank, _communicator, &rst);
      DBG << preface() << "Attempting a receive: " << " from MPI: " << st.MPI_SOURCE;
      hg.setPartition(_receive_vector);

      size_t insertion_value = population.insert(Individual(hg, context), context);

      if (insertion_value == std::numeric_limits<unsigned>::max()) {
        DBG << preface() << "INSERTION DISCARDED";
        break;
      }
      DBG << preface() << "inserted at position " << insertion_value << " value: " << population.individualAt(insertion_value).fitness();

      int received_fitness = population.individualAt(insertion_value).fitness();

      if (received_fitness < _current_best_fitness) {
        _current_best_fitness = received_fitness;
        openAllTargets();
        resetSendQuota();
        updatePartitionBuffer(population);
      }
      closeTarget(st.MPI_SOURCE);  // We do not want to send the partition back
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _communicator, &flag, &st);
    }
  }
  inline void exchangeIndividuals(Population& population, const Context& context, Hypergraph& hg) {
    std::vector<int> permutation_of_mpi_process_numbers(context.communicator.getSize());

    /*Master Thread generates a degenerate permutation. all processes have to use the same permutation
      in order for the exchange protocol to work*/
    if (_rank == 0) {
      std::iota(std::begin(permutation_of_mpi_process_numbers), std::end(permutation_of_mpi_process_numbers), 0);
      Randomize::instance().shuffleVectorDegenerate(permutation_of_mpi_process_numbers);
    }
    MPI_Bcast(permutation_of_mpi_process_numbers.data(), context.communicator.getSize(), MPI_INT, 0, _communicator);


    int sending_to = permutation_of_mpi_process_numbers[_rank];
    int receiving_from = findCorrespondingSender(permutation_of_mpi_process_numbers);

    const std::vector<PartitionID>& outgoing_partition = population.individualAt(population.randomIndividual()).partition();
    DBG << preface() << "receiving from " << receiving_from << " sending to " << sending_to << "quick_start";
    MPI_Status st;

    MPI_Sendrecv(outgoing_partition.data(), 1, _MPI_Partition, sending_to, 0,
                 _receive_vector.data(), 1, _MPI_Partition, receiving_from, 0, _communicator, &st);

    hg.setPartition(_receive_vector);
    population.insert(Individual(hg, context), context);
    DBG << preface() << ":" << "Population " << population << "exchange individuals";
  }


// HELPER METHODS BELOW


  /*
  This method resets the possible targets, since it is not clever to send to oneself
  the value of sending to oneself is set to true
  */
  inline void openAllTargets() {
    for (unsigned i = 0; i < _individual_already_sent_to.size(); ++i) {
      _individual_already_sent_to[i] = false;
    }
    closeTarget(_rank);
  }
  inline void resetSendQuota() {
    _number_of_pushes = 0;
  }
  inline bool hasOpenTargets() {
    bool something_todo = false;
    for (unsigned i = 0; i < _individual_already_sent_to.size(); ++i) {
      if (!_individual_already_sent_to[i]) {
        something_todo = true;
        break;
      }
    }
    return something_todo;
  }
  inline void updatePartitionBuffer(const Population& population) {
    _send_partition_buffer.push_back(population.individualAt(population.best()).partition());
  }
  //Question: This is as in KaHiP, but KaHiP runs in the problem that there will be 2 sends if the maximum is 1 ?
  inline bool isWithinSendQuota() {
    return (_number_of_pushes <= _maximum_allowed_pushes);
  }

  /* This method will return the position of a random MPI process number which is allowed to receive from this
     process (origin). Will return -1 as error if no suitable targets are found. (Reasons why a process is not a valid target:
     target == origin (pointless operation); target already received from origin ; origin received their partition
     from target.
  */
  inline int getRandomOpenTarget() {
    std::vector<int> randomized_targets(_individual_already_sent_to.size());
    std::iota(std::begin(randomized_targets), std::end(randomized_targets), 0);
    Randomize::instance().shuffleVector(randomized_targets, randomized_targets.size());

    for (unsigned i = 0; i < _individual_already_sent_to.size(); ++i) {
      int current_target = randomized_targets[i];
      if (!_individual_already_sent_to[current_target]) {
        return current_target;
      }
    }
    return -1;
  }
  inline void closeTarget(const int target) {
    _individual_already_sent_to[target] = true;
  }
  inline void incrementSendQuotaCounter() {
    _number_of_pushes++;
  }

  inline int findCorrespondingSender(const std::vector<int>& vector) {
    for (unsigned i = 0; i < vector.size(); ++i) {
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
  MPI_Comm _communicator;
  std::vector<PartitionID> _receive_vector;

  static constexpr bool debug = true;

  inline std::string preface() {
    return "[MPI Rank " + std::to_string(_rank) + "] ";
  }
};
#endif
} // namespace kahypar
