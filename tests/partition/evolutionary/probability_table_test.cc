/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Robin Andre <robinandre1995@web.de>
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
#include "gmock/gmock.h"

#include "kahypar/partition/context.h"
#include "kahypar/partition/context_enum_classes.h"
#include "kahypar/partition/evolutionary/probability_tables.h"
#include "kahypar/utils/randomize.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
class TheProbabilityTable : public Test {
 public:
  TheProbabilityTable() :
    context() { }

  Context context;
  static constexpr int kNumRuns = 100;
};

TEST_F(TheProbabilityTable, PicksTheCorrectCombineOperation) {
  for (int j = 0; j < 10; ++j) {
    float edge_frequency_chance = static_cast<float>(j)/10;

    context.evolutionary.edge_frequency_chance = edge_frequency_chance;
    context.evolutionary.random_combine_strategy = true;
    Randomize::instance().setSeed(1);
    float chances[kNumRuns];
    for (int i = 0; i < kNumRuns; ++i) {
      chances[i] = Randomize::instance().getRandomFloat(0, 1);
    }
    Randomize::instance().setSeed(1);
    EvoCombineStrategy strat;
    for (int i = 0; i < kNumRuns; ++i) {
      strat = pick::appropriateCombineStrategy(context);
      ASSERT_EQ((chances[i] > edge_frequency_chance), (strat == EvoCombineStrategy::basic));
      ASSERT_EQ((chances[i] <= edge_frequency_chance), (strat == EvoCombineStrategy::edge_frequency));
    }
  }
}
TEST_F(TheProbabilityTable, PicksRandomVcycles) {
  context.evolutionary.random_vcycles = true;
  Randomize::instance().setSeed(1);
  bool chances[kNumRuns];
  for (int i = 0; i < kNumRuns; ++i) {
    chances[i] = Randomize::instance().flipCoin();
  }
  Randomize::instance().setSeed(1);
  EvoMutateStrategy strat;
  for (int i = 0; i < kNumRuns; ++i) {
    strat = pick::appropriateMutateStrategy(context);
    ASSERT_EQ((chances[i] == true), (strat == EvoMutateStrategy::vcycle));
    ASSERT_EQ((chances[i] == false), (strat == EvoMutateStrategy::new_initial_partitioning_vcycle));
  }
}
}  // namespace kahypar
