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
#include "kahypar/partition/evolutionary/combine.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/metrics.h"
#include "tests/end_to_end/kahypar_test_fixtures.h"


using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
class ACombine : public Test {
 public:
  ACombine() :
    population(),
    context(),
    hypergraph(8, 7, HyperedgeIndexVector { 0, 2, 4, 6, 8, 10, 14  /*sentinel*/, 18 },
               HyperedgeVector { 0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7 }) { hypergraph.changeK(2); }
  Population population;
  Context context;

  Hypergraph hypergraph;
};
std::vector<PartitionID> parent1 { 0, 1, 0, 1, 0, 1, 0, 1 };
std::vector<PartitionID> parent2 { 0, 1, 1, 0, 0, 1, 1, 0 };
TEST_F(ACombine, RespectsItsParents) {
  parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 2;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.partition.mode = Mode::direct_kway;
  context.local_search.algorithm = RefinementAlgorithm::do_nothing;
  context.partition.quiet_mode = true;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
  context.partition_evolutionary = true;
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 0);
  hypergraph.setNodePart(7, 1);
  Individual ind1 = Individual(hypergraph, context);
  population.forceInsert(std::move(ind1), 0);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 0);
  hypergraph.setNodePart(7, 1);
  ind1 = Individual(hypergraph, context);
  population.forceInsert(std::move(ind1), 1);
  Individual result = combine::usingTournamentSelection(hypergraph, context, population);
  result.printDebug();
  ASSERT_EQ(result.fitness(), 6);
}
TEST_F(ACombine, TakesTheBetterParent) {
  parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 2;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::km1;
  context.partition.mode = Mode::direct_kway;
  context.local_search.algorithm = RefinementAlgorithm::do_nothing;
  context.partition.quiet_mode = true;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
  context.partition_evolutionary = true;
  population.generateIndividual(hypergraph, context);
  population.generateIndividual(hypergraph, context);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 0);
  hypergraph.setNodePart(7, 1);
  Individual ind1 = Individual(hypergraph, context);
  population.forceInsert(std::move(ind1), 0);
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 0);
  hypergraph.setNodePart(6, 1);
  hypergraph.setNodePart(7, 1);
  ind1 = Individual(hypergraph, context);
  population.forceInsert(std::move(ind1), 1);
  Individual result = combine::usingTournamentSelection(hypergraph, context, population);
  ASSERT_EQ(result.fitness(), 2);
}
}  // namespace kahypar
