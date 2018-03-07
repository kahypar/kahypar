#include "gmock/gmock.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/context_enum_classes.h"
#include "kahypar/partition/evolutionary/probability_tables.h"
#include "kahypar/utils/randomize.h"
using ::testing::Eq;
using ::testing::Test;

namespace kahypar {

  class TheProbabilityTable : public Test {
    public :
    TheProbabilityTable() :
      context() {}
    
    Context context;

  };

  TEST_F(TheProbabilityTable, PicksTheCorrectCombineOperation) {
    for(float j = 0; j < 1; j = j + 0.1) {
      float edge_frequency_chance = j;
      const int nr_runs = 100;
      context.evolutionary.edge_frequency_chance = edge_frequency_chance;
      context.evolutionary.random_combine_strategy = true;
      Randomize::instance().setSeed(1);
      float chances[nr_runs];
      for(int i = 0; i < nr_runs; ++i) {
        chances[i] =  Randomize::instance().getRandomFloat(0,1);
      }
      Randomize::instance().setSeed(1);
      EvoCombineStrategy strat;
      for(int i = 0; i < nr_runs; ++i) {
        strat = pick::appropriateCombineStrategy(context);
        ASSERT_EQ((chances[i] > edge_frequency_chance),(strat == EvoCombineStrategy::basic));
        ASSERT_EQ((chances[i] <= edge_frequency_chance),(strat == EvoCombineStrategy::edge_frequency));
      }
    }
  }
  TEST_F(TheProbabilityTable, PicksRandomVcycles) {
    const int nr_runs = 100;
    context.evolutionary.random_vcycles = true;
    Randomize::instance().setSeed(1);
    bool chances[nr_runs];
    for(int i = 0; i < nr_runs; ++i) {
      chances[i] =  Randomize::instance().flipCoin();
    }
    Randomize::instance().setSeed(1);
    EvoMutateStrategy strat;
    for(int i = 0; i < nr_runs; ++i) {
      strat = pick::appropriateMutateStrategy(context);
      ASSERT_EQ((chances[i] == true),(strat == EvoMutateStrategy::vcycle));
      ASSERT_EQ((chances[i] == false),(strat == EvoMutateStrategy::new_initial_partitioning_vcycle));
    }
    
  }
}


