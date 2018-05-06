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
#include <set>

#include "gmock/gmock.h"

#include "kahypar/datastructure/binary_heap.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/kway_priority_queue.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/policies/ip_gain_computation_policy.h"
#include "kahypar/partition/initial_partitioning/policies/ip_greedy_queue_selection_policy.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
class AGreedyQueueSelectionTest : public Test {
  using KWayRefinementPQ = ds::KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                                 std::numeric_limits<HyperedgeWeight>, true>;

 public:
  AGreedyQueueSelectionTest() :
    pq(4),
    hypergraph(7, 4,
               HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    context(),
    current_id(0),
    current_hn(-1),
    current_gain(-1),
    is_upper_bound_released(false),
    visit(hypergraph.initialNumNodes()) {
    PartitionID k = 4;
    pq.initialize(hypergraph.initialNumNodes());
    initializeContext(k);
  }

  template <class GainComputationPolicy>
  void pushHypernodesIntoQueue(std::vector<HypernodeID>& nodes, bool assign = true) {
    std::set<HypernodeID> part_0;
    part_0.insert(0);
    part_0.insert(1);
    part_0.insert(2);
    part_0.insert(6);

    if (assign) {
      for (const HypernodeID& hn : hypergraph.nodes()) {
        if (part_0.find(hn) != part_0.end()) {
          hypergraph.setNodePart(hn, 0);
        } else {
          hypergraph.setNodePart(hn, 1);
        }
      }
    }

    for (const HypernodeID& hn : nodes) {
      if (part_0.find(hn) != part_0.end()) {
        insertHypernodeIntoPQ<GainComputationPolicy>(hn, 1);
      } else {
        insertHypernodeIntoPQ<GainComputationPolicy>(hn, 0);
      }
    }
  }

  template <class GainComputationPolicy>
  void insertHypernodeIntoPQ(HypernodeID hn, PartitionID part) {
    pq.insert(hn, part, GainComputationPolicy::calculateGain(hypergraph, hn, part, visit));
    if (!pq.isEnabled(part)) {
      pq.enablePart(part);
    }
  }

  void initializeContext(PartitionID k) {
    context.initial_partitioning.k = k;
    context.partition.k = k;
    context.partition.epsilon = 0.05;
    context.initial_partitioning.upper_allowed_partition_weight.resize(k);
    context.initial_partitioning.perfect_balance_partition_weight.resize(k);
    for (PartitionID i = 0; i < context.initial_partitioning.k; i++) {
      context.initial_partitioning.perfect_balance_partition_weight.push_back(
        ceil(
          hypergraph.totalWeight()
          / static_cast<double>(context.initial_partitioning.k)));
      context.initial_partitioning.upper_allowed_partition_weight.push_back(
        ceil(
          hypergraph.totalWeight()
          / static_cast<double>(context.initial_partitioning.k))
        * (1.0 + context.partition.epsilon));
    }
  }

  KWayRefinementPQ pq;
  Hypergraph hypergraph;
  Context context;
  PartitionID current_id;
  HypernodeID current_hn;
  Gain current_gain;
  bool is_upper_bound_released;
  ds::FastResetFlagArray<> visit;
};

TEST_F(AGreedyQueueSelectionTest, ChecksRoundRobinNextQueueID) {
  for (const HypernodeID& hn : hypergraph.nodes()) {
    insertHypernodeIntoPQ<FMGainComputationPolicy>(hn, hn % 4);
  }
  ASSERT_EQ(current_id, 0);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 1);
  ASSERT_EQ(current_hn, 1);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 2);
  ASSERT_EQ(current_hn, 2);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 3);
  ASSERT_EQ(current_hn, 3);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 0);
}

TEST_F(AGreedyQueueSelectionTest, ChecksRoundRobinNextQueueIDIfSomePartsAreDisabled) {
  for (const HypernodeID& hn : hypergraph.nodes()) {
    insertHypernodeIntoPQ<FMGainComputationPolicy>(hn, 2 * (hn % 2));
  }
  ASSERT_EQ(current_id, 0);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 2);
  ASSERT_EQ(current_hn, 1);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 0);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 2);
  ASSERT_EQ(current_hn, 5);
}

TEST_F(AGreedyQueueSelectionTest, ChecksIfRoundRobinReturnsFalseIfEveryPartIsDisabled) {
  ASSERT_FALSE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, -1);
}

TEST_F(AGreedyQueueSelectionTest, ChecksGlobalNextQueueID) {
  initializeContext(2);
  std::vector<HypernodeID> nodes = { 0, 2, 5 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);
  current_id = 1;
  ASSERT_TRUE(
    GlobalQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                            current_hn, current_gain, current_id,
                                            is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);
}

TEST_F(AGreedyQueueSelectionTest, ChecksGlobalNextQueueIDIfSomePartsAreDisabled) {
  initializeContext(2);
  std::vector<HypernodeID> nodes = { 0, 2, 5 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);

  current_id = 1;
  ASSERT_TRUE(
    GlobalQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                            current_hn, current_gain, current_id,
                                            is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);

  insertHypernodeIntoPQ<FMGainComputationPolicy>(5, 0);

  pq.disablePart(0);
  ASSERT_TRUE(
    GlobalQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                            current_hn, current_gain, current_id,
                                            is_upper_bound_released));
  ASSERT_EQ(current_id, 1);
  ASSERT_EQ(current_hn, 0);
}

TEST_F(AGreedyQueueSelectionTest, ChecksIfGlobalReturnsFalseIfEveryPartIsDisabled) {
  initializeContext(2);
  std::vector<HypernodeID> nodes = { 0, 2, 5 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);

  pq.disablePart(0);
  pq.disablePart(1);
  ASSERT_FALSE(
    GlobalQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                            current_hn, current_gain, current_id,
                                            is_upper_bound_released));
  ASSERT_EQ(current_id, -1);
}

TEST_F(AGreedyQueueSelectionTest, ChecksSequentialNextQueueID) {
  initializeContext(2);
  context.initial_partitioning.unassigned_part = -1;
  std::vector<HypernodeID> nodes = { 0, 1, 2, 3, 4, 5, 6 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes, false);
  context.initial_partitioning.upper_allowed_partition_weight[0] = 2;
  context.initial_partitioning.upper_allowed_partition_weight[1] = 2;

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 3);
  ASSERT_FALSE(pq.contains(3, 0));
  hypergraph.setNodePart(3, 0);

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);
  ASSERT_FALSE(pq.contains(5, 0));
  hypergraph.setNodePart(5, 0);

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 1);
  ASSERT_EQ(current_hn, 0);
  ASSERT_FALSE(pq.contains(0, 1));
}

TEST_F(AGreedyQueueSelectionTest, ChecksSequentialNextQueueIDWithUnassignedPartPlusOne) {
  initializeContext(2);
  context.initial_partitioning.unassigned_part = 1;
  std::vector<HypernodeID> nodes = { 0, 1, 2, 3, 4, 5, 6 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes, false);
  pq.disablePart(context.initial_partitioning.unassigned_part);
  context.initial_partitioning.upper_allowed_partition_weight[0] = 2;
  context.initial_partitioning.upper_allowed_partition_weight[1] = 2;

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 3);
  ASSERT_FALSE(pq.contains(3, 0));
  hypergraph.setNodePart(3, 0);

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);
  ASSERT_FALSE(pq.contains(5, 0));
  hypergraph.setNodePart(5, 0);

  ASSERT_FALSE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, -1);
}

TEST_F(AGreedyQueueSelectionTest, ChecksSequentialNextQueueIDBehaviourIfUpperBoundIsReleased) {
  initializeContext(2);
  std::vector<HypernodeID> nodes = { 0, 2, 5 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);
  is_upper_bound_released = true;
  current_id = 1;

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, context, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);
}
}  // namespace kahypar
