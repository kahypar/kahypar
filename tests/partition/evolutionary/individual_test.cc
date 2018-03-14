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
#include <vector>

#include "gmock/gmock.h"

#include "kahypar/datastructure/hypergraph.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/metrics.h"

using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
class AnIndividual : public Test {
 public:
  AnIndividual() :
    individual(),
    hypergraph(4, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 5 },
               HyperedgeVector { 0, 1, 0, 2, 3 }) { hypergraph.changeK(4); }
  Individual individual;

  Hypergraph hypergraph;
};


TEST_F(AnIndividual, IsCorrectlyExtractedFromTheHypergraphForKM1) {
  Context context;
  context.partition.objective = Objective::km1;
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 3);
  individual = Individual(hypergraph, context);
  ASSERT_EQ(individual.partition()[0], 0);
  ASSERT_EQ(individual.partition()[1], 1);
  ASSERT_EQ(individual.partition()[2], 2);
  ASSERT_EQ(individual.partition()[3], 3);
  ASSERT_EQ(individual.cutEdges()[0], 0);
  ASSERT_EQ(individual.cutEdges()[1], 1);
  ASSERT_EQ(individual.cutEdges().size(), 2);
  ASSERT_EQ(individual.strongCutEdges().size(), 3);
  ASSERT_EQ(individual.strongCutEdges()[0], 0);
  ASSERT_EQ(individual.strongCutEdges()[1], 1);
  ASSERT_EQ(individual.strongCutEdges()[2], 1);
  ASSERT_EQ(individual.fitness(), 3);
}
TEST_F(AnIndividual, IsCorrectlyExtractedFromTheHypergraphForCut) {
  Context context;
  context.partition.objective = Objective::cut;
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 2);
  hypergraph.setNodePart(3, 3);
  individual = Individual(hypergraph, context);
  ASSERT_EQ(individual.partition()[0], 0);
  ASSERT_EQ(individual.partition()[1], 1);
  ASSERT_EQ(individual.partition()[2], 2);
  ASSERT_EQ(individual.partition()[3], 3);
  ASSERT_EQ(individual.cutEdges()[0], 0);
  ASSERT_EQ(individual.cutEdges()[1], 1);
  ASSERT_EQ(individual.cutEdges().size(), 2);
  ASSERT_EQ(individual.strongCutEdges().size(), 3);
  ASSERT_EQ(individual.strongCutEdges()[0], 0);
  ASSERT_EQ(individual.strongCutEdges()[1], 1);
  ASSERT_EQ(individual.strongCutEdges()[2], 1);
  ASSERT_EQ(individual.fitness(), 2);
}
}  // namespace kahypar
