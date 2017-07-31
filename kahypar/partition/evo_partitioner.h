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

namespace kahypar {
namespace partition {
class EvoPartitioner {
 private:
  static constexpr bool debug = true;

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
    context.evolutionary.elapsed_seconds_total = measureTime();
    if(context.evolutionary.dynamic_population_size) {
      _population.generateIndividual(hg, context); 
      ++context.evolutionary.iteration;
      context.evolutionary.elapsed_seconds_total = measureTime();
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
    context.evolutionary.stable_net_amount = sqrt(context.evolutionary.population_size);
    while (_population.size() < context.evolutionary.population_size &&
      context.evolutionary.elapsed_seconds_total.count() <= _timelimit) {
      ++context.evolutionary.iteration;
      _population.generateIndividual(hg, context);
      context.evolutionary.elapsed_seconds_total = measureTime();
      io::serializer::serializeEvolutionary(context, hg);

      DBG << _population;
      
    }
  
   /*context.evolutionary.mutate_strategy = EvoMutateStrategy::edge_frequency;
    performMutation(hg, context);
    _population.print();
    return;*/
    /*performCrossCombine(hg,context);
    LOG <<_population;
    performCrossCombine(hg,context);
    LOG <<_population;
    performCrossCombine(hg,context);
    LOG <<_population;
    performCrossCombine(hg,context);
    LOG <<_population;
    performCrossCombine(hg,context);
    LOG <<_population;
    performCrossCombine(hg,context);
    LOG <<_population;
    return;*/
    /*context.evolutionary.cross_combine_strategy = EvoCrossCombineStrategy::louvain;
    performCrossCombine(hg, context);
    LOG <<_population;
    return;*/
    //context.evolutionary.mutate_strategy = EvoMutateStrategy::single_stable_net;
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
        case EvoDecision::cross_combine:
          performCrossCombine(hg, context);
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

      context.evolutionary.elapsed_seconds_total = measureTime();
    }
  }

 private:
  inline EvoDecision decideNextMove(const Context& context) {
    /*if (context.evolutionary.diversify_interval != -1 &&
        context.evolutionary.iteration % context.evolutionary.diversify_interval == 0) {
      
      return EvoDecision::diversify;
    }*/
    if (Randomize::instance().getRandomFloat(0, 1) < context.evolutionary.mutation_chance) {
      return EvoDecision::mutation;
    }
    if (Randomize::instance().getRandomFloat(0, 1) < context.evolutionary.cross_combine_chance) {
      return EvoDecision::cross_combine;
    }
    return EvoDecision::combine;
  }


  inline void performCombine(Hypergraph& hg, const Context& context) {
    EvoCombineStrategy original_strategy = context.evolutionary.combine_strategy;
    context.evolutionary.combine_strategy = pick::appropriateCombineStrategy(context);
    switch (context.evolutionary.combine_strategy) {
      case EvoCombineStrategy::basic:
        // ASSERT(result.fitness <= parents.first.fitness && result.fitness <= parents.second.fitness);
        _population.insert(combine::usingTournamentSelection(hg, context, _population), context);
        break;
      case EvoCombineStrategy::with_edge_frequency_information:
        _population.insert(combine::usingTournamentSelectionAndEdgeFrequency(hg,
                                                                             context,
                                                                             _population),
                           context);
        break;
    }
    context.evolutionary.combine_strategy = original_strategy;
  }


  inline void performCrossCombine(Hypergraph& hg, const Context& context) {
    context.evolutionary.cross_combine_strategy = pick::appropriateCrossCombineStrategy(context);
    _population.insert(combine::crossCombine(hg, _population.singleTournamentSelection(),
                                             context), context);
  }

  //TODO the best element may be mutated, but in that case the result must be better
  inline void performMutation(Hypergraph& hg, const Context& context) {
    const size_t mutation_position = _population.randomIndividual();
    EvoMutateStrategy original_strategy = context.evolutionary.mutate_strategy;
    context.evolutionary.mutate_strategy = pick::appropriateMutateStrategy(context);
    DBG << V(context.evolutionary.mutate_strategy);
    DBG << V(mutation_position);
    switch (context.evolutionary.mutate_strategy) {
      case EvoMutateStrategy::new_initial_partitioning_vcycle:
     
        _population.forceInsertSaveBest(
          mutate::vCycleWithNewInitialPartitioning(hg,
                                                   _population.individualAt(mutation_position),
                                                   context), mutation_position);
        break;
      case EvoMutateStrategy::vcycle:
        _population.forceInsertSaveBest(
          mutate::vCycle(hg, _population.individualAt(mutation_position), context),
          mutation_position);
        break;
      case EvoMutateStrategy::single_stable_net:
        _population.forceInsertSaveBest(mutate::removeStableNets(hg,
                                                         _population.individualAt(mutation_position),
                                                         context), mutation_position);
        break;
      case EvoMutateStrategy::population_stable_net:
      //TODO perhaps a forceInsertSaveBest would be more appropriate
        _population.insert(mutate::removePopulationStableNets(hg, _population, context), context);
        break;
      case EvoMutateStrategy::edge_frequency:
        _population.insert(mutate::edgeFrequency(hg, context, _population), context);
        break;
    }
    context.evolutionary.mutate_strategy = original_strategy;
  }

  inline void diversify();

  inline std::chrono::duration<double> measureTime() {
    const HighResClockTimepoint currentTime = std::chrono::high_resolution_clock::now();
    return currentTime - _globalstart;
  }

  HighResClockTimepoint _globalstart;
  int _timelimit;
  Population _population;

};
}  // namespace partition
}  // namespace kahypar
