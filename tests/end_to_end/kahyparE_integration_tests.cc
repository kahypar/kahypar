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

#include "gmock/gmock.h"

#include "kahypar/application/command_line_options.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/kahypar.h"
#include "kahypar/partition/evolutionary/combine.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/population.h"
using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
class KaHyParE : public Test {
 public:
  KaHyParE() :
    context() { }
  Context context;
};
TEST_F(KaHyParE, MeasuresTimeForEveryAction) {
  parseIniToContext(context, "../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 2;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.partition.mode = Mode::direct_kway;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
  context.partition.quiet_mode = true;
  context.partition_evolutionary = true;
  context.partition.graph_filename = "../../../tests/partition/evolutionary/TestHypergraph";
  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  Population population;
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);

  context.evolutionary.combine_strategy = EvoCombineStrategy::basic;
  Individual temporary_individual = combine::usingTournamentSelection(hypergraph, context, population);
  hypergraph.reset();
  hypergraph.setPartition(temporary_individual.partition());
  ASSERT_LT(metrics::imbalance(hypergraph, context), 0.03);
  ASSERT_EQ(Timer::instance().evolutionaryResult().evolutionary.size(), 1);

  context.evolutionary.combine_strategy = EvoCombineStrategy::edge_frequency;
  temporary_individual = combine::edgeFrequency(hypergraph, context, population);
  hypergraph.reset();
  hypergraph.setPartition(temporary_individual.partition());
  ASSERT_LT(metrics::imbalance(hypergraph, context), 0.03);
  ASSERT_EQ(Timer::instance().evolutionaryResult().evolutionary.size(), 2);

  context.evolutionary.mutate_strategy = EvoMutateStrategy::new_initial_partitioning_vcycle;

  temporary_individual = partition::mutate::vCycleWithNewInitialPartitioning(hypergraph, population.individualAt(population.randomIndividual()), context);
  hypergraph.reset();
  hypergraph.setPartition(temporary_individual.partition());
  ASSERT_LT(metrics::imbalance(hypergraph, context), 0.03);
  ASSERT_EQ(Timer::instance().evolutionaryResult().evolutionary.size(), 3);

  context.evolutionary.mutate_strategy = EvoMutateStrategy::vcycle;

  temporary_individual = partition::mutate::vCycle(hypergraph, population.individualAt(population.randomIndividual()), context);
  hypergraph.reset();
  hypergraph.setPartition(temporary_individual.partition());
  ASSERT_LT(metrics::imbalance(hypergraph, context), 0.03);
  ASSERT_EQ(Timer::instance().evolutionaryResult().evolutionary.size(), 4);
}
}  // namespace kahypar
