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
#include "kahypar/partition/initial_partitioning/label_propagation_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/policies/ip_gain_computation_policy.h"
#include "kahypar/partition/initial_partitioning/policies/ip_start_node_selection_policy.h"
#include "kahypar/partition/metrics.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
void initializeContext(Context& context, PartitionID k,
                       HypernodeWeight hypergraph_weight) {
  context.initial_partitioning.k = k;
  context.partition.k = k;
  context.partition.epsilon = 1.0;
  context.initial_partitioning.refinement = false;
  context.initial_partitioning.upper_allowed_partition_weight.resize(
    context.initial_partitioning.k);
  context.initial_partitioning.perfect_balance_partition_weight.resize(
    context.initial_partitioning.k);
  for (int i = 0; i < context.initial_partitioning.k; i++) {
    context.initial_partitioning.perfect_balance_partition_weight[i] = ceil(
      hypergraph_weight
      / static_cast<double>(context.initial_partitioning.k));
    context.initial_partitioning.upper_allowed_partition_weight[i] = ceil(
      hypergraph_weight
      / static_cast<double>(context.initial_partitioning.k))
                                                                     * (1.0 + context.partition.epsilon);
  }
}

class ALabelPropagationMaxGainMoveTest : public Test {
 public:
  ALabelPropagationMaxGainMoveTest() :
    partitioner(nullptr),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    context() {
    PartitionID k = 2;
    initializeContext(context, k, 7);

    partitioner = std::make_shared<LabelPropagationInitialPartitioner<
                                     BFSStartNodeSelectionPolicy<>, FMGainComputationPolicy> >(
      hypergraph, context);
  }

  std::shared_ptr<LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                     FMGainComputationPolicy> > partitioner;
  Hypergraph hypergraph;
  Context context;
};

TEST_F(ALabelPropagationMaxGainMoveTest,
       CalculateMaxGainMoveOfAnUnassignedHypernodeIfAllHypernodesAreAssigned) {
  hypergraph.setNodePart(1, 0);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 0);
  hypergraph.setNodePart(4, 0);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(0);
  ASSERT_EQ(max_move.first, 0);
  ASSERT_EQ(max_move.second, -1);
}

TEST_F(ALabelPropagationMaxGainMoveTest,
       CalculateMaxGainMoveOfAnAssignedHypernodeIfAllHypernodesAreAssigned) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);
  std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(0);
  ASSERT_EQ(max_move.first, 1);
  ASSERT_EQ(max_move.second, 2);
}

TEST_F(ALabelPropagationMaxGainMoveTest,
       CalculateMaxGainMoveIfThereAStillUnassignedNodesOfAHyperedgeAreLeft) {
  hypergraph.setNodePart(0, 0);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(3, 1);
  std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(0);
  ASSERT_EQ(max_move.first, 1);
  ASSERT_EQ(max_move.second, 1);
}

TEST_F(ALabelPropagationMaxGainMoveTest, AllMaxGainMovesAZeroGainMovesIfNoHypernodeIsAssigned) {
  for (const HypernodeID& hn : hypergraph.nodes()) {
    std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(
      hn);
    ASSERT_EQ(max_move.first, -1);
  }
}

TEST_F(ALabelPropagationMaxGainMoveTest, MaxGainMoveIsAZeroGainMoveIfANetHasOnlyOneAssignedHypernode) {
  hypergraph.setNodePart(0, 0);
  const HypernodeID hn = 0;
  std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(hn);
  ASSERT_EQ(max_move.first, 0);
  ASSERT_EQ(max_move.second, 0);
}
}  // namespace kahypar
