/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <memory>
#include <set>

#include "gmock/gmock.h"

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/policies/GreedyQueueSelectionPolicy.h"

using::testing::Eq;
using::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using partition::FMGainComputationPolicy;
using datastructure::KWayPriorityQueue;

using Gain = HyperedgeWeight;

namespace partition {
using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                           std::numeric_limits<HyperedgeWeight>,
                                           ArrayStorage<HypernodeID>, true>;

class AGreedyQueueSelectionTest : public Test {
 public:
  AGreedyQueueSelectionTest() :
    pq(4),
    hypergraph(7, 4,
               HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    config(),
    current_id(0),
    current_hn(-1),
    current_gain(-1),
    is_upper_bound_released(false),
    visit(hypergraph.initialNumNodes(), false) {
    PartitionID k = 4;
    pq.initialize(hypergraph.initialNumNodes());
    initializeConfiguration(k);
  }

  template <class GainComputationPolicy>
  void pushHypernodesIntoQueue(std::vector<HypernodeID>& nodes, bool assign = true) {
    std::set<HypernodeID> part_0;
    part_0.insert(0);
    part_0.insert(1);
    part_0.insert(2);
    part_0.insert(6);

    if (assign) {
      for (HypernodeID hn : hypergraph.nodes()) {
        if (part_0.find(hn) != part_0.end()) {
          hypergraph.setNodePart(hn, 0);
        } else {
          hypergraph.setNodePart(hn, 1);
        }
      }
    }

    for (HypernodeID hn : nodes) {
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

  void initializeConfiguration(PartitionID k) {
    config.initial_partitioning.k = k;
    config.partition.k = k;
    config.initial_partitioning.epsilon = 0.05;
    config.partition.epsilon = 0.05;
    config.initial_partitioning.upper_allowed_partition_weight.resize(k);
    config.initial_partitioning.perfect_balance_partition_weight.resize(k);
    for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
      config.initial_partitioning.perfect_balance_partition_weight[i] =
        ceil(
          hypergraph.totalWeight()
          / static_cast<double>(config.initial_partitioning.k));
      config.initial_partitioning.upper_allowed_partition_weight[i] =
        ceil(
          hypergraph.totalWeight()
          / static_cast<double>(config.initial_partitioning.k))
        * (1.0 + config.partition.epsilon);
    }
  }

  KWayRefinementPQ pq;
  Hypergraph hypergraph;
  Configuration config;
  PartitionID current_id;
  HypernodeID current_hn;
  Gain current_gain;
  bool is_upper_bound_released;
  FastResetBitVector<> visit;
};

TEST_F(AGreedyQueueSelectionTest, ChecksRoundRobinNextQueueID) {
  for (HypernodeID hn : hypergraph.nodes()) {
    insertHypernodeIntoPQ<FMGainComputationPolicy>(hn, hn % 4);
  }
  ASSERT_EQ(current_id, 0);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 1);
  ASSERT_EQ(current_hn, 1);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 2);
  ASSERT_EQ(current_hn, 2);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 3);
  ASSERT_EQ(current_hn, 3);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 0);
}

TEST_F(AGreedyQueueSelectionTest, ChecksRoundRobinNextQueueIDIfSomePartsAreDisabled) {
  for (HypernodeID hn : hypergraph.nodes()) {
    insertHypernodeIntoPQ<FMGainComputationPolicy>(hn, 2 * (hn % 2));
  }
  ASSERT_EQ(current_id, 0);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 2);
  ASSERT_EQ(current_hn, 1);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 0);
  ASSERT_TRUE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 2);
  ASSERT_EQ(current_hn, 5);
}

TEST_F(AGreedyQueueSelectionTest, ChecksIfRoundRobinReturnsFalseIfEveryPartIsDisabled) {
  ASSERT_FALSE(
    RoundRobinQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, -1);
}

TEST_F(AGreedyQueueSelectionTest, ChecksGlobalNextQueueID) {
  initializeConfiguration(2);
  std::vector<HypernodeID> nodes = { 0, 2, 5 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);
  current_id = 1;
  ASSERT_TRUE(
    GlobalQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                            current_hn, current_gain, current_id,
                                            is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);
}

TEST_F(AGreedyQueueSelectionTest, ChecksGlobalNextQueueIDIfSomePartsAreDisabled) {
  initializeConfiguration(2);
  std::vector<HypernodeID> nodes = { 0, 2, 5 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);

  current_id = 1;
  ASSERT_TRUE(
    GlobalQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                            current_hn, current_gain, current_id,
                                            is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);

  insertHypernodeIntoPQ<FMGainComputationPolicy>(5, 0);

  pq.disablePart(0);
  ASSERT_TRUE(
    GlobalQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                            current_hn, current_gain, current_id,
                                            is_upper_bound_released));
  ASSERT_EQ(current_id, 1);
  ASSERT_EQ(current_hn, 0);
}

TEST_F(AGreedyQueueSelectionTest, ChecksIfGlobalReturnsFalseIfEveryPartIsDisabled) {
  initializeConfiguration(2);
  std::vector<HypernodeID> nodes = { 0, 2, 5 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);

  pq.disablePart(0);
  pq.disablePart(1);
  ASSERT_FALSE(
    GlobalQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                            current_hn, current_gain, current_id,
                                            is_upper_bound_released));
  ASSERT_EQ(current_id, -1);
}

TEST_F(AGreedyQueueSelectionTest, ChecksSequentialNextQueueID) {
  initializeConfiguration(2);
  config.initial_partitioning.unassigned_part = -1;
  std::vector<HypernodeID> nodes = { 0, 1, 2, 3, 4, 5, 6 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes, false);
  config.initial_partitioning.upper_allowed_partition_weight[0] = 2;
  config.initial_partitioning.upper_allowed_partition_weight[1] = 2;

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 3);
  ASSERT_FALSE(pq.contains(3, 0));
  hypergraph.setNodePart(3, 0);

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);
  ASSERT_FALSE(pq.contains(5, 0));
  hypergraph.setNodePart(5, 0);

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 1);
  ASSERT_EQ(current_hn, 0);
  ASSERT_FALSE(pq.contains(0, 1));
}

TEST_F(AGreedyQueueSelectionTest, ChecksSequentialNextQueueIDWithUnassignedPartPlusOne) {
  initializeConfiguration(2);
  config.initial_partitioning.unassigned_part = 1;
  std::vector<HypernodeID> nodes = { 0, 1, 2, 3, 4, 5, 6 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes, false);
  config.initial_partitioning.upper_allowed_partition_weight[0] = 2;
  config.initial_partitioning.upper_allowed_partition_weight[1] = 2;

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 3);
  ASSERT_FALSE(pq.contains(3, 0));
  hypergraph.setNodePart(3, 0);

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);
  ASSERT_FALSE(pq.contains(5, 0));
  hypergraph.setNodePart(5, 0);

  ASSERT_FALSE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, -1);
}

TEST_F(AGreedyQueueSelectionTest, ChecksSequentialNextQueueIDBehaviourIfUpperBoundIsReleased) {
  initializeConfiguration(2);
  std::vector<HypernodeID> nodes = { 0, 2, 5 };
  pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);
  is_upper_bound_released = true;
  current_id = 1;

  ASSERT_TRUE(
    SequentialQueueSelectionPolicy::nextQueueID(hypergraph, config, pq,
                                                current_hn, current_gain, current_id,
                                                is_upper_bound_released));
  ASSERT_EQ(current_id, 0);
  ASSERT_EQ(current_hn, 5);
}
}  // namespace partition
