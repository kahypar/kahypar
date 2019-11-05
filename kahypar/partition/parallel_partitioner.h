/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <math.h>

#include <algorithm>
#include <chrono>

#include "gtest/gtest_prod.h"

#include "kahypar/datastructure/hypergraph.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/context_enum_classes.h"
#include "kahypar/partition/evolutionary/combine.h"
#include "kahypar/partition/evolutionary/diversifier.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/evolutionary/probability_tables.h"
#include "kahypar/partition/parallel/exchanger.h"



namespace kahypar {


class ParallelPartitioner {
 private:
  static constexpr bool debug = true;

 public:
  explicit ParallelPartitioner(const Context& context) :
    _rank(context.mpi.rank),
    _timelimit(),
    _population(context){
    _timelimit = context.partition.time_limit;
  }
  ParallelPartitioner(const ParallelPartitioner&) = delete;
  ParallelPartitioner& operator= (const ParallelPartitioner&) = delete;

  ParallelPartitioner(ParallelPartitioner&&) = delete;
  ParallelPartitioner& operator= (ParallelPartitioner&&) = delete;

  ~ParallelPartitioner() = default;

  inline void partition(Hypergraph& hg, Context& context) {

    
    context.partition_evolutionary = true;
    
    LOG << preface() << "seed: " << context.partition.seed;
    
    
    Exchanger exchanger(context.mpi.communicator, hg.initialNumNodes());
    DBG << preface();
    generateInitialPopulation(hg, context);
    DBG << preface();

    while (Timer::instance().evolutionaryResult().total_evolutionary <= _timelimit) {
      ++context.evolutionary.iteration;


      if (context.evolutionary.diversify_interval != -1 &&
          context.evolutionary.iteration % context.evolutionary.diversify_interval == 0) {
        kahypar::partition::diversify(context);
      }


      EvoDecision decision = decideNextMove(context);
      DBG << preface() << V(decision);
      switch (decision) {
        case EvoDecision::mutation:
          performMutation(hg, context);
          break;
        case EvoDecision::combine:
          performCombine(hg, context);
          break;
        default:
          LOG << preface() << "Error in evo_partitioner.h: Non-covered case in decision making";
          std::exit(EXIT_FAILURE);
      }
      
      unsigned messages = ceil(log(context.mpi.size));
     
      for(unsigned i = 0; i < messages; ++i) {

        exchanger.sendBestIndividual(_population);
        exchanger.clearBuffer();
        exchanger.receiveIndividual(context,hg, _population);
      }
      
    }
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    MPI_Barrier(MPI_COMM_WORLD);      
    exchanger.receiveIndividual(context,hg, _population);  
     
    
    MPI_Barrier(MPI_COMM_WORLD);
    DBG << preface() << "After recieve Barrier"; 
    exchanger.clearBuffer();
    exchanger.collectBestPartition(_population, hg, context);    
    hg.reset();
    hg.setPartition(_population.individualAt(_population.best()).partition());
    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(context, Timepoint::evolutionary,
                            std::chrono::duration<double>(end - start).count());
  }

  const std::vector<PartitionID> & bestPartition() const {
    return _population.individualAt(_population.best()).partition();
  }

 private:
  FRIEND_TEST(TheEvoPartitioner, ProperlyGeneratesTheInitialPopulation);
  FRIEND_TEST(TheEvoPartitioner, RespectsLimitsOfTheInitialPopulation);
  FRIEND_TEST(TheEvoPartitioner, IsCorrectlyDecidingTheActions);
  
  inline unsigned determinePopulationSize(unsigned measured_time_for_one_partition, const Context& context) {
    int estimated_population_size;
    switch(context.mpi.population_size) {
      DBG << preface() << "the chosen strategy " <<context.mpi.population_size;
      case MPIPopulationSize::equal_sequential_time: {
        DBG << preface() << "equal_seq";
        estimated_population_size = std::round(context.evolutionary.dynamic_population_amount_of_time
                                               * context.partition.time_limit
                                               * context.mpi.size 
                                               / measured_time_for_one_partition); 
        break;
      }
      case MPIPopulationSize::equal_to_the_number_of_mpi_processes: {
       DBG << preface() << "equal_tothe";
        estimated_population_size = context.mpi.size;
        break;
      }
      case MPIPopulationSize::as_usual: {
       DBG << preface() << "as usual";
        estimated_population_size = std::round(context.evolutionary.dynamic_population_amount_of_time
                                               * context.partition.time_limit
                                               / measured_time_for_one_partition);
        break;
      }
    }
    DBG << preface() <<"HERE "<<applyPopulationSizeBounds(estimated_population_size);
    DBG << preface() << "THERE";
    return applyPopulationSizeBounds(estimated_population_size);

    
  }
  inline unsigned applyPopulationSizeBounds(int unbound_population_size) {
  
    int minimal_size = std::max(unbound_population_size, 3);
    return std::min(minimal_size, 50);
  }
  
  
  inline void generateInitialPopulation(Hypergraph& hg, Context& context) {
    if (context.evolutionary.dynamic_population_size) {
    
    
      HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
      _population.generateIndividual(hg, context);
      LOG << preface()  << "Population " << _population <<" generateInitialPopulation";
      HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
      Timer::instance().add(context, Timepoint::evolutionary,
                            std::chrono::duration<double>(end - start).count());

      ++context.evolutionary.iteration;
      io::serializer::serializeEvolutionary(context, hg);
      
      context.evolutionary.population_size = determinePopulationSize(Timer::instance().evolutionaryResult().total_evolutionary, context);
      MPI_Bcast(&context.evolutionary.population_size, 1, MPI_INT, 0, context.mpi.communicator);

      
      
      MPI_Barrier(MPI_COMM_WORLD);

    }
    
    
    context.evolutionary.edge_frequency_amount = sqrt(context.evolutionary.population_size);
    
    
    size_t desired_repetitions_for_initial_partitioning;
    DBG << preface() << " Quickstart? "<< context.evolutionary.parallel_partitioning_quick_start;
    if(context.evolutionary.parallel_partitioning_quick_start) {
          //TODO test line remove if in doubt
      //context.evolutionary.population_size = context.mpi.size;
    
      desired_repetitions_for_initial_partitioning = ceil((float) context.evolutionary.population_size / context.mpi.size);

    }
    else {
      desired_repetitions_for_initial_partitioning = context.evolutionary.population_size;
    }
    DBG << preface() << "desired_repetitions "<<desired_repetitions_for_initial_partitioning;
    DBG << preface() << "context.evolutionary.population_size "<<context.evolutionary.population_size;
    
    while (_population.size() < desired_repetitions_for_initial_partitioning &&
           Timer::instance().evolutionaryResult().total_evolutionary <= _timelimit) {
      ++context.evolutionary.iteration;
      HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
      _population.generateIndividual(hg, context);
      LOG << preface()  << "Population " << _population <<" generateInitialPopulation";
      HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
      Timer::instance().add(context, Timepoint::evolutionary,
                            std::chrono::duration<double>(end - start).count());
      io::serializer::serializeEvolutionary(context, hg);
      DBG << preface() << "Population " << _population;
    }
    
    if(context.evolutionary.parallel_partitioning_quick_start) {
      Exchanger exchanger(context.mpi.communicator, hg.initialNumNodes());
      exchanger.exchangeInitialPopulations(_population, context, hg, desired_repetitions_for_initial_partitioning);
    }
    
  }


  inline EvoDecision decideNextMove(const Context& context) {
    if (Randomize::instance().getRandomFloat(0, 1) < context.evolutionary.mutation_chance) {
      return EvoDecision::mutation;
    }
    return EvoDecision::combine;
  }


  inline void performCombine(Hypergraph& hg, const Context& context) {
    EvoCombineStrategy original_strategy = context.evolutionary.combine_strategy;
    context.evolutionary.combine_strategy = pick::appropriateCombineStrategy(context);
    switch (context.evolutionary.combine_strategy) {
      case EvoCombineStrategy::basic: {
          size_t insert_position = _population.insert(combine::usingTournamentSelection(hg, context, _population), context);
          LOG << preface() << "Population " << _population << " basic combine";
          break;
        }
      case EvoCombineStrategy::edge_frequency: {
          size_t insert_position = _population.insert(combine::edgeFrequency(hg, context, _population), context);
          LOG << preface() << "Population " << _population << " edge frequency combine";
          break;
        }
      case EvoCombineStrategy::UNDEFINED:
        LOG << preface() << "Partitioner called without combine strategy";
        std::exit(-1);
        // omit default case to trigger compiler warning for missing cases
    }
    context.evolutionary.combine_strategy = original_strategy;
  }


  // REMEMBER: When switching back to calling force inserts, add the position i.e:
  // _population.forceInsertSaveBest(mutate::vCycleWithNewInitialPartitioning(hg, _population,_population.individualAt(mutation_position), context),  mutation_position);
  inline void performMutation(Hypergraph& hg, const Context& context) {
    const size_t mutation_position = _population.randomIndividual();
    EvoMutateStrategy original_strategy = context.evolutionary.mutate_strategy;
    context.evolutionary.mutate_strategy = pick::appropriateMutateStrategy(context);
    DBG << preface() << V(context.evolutionary.mutate_strategy);
    DBG << preface() << V(mutation_position);
    switch (context.evolutionary.mutate_strategy) {
      case EvoMutateStrategy::new_initial_partitioning_vcycle:

        _population.insert(
          mutate::vCycleWithNewInitialPartitioning(hg,
                                                   _population.individualAt(mutation_position),
                                                   context), context);
        LOG << preface() << "Population " << _population << " vcyclenewIP mutation";
        break;
      case EvoMutateStrategy::vcycle:
        _population.insert(
          mutate::vCycle(hg, _population.individualAt(mutation_position), context), context);
        LOG << preface() << "Population " << _population << " vcycle mutation";
        break;
      case EvoMutateStrategy::UNDEFINED:
        LOG << preface() << "Partitioner called without mutation strategy";
        std::exit(-1);
        // omit default case to trigger compiler warning for missing cases
    }
    context.evolutionary.mutate_strategy = original_strategy;
  }
  inline std::string preface() {
    return "[MPI Rank " + std::to_string(_rank) + "] ";
  }
  inline void print(const Context& context) {
    
  }
  int _rank;
  int _timelimit;
  Population _population;
  

};

}  // namespace kahypar
