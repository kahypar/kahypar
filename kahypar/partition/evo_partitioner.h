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

#include <chrono>
#include <math.h>
#include "kahypar/datastructure/hypergraph.h"
#include "kahypar/io/evolutionary_io.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/context_enum_classes.h"
#include "kahypar/partition/evolutionary/combine.h"
#include "kahypar/partition/evolutionary/diversifier.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/evolutionary/probability_tables.h"
#include "gtest/gtest_prod.h"

namespace kahypar {
namespace partition {
class EvoPartitioner {
 private:
  static constexpr bool debug = false;

 public:
  explicit EvoPartitioner(const Context& context) :
    _globalstart(),
    _timelimit(),
    _population()
    {
    _globalstart = std::chrono::high_resolution_clock::now();
    _timelimit = context.evolutionary.time_limit_seconds;
  }

  inline void evo_partition(Hypergraph& hg, Context& context) {
    context.partition_evolutionary = true;
    measureTime(context);
    Timer.add();


    generateInitialPopulation(hg, context);

    while (context.evolutionary.elapsed_seconds_total.count() <= _timelimit) {
      ++context.evolutionary.iteration;
      
      
      if (context.evolutionary.diversify_interval != -1 &&
        context.evolutionary.iteration % context.evolutionary.diversify_interval == 0) {
        kahypar::partition::diversify(context);
      }
      
      
      EvoDecision decision = decideNextMove(context);
      DBG << V(decision);
      switch (decision) {
           
        case EvoDecision::mutation:
          performMutation(hg, context);
          DBG <<_population;
          break;
        case EvoDecision::combine:
          performCombine(hg, context);
          DBG <<_population;
          break;
        default:
          LOG << "Error in evo_partitioner.h: Non-covered case in decision making";
          std::exit(EXIT_FAILURE);
      }

      measureTime(context);
    }
    hg.reset();
    hg.setPartition(_population.individualAt(_population.best()).partition());
  }

 private:
 FRIEND_TEST(TheEvoPartitioner, ProperlyGeneratesTheInitialPopulation);
  FRIEND_TEST(TheEvoPartitioner, RespectsLimitsOfTheInitialPopulation);
  FRIEND_TEST(TheEvoPartitioner, IsCorrectlyDecidingTheActions);
  inline void generateInitialPopulation(Hypergraph& hg, Context& context) {
      //INITIAL POPULATION
    if(context.evolutionary.dynamic_population_size) {
      _population.generateIndividual(hg, context); 
      ++context.evolutionary.iteration;
      measureTime(context);
      io::serializer::serializeEvolutionary(context, hg);
      int dynamic_population_size = std::round(context.evolutionary.dynamic_population_amount_of_time
                                           * context.evolutionary.time_limit_seconds
                                           / context.evolutionary.elapsed_seconds_total.count());
      int minimal_size = std::max(dynamic_population_size, 3);
      
      context.evolutionary.population_size = std::min(minimal_size, 50);
      DBG << context.evolutionary.population_size;
      DBG << _population;
    }
    //TODO IMPLEMENT DYNAMIC EDGE FREQUENCY AMOUNT
    context.evolutionary.edge_frequency_amount = sqrt(context.evolutionary.population_size);
    DBG << "EDGE-FREQUENCY-AMOUNT";
    DBG << context.evolutionary.edge_frequency_amount;
    while (_population.size() < context.evolutionary.population_size &&
      context.evolutionary.elapsed_seconds_total.count() <= _timelimit) {
      ++context.evolutionary.iteration;
      _population.generateIndividual(hg, context);
      measureTime(context);
      io::serializer::serializeEvolutionary(context, hg);
      //verbose(context, 0);
      DBG << _population;
      
    }
  }
  
  
  inline EvoDecision decideNextMove(const Context& context) {
    /*if (context.evolutionary.diversify_interval != -1 &&
        context.evolutionary.iteration % context.evolutionary.diversify_interval == 0) {
      
      return EvoDecision::diversify;
    }*/
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
        // ASSERT(result.fitness <= parents.first.fitness && result.fitness <= parents.second.fitness);
        //size_t insert_position = 
        _population.insert(combine::usingTournamentSelection(hg, context, _population), context);
        //verbose(context, insert_position);
        break;
        }
      /*case EvoCombineStrategy::with_edge_frequency_information: {
        //size_t insert_position =
        _population.insert(combine::usingTournamentSelectionAndEdgeFrequency(hg,
                                                                             context,
                                                                             _population),
                           context);
        //verbose(context, insert_position);      
        break;
        }*/
      case EvoCombineStrategy::edge_frequency: {
        //size_t insert_position =
        _population.insert(combine::edgeFrequency(hg, context, _population), context);
        //verbose(context, insert_position);
        break;
        }
      case EvoCombineStrategy::UNDEFINED: {
        LOG << "Partitioner called without combine strategy";
        break;
        }
    }
    context.evolutionary.combine_strategy = original_strategy;
  }


 
  //REMEMBER: When switching back to calling force inserts, add the position i.e:
  //_population.forceInsertSaveBest(mutate::removePopulationStableNets(hg, _population,_population.individualAt(mutation_position), context),  mutation_position);
  inline void performMutation(Hypergraph& hg, const Context& context) {
    const size_t mutation_position = _population.randomIndividual();
    EvoMutateStrategy original_strategy = context.evolutionary.mutate_strategy;
    context.evolutionary.mutate_strategy = pick::appropriateMutateStrategy(context);
    DBG << V(context.evolutionary.mutate_strategy);
    DBG << V(mutation_position);
    switch (context.evolutionary.mutate_strategy) {
      case EvoMutateStrategy::new_initial_partitioning_vcycle: {
     
        _population.insert(
          mutate::vCycleWithNewInitialPartitioning(hg,
                                                   _population.individualAt(mutation_position),
                                                   context), context);
        //verbose(context, mutation_position);
        break;
        }
      case EvoMutateStrategy::vcycle: {
        _population.insert(
          mutate::vCycle(hg, _population.individualAt(mutation_position), context), context);
        //verbose(context, mutation_position);
        break;
        }
      case EvoMutateStrategy::UNDEFINED: {
        LOG << "Partitioner called without mutation strategy";
        break;
        }
    }
    context.evolutionary.mutate_strategy = original_strategy;
  }
  inline void verbose(const Context& context, size_t position) {
    io::printPopulationBanner(context);
    //LOG << _population.individualAt(_population.worst()).fitness();
    unsigned number_of_digits = 0;
    unsigned n = _population.individualAt(_population.worst()).fitness();
    unsigned best = _population.best();
      do {
        ++number_of_digits; 
        n /= 10;
      } while (n);
    
    for(size_t i = 0; i < _population.size(); ++i) {
      if(i == position) {
        std::cout  <<">" <<  _population.individualAt(i).fitness() << "<";
      } else 
      if(i == best) {
        std::cout   <<"(" <<  _population.individualAt(i).fitness() << ")";
      } else {
        std::cout  <<" " <<  _population.individualAt(i).fitness() << " ";
      }
      
    }LOG << "";
    for(size_t i = 0; i < _population.size(); ++i) {
      std::cout << " ";
      std::cout << std::setw(number_of_digits) << _population.difference(_population.individualAt(best), i, true);
      std::cout << " ";
    }
    LOG << "";
  }

  inline void measureTime(const Context& context) {
    const HighResClockTimepoint currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_seconds_total = currentTime - _globalstart;
    context.evolutionary.elapsed_seconds_total = elapsed_seconds_total;
  }

  HighResClockTimepoint _globalstart;
  int _timelimit;
  Population _population;

};
}  // namespace partition
}  // namespace kahypar
