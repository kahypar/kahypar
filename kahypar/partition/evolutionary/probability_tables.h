/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2017 Robin Andre <robinandre1995@web.de>
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
#include "kahypar/utils/randomize.h"

namespace kahypar {
namespace pick {
// NOTE: edge-frequency-information will not be picked by the random strategy.
inline static EvoCombineStrategy appropriateCombineStrategy(const Context& context) {
  if (context.evolutionary.random_combine_strategy) {
    const float random_pick = Randomize::instance().getRandomFloat(0, 1);
    if (context.evolutionary.edge_frequency_chance >= random_pick) {
      return EvoCombineStrategy::edge_frequency;
    } else {
      return EvoCombineStrategy::basic;
    }
  } else {
    return context.evolutionary.combine_strategy;
  }
}
inline static EvoMutateStrategy appropriateMutateStrategy(const Context& context) {
  if (context.evolutionary.random_vcycles) {
    if (Randomize::instance().flipCoin()) {
      return EvoMutateStrategy::vcycle;
    } else {
      return EvoMutateStrategy::new_initial_partitioning_vcycle;
    }
  } else {
    return context.evolutionary.mutate_strategy;
  }
}
}  // namespace pick
}  // namespace kahypar
