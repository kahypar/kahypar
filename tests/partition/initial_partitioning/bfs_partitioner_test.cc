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
#include "kahypar/partition/initial_partitioning/bfs_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/policies/ip_start_node_selection_policy.h"
#include "kahypar/partition/metrics.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
using TestStartNodeSelectionPolicy = BFSStartNodeSelectionPolicy<false>;

void initializeContext(Context& context, PartitionID k,
                       HypernodeWeight hypergraph_weight) {
  context.initial_partitioning.k = k;
  context.partition.k = k;
  context.partition.epsilon = 0.05;
  context.initial_partitioning.unassigned_part = 1;
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

class ABFSBisectionInitialPartioner : public Test {
 public:
  ABFSBisectionInitialPartioner() :
    partitioner(nullptr),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    context() {
    PartitionID k = 2;
    initializeContext(context, k, 7);

    partitioner = std::make_shared<BFSInitialPartitioner<TestStartNodeSelectionPolicy> >(
      hypergraph, context);
  }

  std::shared_ptr<BFSInitialPartitioner<TestStartNodeSelectionPolicy> > partitioner;
  Hypergraph hypergraph;
  Context context;
};

class AKWayBFSInitialPartitioner : public Test {
 public:
  AKWayBFSInitialPartitioner() :
    partitioner(nullptr),
    hypergraph(nullptr),
    context() {
    std::string coarse_graph_filename =
      "test_instances/test_instance.hgr";

    HypernodeID num_hypernodes;
    HyperedgeID num_hyperedges;
    HyperedgeIndexVector index_vector;
    HyperedgeVector edge_vector;
    HyperedgeWeightVector hyperedge_weights;
    HypernodeWeightVector hypernode_weights;

    PartitionID k = 4;
    io::readHypergraphFile(
      coarse_graph_filename,
      num_hypernodes, num_hyperedges, index_vector, edge_vector,
      &hyperedge_weights, &hypernode_weights);
    hypergraph = std::make_shared<Hypergraph>(num_hypernodes, num_hyperedges,
                                              index_vector, edge_vector, k, &hyperedge_weights,
                                              &hypernode_weights);

    HypernodeWeight hypergraph_weight = 0;
    for (const HypernodeID& hn : hypergraph->nodes()) {
      hypergraph_weight += hypergraph->nodeWeight(hn);
    }
    initializeContext(context, k, hypergraph_weight);

    partitioner = std::make_shared<BFSInitialPartitioner<TestStartNodeSelectionPolicy> >(
      *hypergraph, context);
  }

  std::shared_ptr<BFSInitialPartitioner<TestStartNodeSelectionPolicy> > partitioner;
  std::shared_ptr<Hypergraph> hypergraph;
  Context context;
};


TEST_F(ABFSBisectionInitialPartioner, ChecksCorrectBisectionCut) {
  partitioner->partition();
  std::vector<HypernodeID> partition_zero { 0, 1, 2, 3 };
  std::vector<HypernodeID> partition_one { 4, 5, 6 };
  for (unsigned int i = 0; i < partition_zero.size(); i++) {
    ASSERT_EQ(hypergraph.partID(partition_zero[i]), 0);
  }
  for (unsigned int i = 0; i < partition_one.size(); i++) {
    ASSERT_EQ(hypergraph.partID(partition_one[i]), 1);
  }
}


TEST_F(ABFSBisectionInitialPartioner, LeavesNoHypernodeUnassigned) {
  partitioner->partition();

  for (const HypernodeID& hn : hypergraph.nodes()) {
    ASSERT_NE(hypergraph.partID(hn), -1);
  }
}

TEST_F(ABFSBisectionInitialPartioner,
       HasCorrectInQueueMapValuesAfterPushingIncidentHypernodesNodesIntoQueue) {
  std::queue<HypernodeID> q;
  partitioner->_hypernode_in_queue.set(0, true);
  q.push(0);
  hypergraph.setNodePart(0, 0);
  context.initial_partitioning.unassigned_part = -1;
  partitioner->pushIncidentHypernodesIntoQueue(q, 0);
  for (HypernodeID hn = 0; hn < 5; hn++) {
    ASSERT_TRUE(partitioner->_hypernode_in_queue[hn]);
  }
  for (HypernodeID hn = 5; hn < 7; hn++) {
    ASSERT_FALSE(partitioner->_hypernode_in_queue[hn]);
  }
}

TEST_F(ABFSBisectionInitialPartioner, HasCorrectHypernodesInQueueAfterPushingIncidentHypernodesIntoQueue) {
  std::queue<HypernodeID> q;
  partitioner->_hypernode_in_queue.set(0, true);
  q.push(0);
  hypergraph.setNodePart(0, 0);
  context.initial_partitioning.unassigned_part = -1;
  partitioner->pushIncidentHypernodesIntoQueue(q, 0);
  std::vector<HypernodeID> expected_in_queue { 0, 2, 1, 3, 4 };
  for (unsigned int i = 0; i < expected_in_queue.size(); i++) {
    HypernodeID hn = q.front();
    q.pop();
    ASSERT_EQ(hn, expected_in_queue[i]);
  }

  ASSERT_TRUE(q.empty());
}

TEST_F(ABFSBisectionInitialPartioner, SetCorrectFixedVertexPart) {
  generateRandomFixedVertices(hypergraph, 0.1, 2);

  partitioner->partition();

  for (const HypernodeID& hn : hypergraph.fixedVertices()) {
    ASSERT_EQ(hypergraph.partID(hn), hypergraph.fixedVertexPartID(hn));
  }
}

TEST_F(AKWayBFSInitialPartitioner, HasValidImbalance) {
  partitioner->partition();

  ASSERT_LE(metrics::imbalance(*hypergraph, context), context.partition.epsilon);
}

TEST_F(AKWayBFSInitialPartitioner, HasNoSignificantLowPartitionWeights) {
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
    ASSERT_GE(hypergraph->partWeight(k), (lower_bound_factor / 100.0) * heaviest_part);
  }
}

TEST_F(AKWayBFSInitialPartitioner, LeavesNoHypernodeUnassigned) {
  partitioner->partition();

  for (const HypernodeID& hn : hypergraph->nodes()) {
    ASSERT_NE(hypergraph->partID(hn), -1);
  }
}

TEST_F(AKWayBFSInitialPartitioner, GrowPartitionOnPartitionMinus1) {
  context.initial_partitioning.unassigned_part = -1;
  partitioner->partition();

  for (const HypernodeID& hn : hypergraph->nodes()) {
    ASSERT_NE(hypergraph->partID(hn), -1);
  }
}

TEST_F(AKWayBFSInitialPartitioner, SetCorrectFixedVertexPart) {
  generateRandomFixedVertices(*hypergraph, 0.1, 4);
  ASSERT_GE(hypergraph->numFixedVertices(), 0);

  partitioner->partition();

  for (const HypernodeID& hn : hypergraph->fixedVertices()) {
    ASSERT_EQ(hypergraph->partID(hn), hypergraph->fixedVertexPartID(hn));
  }
}
}  // namespace kahypar
