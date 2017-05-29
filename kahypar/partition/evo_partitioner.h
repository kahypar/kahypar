#pragma once
#include <chrono>
#include "kahypar/datastructure/hypergraph.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/combine.h"

namespace kahypar {
namespace partition {
  enum Decision {
    mutateChoice,
    combineChoice,
    edgeFrequencyChoice,
    crossCombineChoice
  };
class EvoPartitioner {
  public:
  
  EvoPartitioner(Hypergraph &hg, const Context& context) : 
  _globalstart(),
  _timelimit(), 
  _population(hg)
  { 
    _globalstart = std::chrono::high_resolution_clock::now();
    _timelimit = context.evolutionary.time_limit_seconds;
   } 
  
  inline void evo_partition(Hypergraph&hg, Context &context);
  
  private:

  inline Decision decideNextMove(const Context& context);
  inline void performCombine(Hypergraph &hg, Context& context);
  inline void performCrossCombine(Hypergraph &hg, Context& context);
  inline void performMutation(Hypergraph &hg, Context& context);
  inline void performEdgeFrequency(Hypergraph &hg, Context& context);
  inline void diversify();
  inline std::chrono::duration<double> measureTime();
  HighResClockTimepoint _globalstart;
  int _timelimit;
  Population _population;
  int _iteration;
};









inline void EvoPartitioner::evo_partition(Hypergraph& hg, Context &context) {
  std::chrono::duration<double> elapsed_seconds_total = measureTime();
  while(_population.size() < context.evolutionary.population_size && elapsed_seconds_total.count() <= _timelimit) {
    ++_iteration;
    std::cout << std::endl<< std::endl<< std::endl<< std::endl<< std::endl;
    std::cout<<_population.size();
    std::cout << std::endl<< std::endl<< std::endl<< std::endl<< std::endl;
    _population.generateIndividual(context);
    elapsed_seconds_total = measureTime();   
    _population.print(); 
  }

  return;
  while(elapsed_seconds_total.count() <= _timelimit) {
    ++_iteration;
    Decision decision = decideNextMove(context);
    switch(decision) {
      case(mutateChoice) : {
        performMutation(hg, context);
        break;
      } 
      case(edgeFrequencyChoice) : {
        performEdgeFrequency(hg, context);
        break;
      }
      case(crossCombineChoice) : {
        performCrossCombine(hg, context);
        break;
      }
      case(combineChoice) : {
        performCombine(hg, context);
        break;
      }
      default : {
        std::cout << "Error in evo_partitioner.h: Non-covered case in decision making" << std::endl;
        std::exit(EXIT_FAILURE);
      }
    }
    elapsed_seconds_total = measureTime();
  }
}

inline void EvoPartitioner::performMutation(Hypergraph& hg, Context& context) {
  const std::size_t mutationPosition = _population.randomIndividualExcept(_population.best());
  
  switch(context.evolutionary.mutate_strategy) {
    case(MutateStrategy::vcycle_with_new_initial_partitioning):{
      Individual result = mutate::vCycleWithNewInitialPartitioning(hg, _population.individualAt(mutationPosition), context);
      _population.forceInsert(result, mutationPosition);
      break;
    }
    case(MutateStrategy::single_stable_net):{
      Individual result = mutate::stableNetMutate(hg, _population.individualAt(mutationPosition), context);
      _population.forceInsert(result, mutationPosition);
      break;
    }
    case(MutateStrategy::single_stable_net_vcycle):{
      Individual result = mutate::stableNetMutateWithVCycle(hg, _population.individualAt(mutationPosition), context);
      _population.forceInsert(result, mutationPosition);
      break;
    }
  }
  
}

inline void EvoPartitioner::performCombine(Hypergraph& hg, Context& context) {
  const std::pair<Individual, Individual> parents = _population.tournamentSelect();
  switch(context.evolutionary.combine_strategy) {
    case(CombineStrategy::basic): {
      Individual result = combine::partitions(hg, parents, context);
      ASSERT(result.fitness <= parents.first.fitness && result.fitness <= parents.second.fitness);
      _population.insert(result, context);
      break;
    }
    case(CombineStrategy::with_edge_frequency_information): {
      Individual result = combine::edgeFrequencyWithAdditionalPartitionInformation(hg, parents, context, _population);
      _population.insert(result, context);
    }
  }
}
inline void EvoPartitioner::performCrossCombine(Hypergraph &hg, Context& context) {
  Individual result = combine::crossCombine(hg, _population.singleTournamentSelection(), context);
  _population.insert(result, context);
}
inline void EvoPartitioner::performEdgeFrequency(Hypergraph &hg, Context& context) {
  Individual result = combine::edgeFrequency(hg, context, _population);
  _population.insert(result, context);
}
inline std::chrono::duration<double> EvoPartitioner::measureTime() {
  HighResClockTimepoint currentTime = std::chrono::high_resolution_clock::now();
  return currentTime - _globalstart;
}
inline Decision EvoPartitioner::decideNextMove(const Context& context) {
  if(_iteration % context.evolutionary.perform_edge_frequency_interval == 0) {
    return Decision::edgeFrequencyChoice;
  }
  if(Randomize::instance().getRandomFloat(0,1) < context.evolutionary.mutation_chance) {
    return Decision::mutateChoice;
  }
  if(Randomize::instance().getRandomFloat(0,1) < context.evolutionary.cross_combine_chance) {
    return Decision::crossCombineChoice;
  }
  return Decision::combineChoice;
}
} //namespace partition
} //namespace kahypar
