/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@gmx.net>
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

#include "kahypar/definitions.h"
#include "kahypar/kahypar.h"
#include "kahypar/macros.h"
#include "kahypar/partition/fixed_vertices.h"

using ::testing::Test;
using ::testing::Eq;
using ::testing::AnyOf;
using ::testing::AllOf;

namespace kahypar {
class FixedVertex : public Test {
 public:
  static constexpr bool debug = false;

  FixedVertex() :
    hypergraph(new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                              HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })),
    context() {
    context.partition.objective = Objective::cut;
    context.coarsening.max_allowed_node_weight = 5;
    context.partition.graph_filename = "PartitionerTest.hgr";
    context.partition.graph_partition_filename = "PartitionerTest.hgr.part.2.KaHyPar";
    context.partition.epsilon = 0.15;
    context.partition.k = 2;
    context.partition.rb_lower_k = 0;
    context.partition.rb_upper_k = context.partition.k - 1;
    context.partition.perfect_balance_part_weights.push_back(ceil(
                                                               7 / static_cast<double>(context.partition.k)));
    context.partition.perfect_balance_part_weights.push_back(ceil(
                                                               7 / static_cast<double>(context.partition.k)));

    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[0]);
    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[1]);
    kahypar::Randomize::instance().setSeed(context.partition.seed);

    hypergraph->setFixedVertex(0, 1);
    hypergraph->setNodePart(1, 0);
    hypergraph->setNodePart(2, 0);
    hypergraph->setNodePart(3, 1);
    hypergraph->setNodePart(4, 1);
    hypergraph->setNodePart(5, 1);
    hypergraph->setFixedVertex(6, 0);
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Context context;
};

TEST_F(FixedVertex, AssignmentToBestPartitionIfCutIsObjective) {
  context.partition.objective = Objective::cut;
  fixed_vertices::partition(*hypergraph, context);
  ASSERT_THAT(true,
              AllOf(
                hypergraph->partID(0) == 1,
                hypergraph->partID(1) == 1,
                hypergraph->partID(2) == 1,
                hypergraph->partID(3) == 0,
                hypergraph->partID(4) == 0,
                hypergraph->partID(5) == 0,
                hypergraph->partID(6) == 0));
}

TEST_F(FixedVertex, AssignmentToBestPartitionIfKm1IsObjective) {
  context.partition.objective = Objective::km1;
  fixed_vertices::partition(*hypergraph, context);
  ASSERT_THAT(true,
              AllOf(
                hypergraph->partID(0) == 1,
                hypergraph->partID(1) == 1,
                hypergraph->partID(2) == 1,
                hypergraph->partID(3) == 0,
                hypergraph->partID(4) == 0,
                hypergraph->partID(5) == 0,
                hypergraph->partID(6) == 0));
}
}  // namespace kahypar
