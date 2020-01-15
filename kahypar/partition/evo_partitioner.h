/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
class EvoPartitioner {
 private:
  static constexpr bool debug = true;

 public:
  explicit EvoPartitioner(const Hypergraph& hg, const Context& context) :
    _timelimit(),
    _population(context),
    _exchanger(hg.initialNumNodes()) {
    _timelimit = context.partition.time_limit;
  }
  EvoPartitioner(const EvoPartitioner&) = delete;
  EvoPartitioner& operator= (const EvoPartitioner&) = delete;

  EvoPartitioner(EvoPartitioner&&) = delete;
  EvoPartitioner& operator= (EvoPartitioner&&) = delete;

  ~EvoPartitioner() = default;

  inline void partition(Hypergraph& hg, Context& context) {
    context.partition_evolutionary = true;

    LOG << context.communicator.preface() << "seed: " << context.partition.seed;


    generateInitialPopulation(hg, context);

    while (Timer::instance().evolutionaryResult().total_evolutionary <= _timelimit) {
      ++context.evolutionary.iteration;


      if (context.evolutionary.diversify_interval != -1 &&
          context.evolutionary.iteration % context.evolutionary.diversify_interval == 0) {
        kahypar::partition::diversify(context);
      }


      EvoDecision decision = decideNextMove(context);
      DBG << context.communicator.preface() << V(decision);
      switch (decision) {
        case EvoDecision::mutation:
          performMutation(hg, context);
          break;
        case EvoDecision::combine:
          performCombine(hg, context);
          break;
        default:
          LOG << context.communicator.preface() << "Error in evo_partitioner.h: Non-covered case in decision making";
          std::exit(EXIT_FAILURE);
      }
      _exchanger.sendMessages(context, hg, _population);
    }
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();


    _exchanger.collectBestPartition(_population, hg, context);


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
  FRIEND_TEST(TheEvoPartitioner, CalculatesTheRightPopulationSize);
  FRIEND_TEST(TheEvoPartitioner, MaintainsPopulationBounds);
  FRIEND_TEST(TheEvoPartitioner, RespectsTheTimeLimit);

  inline unsigned determinePopulationSize(double measured_time_for_one_partition, const Context& context) {
    LOG << context.communicator.preface() << "MEASURED TIME " << measured_time_for_one_partition;
    int estimated_population_size;
    switch (context.communicator.getPopulationSize()) {
      LOG << context.communicator.preface() << "the chosen strategy " << context.communicator.getPopulationSize();
      case MPIPopulationSize::dynamic_percentage_of_total_time_times_num_procs:
        LOG << context.communicator.preface() << "dynamic_percentage_of_total_time_times_num_procs";
        estimated_population_size = std::round(context.evolutionary.dynamic_population_amount_of_time
                                               * context.partition.time_limit
                                               * context.communicator.getSize()
                                               / measured_time_for_one_partition);
        break;
      case MPIPopulationSize::equal_to_the_number_of_mpi_processes:
        LOG << context.communicator.preface() << "equal_to_the_number_of_mpi_processes";
        estimated_population_size = context.communicator.getSize();
        break;
      case MPIPopulationSize::dynamic_percentage_of_total_time:
        LOG << context.communicator.preface() << "dynamic_percentage_of_total_time";
        estimated_population_size = std::round(context.evolutionary.dynamic_population_amount_of_time
                                               * context.partition.time_limit
                                               / measured_time_for_one_partition);
        break;
    }

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
      DBG << context.communicator.preface() << "Population " << _population << " generateInitialPopulation";
      HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
      Timer::instance().add(context, Timepoint::evolutionary,
                            std::chrono::duration<double>(end - start).count());

      ++context.evolutionary.iteration;
      io::serializer::serializeEvolutionary(context, hg);

      context.evolutionary.population_size = determinePopulationSize(Timer::instance().evolutionaryResult().total_evolutionary, context);
      LOG << context.communicator.preface() << context.evolutionary.population_size;

      _exchanger.broadcastPopulationSize(context);
    }


    context.evolutionary.edge_frequency_amount = sqrt(context.evolutionary.population_size);


    size_t desired_repetitions_for_initial_partitioning;
    DBG << context.communicator.preface() << " Quickstart? " << context.evolutionary.parallel_partitioning_quick_start;

    if (context.evolutionary.parallel_partitioning_quick_start) {
      desired_repetitions_for_initial_partitioning = ceil((float)context.evolutionary.population_size / context.communicator.getSize());
    } else {
      desired_repetitions_for_initial_partitioning = context.evolutionary.population_size;
    }
    DBG << context.communicator.preface() << "desired_repetitions " << desired_repetitions_for_initial_partitioning;
    DBG << context.communicator.preface() << "context.evolutionary.population_size " << context.evolutionary.population_size;

    while (_population.size() < desired_repetitions_for_initial_partitioning &&
           Timer::instance().evolutionaryResult().total_evolutionary <= _timelimit) {
      ++context.evolutionary.iteration;
      HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
      _population.generateIndividual(hg, context);
      DBG << context.communicator.preface() << "Population " << _population << " generateInitialPopulation";
      HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
      Timer::instance().add(context, Timepoint::evolutionary,
                            std::chrono::duration<double>(end - start).count());
      io::serializer::serializeEvolutionary(context, hg);
      DBG << context.communicator.preface() << "Population " << _population;
    }


    if (context.evolutionary.parallel_partitioning_quick_start) {
      _exchanger.exchangeInitialPopulations(_population, context, hg, desired_repetitions_for_initial_partitioning);
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
          DBG << context.communicator.preface() << "Population " << _population << " basic combine";
          break;
        }
      case EvoCombineStrategy::edge_frequency: {
          size_t insert_position = _population.insert(combine::edgeFrequency(hg, context, _population), context);
          DBG << context.communicator.preface() << "Population " << _population << " edge frequency combine";
          break;
        }
      case EvoCombineStrategy::UNDEFINED:
        LOG << context.communicator.preface() << "Partitioner called without combine strategy";
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
    DBG << context.communicator.preface() << V(context.evolutionary.mutate_strategy);
    DBG << context.communicator.preface() << V(mutation_position);
    switch (context.evolutionary.mutate_strategy) {
      case EvoMutateStrategy::new_initial_partitioning_vcycle:

        _population.insert(
          mutate::vCycleWithNewInitialPartitioning(hg,
                                                   _population.individualAt(mutation_position),
                                                   context), context);
        DBG << context.communicator.preface() << "Population " << _population << " vcyclenewIP mutation";
        break;
      case EvoMutateStrategy::vcycle:
        _population.insert(
          mutate::vCycle(hg, _population.individualAt(mutation_position), context), context);
        DBG << context.communicator.preface() << "Population " << _population << " vcycle mutation";
        break;
      case EvoMutateStrategy::UNDEFINED:
        LOG << context.communicator.preface() << "Partitioner called without mutation strategy";
        std::exit(-1);
        // omit default case to trigger compiler warning for missing cases
    }
    context.evolutionary.mutate_strategy = original_strategy;
  }

  int _timelimit;
  Exchanger _exchanger;
  Population _population;
};
}  // namespace kahypar
