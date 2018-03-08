#pragma once
#include "kahypar/utils/randomize.h"
namespace kahypar {
namespace pick {

  // NOTE: edge-frequency-information will not be picked by the random strategy.
  inline static EvoCombineStrategy appropriateCombineStrategy(const Context& context) {
    if(context.evolutionary.random_combine_strategy) {
      float random_pick = Randomize::instance().getRandomFloat(0,1);
      if(context.evolutionary.edge_frequency_chance >= random_pick) {
        return EvoCombineStrategy::edge_frequency;
      }
      else {
        return EvoCombineStrategy::basic;
      }
    }
    else {
      return context.evolutionary.combine_strategy;
    }
  }
  inline static EvoMutateStrategy appropriateMutateStrategy(const Context& context) {

    if(context.evolutionary.random_vcycles) {
      if(Randomize::instance().flipCoin()) {
        return EvoMutateStrategy::vcycle;
      } 
      
      else {
        return EvoMutateStrategy::new_initial_partitioning_vcycle;
      }
    }
    else {
      return context.evolutionary.mutate_strategy;
    }
  }
  
  }
}
