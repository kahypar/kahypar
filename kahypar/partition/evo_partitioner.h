#pragma once
#include <chrono>
#include "kahypar/datastructure/hypergraph.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/combine.h"
#include "kahypar/partition/context_enum_classes.h"
namespace kahypar {
namespace partition {

  enum class Decision  {
    normal,
    mutation,
    combine,
    edgeFrequency,
    crossCombine
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
  inline void performCombine(Hypergraph &hg, const Context& context);
  inline void performCrossCombine(Hypergraph &hg, const Context& context);
  inline void performMutation(Hypergraph &hg, const Context& context);
  inline void performEdgeFrequency(Hypergraph &hg, const Context& context);
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
    _population.generateIndividual(hg, context);
    elapsed_seconds_total = measureTime();   
    _population.print(); 
  }
  performEdgeFrequency(hg, context);
  return;
  while(elapsed_seconds_total.count() <= _timelimit) {
    ++_iteration;
    Decision decision = decideNextMove(context);
    switch(decision) {
      case(Decision::mutation) : {
        performMutation(hg, context);
        break;
      } 
      case(Decision::edgeFrequency) : {
        performEdgeFrequency(hg, context);
        break;
      }
      case(Decision::crossCombine) : {
        performCrossCombine(hg, context);
        break;
      }
      case(Decision::combine) : {
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

inline void EvoPartitioner::performMutation(Hypergraph& hg,const Context& context) {
  
  const size_t mutationPosition = _population.randomIndividualExcept(_population.best());
  
  switch(context.evolutionary.mutate_strategy) {
    case(MutateStrategy::vcycle_with_new_initial_partitioning):{
      _population.forceInsert(mutate::vCycleWithNewInitialPartitioning(hg, _population.individualAt(mutationPosition), context), mutationPosition);
      break;
    }
    case(MutateStrategy::single_stable_net):{
      _population.forceInsert(mutate::stableNetMutate(hg, _population.individualAt(mutationPosition), context), mutationPosition);
      break;
    }
    case(MutateStrategy::single_stable_net_vcycle):{   
      _population.forceInsert(mutate::stableNetMutateWithVCycle(hg, _population.individualAt(mutationPosition), context), mutationPosition);
      break;
    }
  }
  
}

inline void EvoPartitioner::performCombine(Hypergraph& hg,const Context& context) {
  const std::pair<Individual, Individual> parents = _population.tournamentSelect();
  switch(context.evolutionary.combine_strategy) {
    case(CombineStrategy::basic): {
      //ASSERT(result.fitness <= parents.first.fitness && result.fitness <= parents.second.fitness);
      _population.insert(combine::partitions(hg, parents, context), context);
      break;
    }
    case(CombineStrategy::with_edge_frequency_information): {
      _population.insert(combine::edgeFrequencyWithAdditionalPartitionInformation(hg, parents, context, _population), context);
    }
  }
}
inline void EvoPartitioner::performCrossCombine(Hypergraph &hg,const Context& context) {
  _population.insert(combine::crossCombine(hg, _population.singleTournamentSelection(), context), context);
}
inline void EvoPartitioner::performEdgeFrequency(Hypergraph &hg,const Context& context) {
  _population.insert(combine::edgeFrequency(hg, context, _population), context);
}
inline std::chrono::duration<double> EvoPartitioner::measureTime() {
  HighResClockTimepoint currentTime = std::chrono::high_resolution_clock::now();
  return currentTime - _globalstart;
}
inline Decision EvoPartitioner::decideNextMove(const Context& context) {
  if(_iteration % context.evolutionary.perform_edge_frequency_interval == 0) {
    return Decision::edgeFrequency;
  }
  if(Randomize::instance().getRandomFloat(0,1) < context.evolutionary.mutation_chance) {
    return Decision::mutation;
  }
  if(Randomize::instance().getRandomFloat(0,1) < context.evolutionary.cross_combine_chance) {
    return Decision::crossCombine;
  }
  return Decision::combine;
}
} //namespace partition
} //namespace kahypar
