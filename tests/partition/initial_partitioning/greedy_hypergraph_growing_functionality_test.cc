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

#include "kahypar/datastructure/binary_heap.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/initial_partitioning/bfs_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/greedy_hypergraph_growing_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/policies/ip_gain_computation_policy.h"
#include "kahypar/partition/initial_partitioning/policies/ip_greedy_queue_selection_policy.h"
#include "kahypar/partition/initial_partitioning/policies/ip_start_node_selection_policy.h"
#include "kahypar/partition/metrics.h"


using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
void initializeContext(Hypergraph& hg, Context& context,
                       PartitionID k) {
  context.initial_partitioning.k = k;
  context.partition.k = k;
  context.partition.epsilon = 0.05;
  context.initial_partitioning.unassigned_part = 1;
  context.initial_partitioning.refinement = false;
  context.initial_partitioning.upper_allowed_partition_weight.resize(
    context.initial_partitioning.k);
  context.initial_partitioning.perfect_balance_partition_weight.resize(
    context.initial_partitioning.k);
  context.partition.max_part_weights.resize(context.partition.k);
  context.partition.perfect_balance_part_weights.resize(context.partition.k);
  for (int i = 0; i < context.initial_partitioning.k; i++) {
    context.initial_partitioning.perfect_balance_partition_weight[i] = ceil(
      hg.totalWeight()
      / static_cast<double>(context.initial_partitioning.k));
    context.initial_partitioning.upper_allowed_partition_weight[i] = ceil(
      hg.totalWeight()
      / static_cast<double>(context.initial_partitioning.k))
                                                                     * (1.0 + context.partition.epsilon);
  }
  for (int i = 0; i < context.initial_partitioning.k; ++i) {
    context.partition.perfect_balance_part_weights[i] =
      context.initial_partitioning.perfect_balance_partition_weight[i];
    context.partition.max_part_weights[i] = context.initial_partitioning.upper_allowed_partition_weight[i];
  }
  Randomize::instance().setSeed(context.partition.seed);
}

class AGreedyHypergraphGrowingFunctionalityTest : public Test {
 public:
  AGreedyHypergraphGrowingFunctionalityTest() :
    ghg(nullptr),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    context() {
    PartitionID k = 2;
    initializeContext(hypergraph, context, k);

    for (const HypernodeID& hn : hypergraph.nodes()) {
      if (hn != 0) {
        hypergraph.setNodePart(hn, 1);
      } else {
        hypergraph.setNodePart(hn, 0);
      }
    }

    ghg = std::make_shared<GreedyHypergraphGrowingInitialPartitioner<
                             BFSStartNodeSelectionPolicy<>, FMGainComputationPolicy,
                             GlobalQueueSelectionPolicy> >(hypergraph, context);
  }

  std::shared_ptr<GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                            FMGainComputationPolicy,
                                                            GlobalQueueSelectionPolicy> > ghg;
  Hypergraph hypergraph;
  Context context;
};

TEST_F(AGreedyHypergraphGrowingFunctionalityTest, InsertionOfAHypernodeIntoPQ) {
  ghg->insertNodeIntoPQ(2, 0);
  ASSERT_TRUE(ghg->_pq.contains(2, 0) && ghg->_pq.isEnabled(0));
}

TEST_F(AGreedyHypergraphGrowingFunctionalityTest, TryingToInsertAHypernodeIntoTheSamePQAsHisCurrentPart) {
  ghg->insertNodeIntoPQ(0, 0);
  ASSERT_TRUE(!ghg->_pq.contains(0, 0) && !ghg->_pq.isEnabled(0));
}

TEST_F(AGreedyHypergraphGrowingFunctionalityTest, TryingToInsertAFixedVertex) {
  hypergraph.setFixedVertex(0, 0);
  ghg->insertNodeIntoPQ(0, 0);
  ASSERT_TRUE(!ghg->_pq.contains(0, 0) && !ghg->_pq.isEnabled(0));
}


TEST_F(AGreedyHypergraphGrowingFunctionalityTest,
       ChecksCorrectMaxGainValueAndHypernodeAfterPushingSomeHypernodesIntoPriorityQueue) {
  ghg->insertNodeIntoPQ(2, 0, true);
  ghg->insertNodeIntoPQ(4, 0, true);
  ghg->insertNodeIntoPQ(6, 0, true);
  ASSERT_EQ(ghg->_pq.size(), 3);
  ASSERT_TRUE(ghg->_pq.max(0) == 2 && ghg->_pq.maxKey(0) == 0);
}


TEST_F(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectGainValueAfterUpdatePriorityQueue) {
  hypergraph.initializeNumCutHyperedges();
  context.initial_partitioning.unassigned_part = 0;
  ghg->insertNodeIntoPQ(0, 1, true);
  ASSERT_EQ(ghg->_pq.max(1), 0);
  ASSERT_EQ(ghg->_pq.maxKey(1), 2);

  hypergraph.changeNodePart(2, 1, 0);

  ghg->insertNodeIntoPQ(0, 1, true);
  ASSERT_EQ(ghg->_pq.max(1), 0);
  ASSERT_EQ(ghg->_pq.maxKey(1), 0);
}

TEST_F(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectMaxGainValueAfterDeltaGainUpdate) {
  hypergraph.initializeNumCutHyperedges();
  ghg->_pq.disablePart(context.initial_partitioning.unassigned_part);
  ghg->insertNodeIntoPQ(2, 0, false);
  ghg->insertNodeIntoPQ(4, 0, false);
  ghg->insertNodeIntoPQ(6, 0, false);

  hypergraph.changeNodePart(2, 1, 0);
  ghg->insertAndUpdateNodesAfterMove(2, 0, false);

  hypergraph.changeNodePart(4, 1, 0);
  ghg->insertAndUpdateNodesAfterMove(4, 0, false);

  ASSERT_TRUE(ghg->_pq.max(0) == 6 && ghg->_pq.maxKey(0) == 0);
  ASSERT_EQ(ghg->_pq.size(), 1);
}

TEST_F(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectHypernodesAndGainValuesInPQAfterAMove) {
  hypergraph.initializeNumCutHyperedges();
  ghg->_pq.disablePart(context.initial_partitioning.unassigned_part);

  hypergraph.changeNodePart(0, 0, 1);
  ghg->insertNodeIntoPQ(0, 0);
  ASSERT_TRUE(ghg->_pq.contains(0, 0));

  hypergraph.changeNodePart(0, 1, 0);
  ghg->insertAndUpdateNodesAfterMove(0, 0);

  ASSERT_EQ(ghg->_pq.size(), 4);
  ASSERT_TRUE(!ghg->_pq.contains(0, 0));

  ASSERT_TRUE(ghg->_pq.contains(1, 0) && !ghg->_pq.contains(1, 1));
  ASSERT_EQ(ghg->_pq.key(1, 0), 0);
  ASSERT_TRUE(ghg->_pq.contains(2, 0) && !ghg->_pq.contains(2, 1));
  ASSERT_EQ(ghg->_pq.key(2, 0), 0);
  ASSERT_TRUE(ghg->_pq.contains(3, 0) && !ghg->_pq.contains(3, 1));
  ASSERT_EQ(ghg->_pq.key(3, 0), -1);
  ASSERT_TRUE(ghg->_pq.contains(4, 0) && !ghg->_pq.contains(4, 1));
  ASSERT_EQ(ghg->_pq.key(4, 0), -1);
}

TEST_F(AGreedyHypergraphGrowingFunctionalityTest,
       ChecksCorrectMaxGainValueAfterDeltaGainUpdateWithUnassignedPartMinusOne) {
  hypergraph.resetPartitioning();
  context.initial_partitioning.unassigned_part = -1;
  ghg->insertNodeIntoPQ(2, 0);
  ghg->insertNodeIntoPQ(3, 0);
  ghg->insertNodeIntoPQ(4, 0);
  ghg->insertNodeIntoPQ(5, 1);
  ghg->insertNodeIntoPQ(6, 0);

  hypergraph.setNodePart(6, 1);
  ghg->insertAndUpdateNodesAfterMove(6, 1, false);
  ASSERT_TRUE(!ghg->_pq.contains(6, 0));
  ASSERT_EQ(ghg->_pq.key(2, 0), -1);
  ASSERT_EQ(ghg->_pq.key(3, 0), -1);
  ASSERT_EQ(ghg->_pq.key(4, 0), -1);
  ASSERT_EQ(ghg->_pq.key(5, 1), 0);


  hypergraph.setNodePart(4, 1);
  ghg->insertAndUpdateNodesAfterMove(4, 1, false);
  ASSERT_TRUE(!ghg->_pq.contains(4, 0));
  ASSERT_EQ(ghg->_pq.key(2, 0), -1);
  ASSERT_EQ(ghg->_pq.key(3, 0), -2);
  ASSERT_EQ(ghg->_pq.key(5, 1), 0);
}


TEST_F(AGreedyHypergraphGrowingFunctionalityTest, DeletesAssignedHypernodesFromPriorityQueue) {
  ghg->insertNodeIntoPQ(2, 0, false);
  ghg->insertNodeIntoPQ(4, 0, false);
  ghg->insertNodeIntoPQ(6, 0, false);

  ASSERT_EQ(ghg->_pq.size(), 3);
  ASSERT_TRUE(ghg->_pq.contains(2));
  ASSERT_TRUE(ghg->_pq.contains(4));
  ASSERT_TRUE(ghg->_pq.contains(6));

  ghg->removeHypernodeFromAllPQs(2);

  ASSERT_EQ(ghg->_pq.size(), 2);
  ASSERT_TRUE(!ghg->_pq.contains(2));
  ASSERT_TRUE(ghg->_pq.contains(4));
  ASSERT_TRUE(ghg->_pq.contains(6));
}
}  // namespace kahypar
