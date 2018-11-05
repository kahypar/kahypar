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
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/metrics.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
class AMutation : public Test {
 public:
  AMutation() :
    context(),
    // hypergraph(6, 3, HyperedgeIndexVector { 0, 3, 6, /*sentinel*/ 8},
    //           HyperedgeVector {0,1,2,3,4,5,4,5}) {hypergraph.changeK(2); }
    hypergraph(6, 2, HyperedgeIndexVector { 0, 6, 8 },
               HyperedgeVector { 0, 1, 2, 3, 4, 5, 2, 3 }) { hypergraph.changeK(2); }
  Context context;

  Hypergraph hypergraph;
};

TEST_F(AMutation, IsPerformingVcyclesCorrectly) {
  parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 2;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.partition.mode = Mode::direct_kway;
  context.partition_evolutionary = true;
  context.local_search.algorithm = RefinementAlgorithm::do_nothing;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
  context.evolutionary.mutate_strategy = EvoMutateStrategy::vcycle;
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  Individual ind1 = Individual(hypergraph, context);
  Individual ind2 = mutate::vCycle(hypergraph, ind1, context);

  ASSERT_EQ(ind2.partition().at(0), ind2.partition().at(2));
  ASSERT_EQ(ind2.partition().at(0), ind2.partition().at(1));
  ASSERT_EQ(ind2.partition().at(3), ind2.partition().at(4));
  ASSERT_EQ(ind2.partition().at(3), ind2.partition().at(5));
}
TEST_F(AMutation, IsPerformingVcyclesNewIPCorrectly) {
  parseIniToContext(context, "../../../../config/km1_direct_kway_gecco18.ini");
  context.partition.k = 2;
  context.partition.epsilon = 0.03;
  context.partition.objective = Objective::cut;
  context.partition.mode = Mode::direct_kway;
  context.local_search.algorithm = RefinementAlgorithm::do_nothing;
  context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
  context.evolutionary.mutate_strategy = EvoMutateStrategy::new_initial_partitioning_vcycle;
  context.partition_evolutionary = true;
  hypergraph.reset();
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  Individual ind1 = Individual(hypergraph, context);
  Individual ind2 = mutate::vCycleWithNewInitialPartitioning(hypergraph, ind1, context);

  ASSERT_EQ(ind2.partition().at(3), ind2.partition().at(2));
}
}  // namespace kahypar
