/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
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

#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "gmock/gmock.h"

#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/policies/ip_start_node_selection_policy.h"
#include "kahypar/partition/initial_partitioning/random_initial_partitioner.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/utils/randomize.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
void initializeContext(Context& context, PartitionID k,
                       HypernodeWeight hypergraph_weight) {
  context.initial_partitioning.k = k;
  context.partition.k = k;
  context.partition.epsilon = 0.05;
  context.initial_partitioning.refinement = false;
  context.initial_partitioning.nruns = 20;
  context.initial_partitioning.upper_allowed_partition_weight.resize(
    context.initial_partitioning.k);
  context.initial_partitioning.perfect_balance_partition_weight.resize(
    context.initial_partitioning.k);
  context.partition.max_part_weights.resize(context.partition.k);
  context.partition.perfect_balance_part_weights.resize(context.partition.k);
  for (int i = 0; i < context.initial_partitioning.k; i++) {
    context.initial_partitioning.perfect_balance_partition_weight[i] = ceil(
      hypergraph_weight
      / static_cast<double>(context.initial_partitioning.k));
    context.initial_partitioning.upper_allowed_partition_weight[i] = ceil(
      hypergraph_weight
      / static_cast<double>(context.initial_partitioning.k))
                                                                     * (1.0 + context.partition.epsilon);
  }
  for (int i = 0; i < context.initial_partitioning.k; ++i) {
    context.partition.perfect_balance_part_weights[i] =
      context.initial_partitioning.perfect_balance_partition_weight[i];
    context.partition.max_part_weights[i] = context.initial_partitioning.upper_allowed_partition_weight[i];
  }
}

void generateRandomFixedVertices(Hypergraph& hypergraph,
                                 const double fixed_vertices_percentage,
                                 const PartitionID k) {
  for (const HypernodeID& hn : hypergraph.nodes()) {
    int p = Randomize::instance().getRandomInt(0, 100);
    if (p < fixed_vertices_percentage * 100) {
      PartitionID part = Randomize::instance().getRandomInt(0, k - 1);
      hypergraph.setFixedVertex(hn, part);
    }
  }
}

class ARandomBisectionInitialPartitionerTest : public Test {
 public:
  ARandomBisectionInitialPartitionerTest() :
    partitioner(nullptr),
    hypergraph(nullptr),
    context() {
    std::string coarse_graph_filename = "test_instances/test_instance.hgr";

    HypernodeID num_hypernodes;
    HyperedgeID num_hyperedges;
    HyperedgeIndexVector index_vector;
    HyperedgeVector edge_vector;
    HyperedgeWeightVector hyperedge_weights;
    HypernodeWeightVector hypernode_weights;

    PartitionID k = 2;
    io::readHypergraphFile(coarse_graph_filename, num_hypernodes,
                           num_hyperedges, index_vector, edge_vector, &hyperedge_weights,
                           &hypernode_weights);
    hypergraph = std::make_shared<Hypergraph>(num_hypernodes, num_hyperedges,
                                              index_vector, edge_vector, k, &hyperedge_weights,
                                              &hypernode_weights);

    HypernodeWeight hypergraph_weight = 0;
    for (const HypernodeID& hn : hypergraph->nodes()) {
      hypergraph_weight += hypergraph->nodeWeight(hn);
    }
    initializeContext(context, k, hypergraph_weight);

    partitioner = std::make_shared<RandomInitialPartitioner>(*hypergraph, context);
  }

  std::shared_ptr<RandomInitialPartitioner> partitioner;
  std::shared_ptr<Hypergraph> hypergraph;
  Context context;
};

class AKWayRandomInitialPartitionerTest : public Test {
 public:
  AKWayRandomInitialPartitionerTest() :
    partitioner(nullptr),
    hypergraph(nullptr),
    context() { }

  void initializePartitioning(PartitionID k) {
    std::string coarse_graph_filename = "test_instances/test_instance.hgr";

    HypernodeID num_hypernodes;
    HyperedgeID num_hyperedges;
    HyperedgeIndexVector index_vector;
    HyperedgeVector edge_vector;
    HyperedgeWeightVector hyperedge_weights;
    HypernodeWeightVector hypernode_weights;

    io::readHypergraphFile(coarse_graph_filename, num_hypernodes,
                           num_hyperedges, index_vector, edge_vector, &hyperedge_weights,
                           &hypernode_weights);
    hypergraph = std::make_shared<Hypergraph>(num_hypernodes, num_hyperedges,
                                              index_vector, edge_vector, k, &hyperedge_weights,
                                              &hypernode_weights);

    HypernodeWeight hypergraph_weight = 0;
    for (const HypernodeID& hn : hypergraph->nodes()) {
      hypergraph_weight += hypergraph->nodeWeight(hn);
    }
    initializeContext(context, k, hypergraph_weight);

    partitioner = std::make_shared<RandomInitialPartitioner>(*hypergraph, context);
  }

  std::shared_ptr<RandomInitialPartitioner> partitioner;
  std::shared_ptr<Hypergraph> hypergraph;
  Context context;
};

TEST_F(ARandomBisectionInitialPartitionerTest, HasValidImbalance) {
  partitioner->partition();

  ASSERT_LE(metrics::imbalance(*hypergraph, context),
            context.partition.epsilon);
}

TEST_F(ARandomBisectionInitialPartitionerTest, LeavesNoHypernodeUnassigned) {
  partitioner->partition();

  for (const HypernodeID& hn : hypergraph->nodes()) {
    ASSERT_NE(hypergraph->partID(hn), -1);
  }
}

TEST_F(ARandomBisectionInitialPartitionerTest, SetCorrectFixedVertexPart) {
  generateRandomFixedVertices(*hypergraph, 0.1, 2);

  partitioner->partition();

  for (const HypernodeID& hn : hypergraph->fixedVertices()) {
    ASSERT_EQ(hypergraph->partID(hn), hypergraph->fixedVertexPartID(hn));
  }
}

TEST_F(AKWayRandomInitialPartitionerTest, HasValidImbalance) {
  PartitionID k = 4;
  initializePartitioning(k);
  partitioner->partition();

  ASSERT_LE(metrics::imbalance(*hypergraph, context),
            context.partition.epsilon);
}

TEST_F(AKWayRandomInitialPartitionerTest, HasNoSignificantLowPartitionWeights) {
  PartitionID k = 4;
  initializePartitioning(k);
  partitioner->partition();

  // Upper bounds of maximum partition weight should not be exceeded.
  HypernodeWeight heaviest_part = 0;
  for (PartitionID k = 0; k < context.initial_partitioning.k; k++) {
    if (heaviest_part < hypergraph->partWeight(k)) {
      heaviest_part = hypergraph->partWeight(k);
    }
  }

  // No partition weight should fall below under "lower_bound_factor"
  // percent of the heaviest partition weight.
  double lower_bound_factor = 50.0;
  for (PartitionID k = 0; k < context.initial_partitioning.k; k++) {
    ASSERT_GE(hypergraph->partWeight(k),
              (lower_bound_factor / 100.0) * heaviest_part);
  }
}

TEST_F(AKWayRandomInitialPartitionerTest, LeavesNoHypernodeUnassigned) {
  PartitionID k = 4;
  initializePartitioning(k);
  partitioner->partition();

  for (const HypernodeID& hn : hypergraph->nodes()) {
    ASSERT_NE(hypergraph->partID(hn), -1);
  }
}

TEST_F(AKWayRandomInitialPartitionerTest, SetCorrectFixedVertexPart) {
  PartitionID k = 4;
  initializePartitioning(k);
  generateRandomFixedVertices(*hypergraph, 0.1, 4);
  ASSERT_GE(hypergraph->numFixedVertices(), 0);

  partitioner->partition();

  for (const HypernodeID& hn : hypergraph->fixedVertices()) {
    ASSERT_EQ(hypergraph->partID(hn), hypergraph->fixedVertexPartID(hn));
  }
}
}  // namespace kahypar
