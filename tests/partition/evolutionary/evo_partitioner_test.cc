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
#include <limits>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "kahypar/application/command_line_options.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/evo_partitioner.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/metrics.h"

using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
class TheEvoPartitioner : public Test {
 public:
  TheEvoPartitioner() :
    context(),
    hypergraph(6, 1, HyperedgeIndexVector { 0, 6 },
               HyperedgeVector { 0, 1, 2, 3, 4, 5 })
  {
    hypergraph.changeK(2);
    parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
    context.partition.k = 2;
    context.partition.epsilon = 0.03;
    context.partition.objective = Objective::cut;
    context.partition.mode = Mode::direct_kway;
    context.local_search.algorithm = RefinementAlgorithm::kway_fm;
    context.partition_evolutionary = true;
    context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
    context.evolutionary.mutate_strategy = EvoMutateStrategy::vcycle;
    context.evolutionary.mutation_chance = 0.2;
    context.evolutionary.diversify_interval = -1;
    context.preprocessing.enable_community_detection = false;
    Timer::instance().clear();
  }
  Context context;

  Hypergraph hypergraph;

 private:
  FRIEND_TEST(TheEvoPartitioner, PicksTheRightStrategy);
};

TEST_F(TheEvoPartitioner, IsCorrectlyDecidingTheActions) {
  EvoPartitioner evo_part(context);


  const int nr_runs = 10;
  float chances[nr_runs];
  Randomize::instance().setSeed(1);
  for (int i = 0; i < nr_runs; ++i) {
    chances[i] = Randomize::instance().getRandomFloat(0, 1);
  }
  Randomize::instance().setSeed(1);
  EvoDecision decision;
  for (int i = 0; i < nr_runs; ++i) {
    decision = evo_part.decideNextMove(context);

    ASSERT_EQ((chances[i] < 0.2), (decision == EvoDecision::mutation));
    ASSERT_EQ((chances[i] >= 0.2), (decision == EvoDecision::combine));
  }
}
TEST_F(TheEvoPartitioner, RespectsLimitsOfTheInitialPopulation) {
  context.partition.quiet_mode = true;
  EvoPartitioner evo_part(context);
  evo_part.generateInitialPopulation(hypergraph, context);
  ASSERT_EQ(evo_part._population.size(), 50);
}

TEST_F(TheEvoPartitioner, ProperlyGeneratesTheInitialPopulation) {
  context.partition.quiet_mode = true;
  context.partition.time_limit = 1;
  context.evolutionary.dynamic_population_size = true;
  context.evolutionary.dynamic_population_amount_of_time = 0.15;


  EvoPartitioner evo_part(context);
  evo_part.generateInitialPopulation(hypergraph, context);
  ASSERT_EQ(evo_part._population.size(), std::min(50.0, std::max(3.0, std::round(context.evolutionary.dynamic_population_amount_of_time
                                                                                 * context.partition.time_limit
                                                                                 / Timer::instance().evolutionaryResult().evolutionary.at(0)))));
}
TEST_F(TheEvoPartitioner, RespectsTheTimeLimit) {
  context.partition.quiet_mode = true;
  context.partition.time_limit = 1;
  context.evolutionary.dynamic_population_size = true;
  context.evolutionary.dynamic_population_amount_of_time = 0.15;


  EvoPartitioner evo_part(context);
  evo_part.partition(hypergraph, context);
  std::vector<double> times = Timer::instance().evolutionaryResult().evolutionary;
  double total_time = Timer::instance().evolutionaryResult().total_evolutionary;
  ASSERT_GT(total_time, context.partition.time_limit);
  ASSERT_LT(total_time - times.at(times.size() - 1), context.partition.time_limit);
}
}  // namespace kahypar
