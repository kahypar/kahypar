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
#include "kahypar-resources/macros.h"
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
    context.partition.mode = Mode::direct_kway;
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


TEST_F(FixedVertex, FlowSnapshotDoesNotContainFixedVertices) {
  // have to set manually, whereas fixed_vertices::partition already does that
  hypergraph->setNodePart(0, 1);
  hypergraph->setNodePart(6, 0);

  // get the entire hypergraph in the snapshot
  context.local_search.hyperflowcutter.flowhypergraph_size_constraint = FlowHypergraphSizeConstraint::part_weight_fraction;
  context.local_search.hyperflowcutter.snapshot_scaling = 1.2;

  context.partition.objective = Objective::km1;
  whfcInterface::FlowHypergraphExtractor extractor(*hypergraph, context);
  whfc::DistanceFromCut dummy_distance_tracker(hypergraph->initialNumNodes());

  std::vector<HyperedgeID> cut_hes;
  for (HyperedgeID e : hypergraph->edges()) {
    if (hypergraph->connectivity(e) > 1) {
      cut_hes.push_back(e);
    }
  }
  auto r = extractor.run(*hypergraph, context, cut_hes, 0, 1, dummy_distance_tracker);

  ASSERT_EQ(extractor.global2local(0), whfc::invalidNode);  // means they were not mapped!
  ASSERT_EQ(extractor.global2local(6), whfc::invalidNode);
  ASSERT_EQ(extractor.flow_hg_builder.nodeWeight(r.source), 1);
  ASSERT_EQ(extractor.flow_hg_builder.nodeWeight(r.target), 1);
}

TEST_F(FixedVertex, AssignmentWithIndividualPartWeights) {
  context.partition.verbose_output = true;
  hypergraph->resetFixedVertices();
  hypergraph->reset();
  hypergraph->setNodeWeight(5, 5);
  hypergraph->setEdgeWeight(2, 3);

  hypergraph->setNodePart(0, 0);
  hypergraph->setFixedVertex(1, 0);
  hypergraph->setNodePart(2, 0);
  hypergraph->setNodePart(3, 0);
  hypergraph->setNodePart(4, 0);
  hypergraph->setNodePart(5, 1);
  hypergraph->setFixedVertex(6, 1);

  context.partition.use_individual_part_weights = true;
  context.partition.max_part_weights[0] = 5;
  context.partition.max_part_weights[1] = 6;
  context.setupPartWeights(hypergraph->totalWeight());

  context.partition.objective = Objective::km1;
  fixed_vertices::partition(*hypergraph, context);

  ASSERT_LE(hypergraph->partWeight(0), context.partition.max_part_weights[0]);
  ASSERT_LE(hypergraph->partWeight(1), context.partition.max_part_weights[1]);
}

}  // namespace kahypar
