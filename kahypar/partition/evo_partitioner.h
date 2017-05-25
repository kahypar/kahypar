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
    _timelimit = context.evolutionary.timeLimitSeconds;
   } 
  
  inline void evo_partition(Hypergraph&hg, Context &context);
  
  private:

  inline Decision decideNextMove(const Context& context);
  inline void performCombine(const Context& context);
  inline void performCrossCombine(const Context& context);
  inline void performMutation(const Context& context);
  inline void performEdgeFrequency(const Context& context);
  inline void diversify();
  inline std::chrono::duration<double> measureTime();
  HighResClockTimepoint _globalstart;
  int _timelimit;
  Population _population;
  int _iteration;
};









inline void EvoPartitioner::evo_partition(Hypergraph& hg, Context &context) {
  std::chrono::duration<double> elapsed_seconds_total = measureTime();
  
  
  while(_population.size() < context.evolutionary.populationSize && elapsed_seconds_total.count() <= _timelimit) {
    ++_iteration;
    _population.generateIndividual(context);
    elapsed_seconds_total = measureTime();    
  }
  
  while(elapsed_seconds_total.count() <= _timelimit) {
    ++_iteration;
    Decision decision = decideNextMove(context);
    switch(decision) {
      case(mutateChoice) : {
        performMutation(context);
        break;
      } 
      case(edgeFrequencyChoice) : {
        performEdgeFrequency(context);
        break;
      }
      case(crossCombineChoice) : {
        performCrossCombine(context);
        break;
      }
      case(combineChoice) : {
        performCombine(context);
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

inline void EvoPartitioner::performMutation(const Context& context) {
  const std::size_t mutationPosition = _population.randomIndividualExcept(_population.best());
  
  switch(context.evolutionary.mutateStrategy) {
    case(MutateStrategy::vcycle_with_new_initial_partitioning):{
      Individual result = mutate::vCycleWithNewInitialPartitioning(_population.individualAt(mutationPosition));
      _population.forceInsert(result, mutationPosition);
      break;
    }
    case(MutateStrategy::single_stable_net):{
      Individual result = mutate::stableNetMutate(_population.individualAt(mutationPosition));
      _population.forceInsert(result, mutationPosition);
      break;
    }
    case(MutateStrategy::single_stable_net_vcycle):{
      Individual result = mutate::stableNetMutateWithVCycle(_population.individualAt(mutationPosition));
      _population.forceInsert(result, mutationPosition);
      break;
    }
  }
  
}

inline void EvoPartitioner::performCombine(const Context& context) {
  switch(context.evolutionary.combineStrategy) {
    case(CombineStrategy::basic): {
      Individual result = combine::partitions(context);
      _population.insert(result, context);
      break;
    }
    case(CombineStrategy::with_edge_frequency_information): {
      Individual result = combine::edgeFrequencyWithAdditionalPartitionInformation(context);
    }
  }
}
inline void EvoPartitioner::performCrossCombine(const Context& context) {
 //TODO stub
}
inline void EvoPartitioner::performEdgeFrequency(const Context& context) {
  //TODO stub
}
inline std::chrono::duration<double> EvoPartitioner::measureTime() {
  HighResClockTimepoint currentTime = std::chrono::high_resolution_clock::now();
  return currentTime - _globalstart;
}
inline Decision EvoPartitioner::decideNextMove(const Context& context) {
  if(_iteration % context.evolutionary.performEdgeFrequencyInterval == 0) {
    return Decision::edgeFrequencyChoice;
  }
  if(Randomize::instance().getRandomFloat(0,1) < context.evolutionary.mutationChance) {
    return Decision::mutateChoice;
  }
  if(Randomize::instance().getRandomFloat(0,1) < context.evolutionary.crossCombineChance) {
    return Decision::crossCombineChoice;
  }
  return Decision::combineChoice;
}
} //namespace partition
} //namespace kahypar
