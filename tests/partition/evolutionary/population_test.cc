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
#include "kahypar/kahypar.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/metrics.h"
#include "tests/end_to_end/kahypar_test_fixtures.h"


using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
class APopulation : public Test {
 public:
  APopulation() :
    population(),
    context(),
    hypergraph(8, 5, HyperedgeIndexVector { 0, 2, 4, 7, 10,  /*sentinel*/ 15 },
              HyperedgeVector { 0, 1, 4, 5, 1, 5, 6, 3, 6, 7, 0, 1, 2, 4, 5 }) { 
    hypergraph.changeK(4);
    context.partition.quiet_mode = true;
  }
  Population population;
  Context context;

  Hypergraph hypergraph;
};

TEST_F(APopulation, IsCorrectlyGeneratingIndividuals) {
  parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 8;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.partition.mode = Mode::direct_kway;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm;
  context.partition.graph_filename = "../../../../tests/partition/evolutionary/TestHypergraph";
  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));
  population.generateIndividual(hypergraph, context);
  population.individualAt(0).printDebug();
  ASSERT_EQ(population.size(), 1);
  population.generateIndividual(hypergraph, context);
  ASSERT_EQ(population.size(), 2);
}
TEST_F(APopulation, IsCorrectlyReplacingWithDiverseStrategy) {
  parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 4;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.partition.mode = Mode::direct_kway;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 3);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 2);
  hypergraph.setNodePart(7, 3);
  Individual ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 0);
  ASSERT_EQ(population.individualAt(0).fitness(), 5);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 2);
  hypergraph.setNodePart(4, 3);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 1);
  ASSERT_EQ(population.individualAt(1).fitness(), 4);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 2);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 3);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 2);
  ASSERT_EQ(population.individualAt(2).fitness(), 4);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 2);
  hypergraph.setNodePart(5, 2);
  hypergraph.setNodePart(6, 3);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  ASSERT_EQ(ind1.fitness(), 3);
  ASSERT_EQ(population.difference(ind1, 0, false), 2);
  ASSERT_EQ(population.difference(ind1, 1, false), 1);
  ASSERT_EQ(population.difference(ind1, 2, false), 1);
  population.insert(std::move(ind1), context);
  ASSERT_EQ(population.individualAt(1).fitness(), 3);
  ASSERT_EQ(population.best(), 1);
}
TEST_F(APopulation, IsCorrectlyReplacingWithStrongDiverseStrategy) {
  parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 4;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.partition.mode = Mode::direct_kway;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::strong_diverse;
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 3);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 2);
  hypergraph.setNodePart(7, 3);
  Individual ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 0);
  ASSERT_EQ(population.individualAt(0).fitness(), 6);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 2);
  hypergraph.setNodePart(4, 3);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 1);
  ASSERT_EQ(population.individualAt(1).fitness(), 7);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 2);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 3);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 2);
  ASSERT_EQ(population.individualAt(2).fitness(), 6);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 2);
  hypergraph.setNodePart(5, 2);
  hypergraph.setNodePart(6, 3);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  ASSERT_EQ(ind1.fitness(), 5);
  ASSERT_EQ(population.difference(population.individualAt(0), 1, true), 3);
  ASSERT_EQ(population.difference(population.individualAt(0), 2, true), 2);
  ASSERT_EQ(population.difference(ind1, 0, true), 3);
  ASSERT_EQ(population.difference(population.individualAt(1), 2, true), 5);
  ASSERT_EQ(population.difference(ind1, 1, true), 4);
  ASSERT_EQ(population.difference(ind1, 2, true), 1);
  population.insert(std::move(ind1), context);
  ASSERT_EQ(population.individualAt(2).fitness(), 5);
  ASSERT_EQ(population.best(), 2);

  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 2);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 3);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();

  population.insert(std::move(ind1), context);
  ASSERT_NE(population.individualAt(2).fitness(), 6);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 2);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 3);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ASSERT_EQ(population.difference(ind1, 0, true), 0);
}
TEST_F(APopulation, IsPerformingTournamentSelection) {
  parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 4;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.partition.mode = Mode::direct_kway;
  context.local_search.algorithm = RefinementAlgorithm::kway_fm;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 3);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 2);
  hypergraph.setNodePart(7, 3);
  Individual ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 0);
  ASSERT_EQ(population.individualAt(0).fitness(), 6);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 2);
  hypergraph.setNodePart(4, 3);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 1);
  ASSERT_EQ(population.individualAt(1).fitness(), 7);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 2);
  hypergraph.setNodePart(5, 2);
  hypergraph.setNodePart(6, 3);
  hypergraph.setNodePart(7, 3);
  ind1 = Individual(hypergraph, context);
  ind1.printDebug();
  population.forceInsert(std::move(ind1), 2);
  ASSERT_EQ(population.individualAt(2).fitness(), 5);
  for (int i = 0; i < 10; ++i) {
    ASSERT_LT(population.singleTournamentSelection().fitness(), 7);
  }
}
}  // namespace kahypar
