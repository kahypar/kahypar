#pragma once
#include "kahypar/utils/randomize.h"
namespace kahypar {
namespace pick {

  // NOTE: edge-frequency-information will not be picked by the random strategy.
  inline static EvoCombineStrategy appropriateCombineStrategy(const Context& context) {
    if(context.evolutionary.random_combine_strategy) {
      int random_pick = Randomize::instance().getRandomInt(0,1);
      return static_cast<EvoCombineStrategy>(random_pick);
    }
    else {
      return context.evolutionary.combine_strategy;
    }
  }
  inline static EvoMutateStrategy appropriateMutateStrategy(const Context& context) {
    if(context.evolutionary.random_mutate_strategy) {
      int random_pick = Randomize::instance().getRandomInt(0,3);
      return static_cast<EvoMutateStrategy>(random_pick);
    }
    else {
      return context.evolutionary.mutate_strategy;
    }
  }
  inline static EvoCrossCombineStrategy appropriateCrossCombineStrategy(const Context& context) {
    if(context.evolutionary.random_cross_combine_strategy) {
      int random_pick = Randomize::instance().getRandomInt(0,4);
      return static_cast<EvoCrossCombineStrategy>(random_pick);
    }
    else {
      return context.evolutionary.cross_combine_strategy;
    }
  }
  }
}
