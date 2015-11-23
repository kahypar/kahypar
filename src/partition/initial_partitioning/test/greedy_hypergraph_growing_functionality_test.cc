/*
 * greedy_hypergraph_growing_test.cc
 *
 *  Created on: 21.05.2015
 *      Author: theuer
 */

#include "gmock/gmock.h"

#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/io/HypergraphIO.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GreedyQueueSelectionPolicy.h"


using::testing::Eq;
using::testing::Test;

using datastructure::NoDataBinaryMaxHeap;
using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using defs::PartitionID;


using Gain = HyperedgeWeight;


namespace partition {
void initializeConfiguration(Hypergraph& hg, Configuration& config,
                             PartitionID k) {
  config.initial_partitioning.k = k;
  config.partition.k = k;
  config.initial_partitioning.epsilon = 0.05;
  config.partition.epsilon = 0.05;
  config.initial_partitioning.seed = 1;
  config.initial_partitioning.unassigned_part = 1;
  config.initial_partitioning.refinement = false;
  config.initial_partitioning.upper_allowed_partition_weight.resize(
    config.initial_partitioning.k);
  config.initial_partitioning.perfect_balance_partition_weight.resize(
    config.initial_partitioning.k);
  for (int i = 0; i < config.initial_partitioning.k; i++) {
    config.initial_partitioning.perfect_balance_partition_weight[i] = ceil(
      hg.totalWeight()
      / static_cast<double>(config.initial_partitioning.k));
    config.initial_partitioning.upper_allowed_partition_weight[i] = ceil(
      hg.totalWeight()
      / static_cast<double>(config.initial_partitioning.k))
                                                                    * (1.0 + config.partition.epsilon);
  }
  config.partition.perfect_balance_part_weights[0] =
    config.initial_partitioning.perfect_balance_partition_weight[0];
  config.partition.perfect_balance_part_weights[1] =
    config.initial_partitioning.perfect_balance_partition_weight[1];
  config.partition.max_part_weights[0] =
    config.initial_partitioning.upper_allowed_partition_weight[0];
  config.partition.max_part_weights[1] =
    config.initial_partitioning.upper_allowed_partition_weight[1];
  Randomize::setSeed(config.initial_partitioning.seed);
}

class AGreedyHypergraphGrowingFunctionalityTest : public Test {
 public:
  AGreedyHypergraphGrowingFunctionalityTest() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), ghg(
      nullptr) {
    PartitionID k = 2;
    initializeConfiguration(hypergraph, config, k);

    for (HypernodeID hn : hypergraph.nodes()) {
      if (hn != 0) {
        hypergraph.setNodePart(hn, 1);
      } else {
        hypergraph.setNodePart(hn, 0);
      }
    }

    ghg = new GreedyHypergraphGrowingInitialPartitioner<
      BFSStartNodeSelectionPolicy, FMGainComputationPolicy,
      GlobalQueueSelectionPolicy>(hypergraph, config);
  }

  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                            FMGainComputationPolicy, GlobalQueueSelectionPolicy>* ghg;
  Hypergraph hypergraph;
  Configuration config;
};

TEST_F(AGreedyHypergraphGrowingFunctionalityTest, InsertionOfAHypernodeIntoPQ) {
  ghg->insertNodeIntoPQ(2, 0);
  ASSERT_TRUE(ghg->_pq.contains(2, 0) && ghg->_pq.isEnabled(0));
}

TEST_F(AGreedyHypergraphGrowingFunctionalityTest, TryingToInsertAHypernodeIntoTheSamePQAsHisCurrentPart) {
  ghg->insertNodeIntoPQ(0, 0);
  ASSERT_TRUE(!ghg->_pq.contains(0, 0) && !ghg->_pq.isEnabled(0));
}


TEST_F(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectMaxGainValueAndHypernodeAfterPushingSomeHypernodesIntoPriorityQueue) {
  ghg->insertNodeIntoPQ(2, 0, true);
  ghg->insertNodeIntoPQ(4, 0, true);
  ghg->insertNodeIntoPQ(6, 0, true);
  ASSERT_EQ(ghg->_pq.size(), 3);
  ASSERT_TRUE(ghg->_pq.max(0) == 2 && ghg->_pq.maxKey(0) == 0);
}


TEST_F(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectGainValueAfterUpdatePriorityQueue) {
  hypergraph.initializeNumCutHyperedges();
  config.initial_partitioning.unassigned_part = 0;
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
  ghg->_pq.disablePart(config.initial_partitioning.unassigned_part);
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
  ghg->_pq.disablePart(config.initial_partitioning.unassigned_part);

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

TEST_F(AGreedyHypergraphGrowingFunctionalityTest, ChecksCorrectMaxGainValueAfterDeltaGainUpdateWithUnassignedPartMinusOne) {
  hypergraph.resetPartitioning();
  config.initial_partitioning.unassigned_part = -1;
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

  ghg->deleteNodeInAllBucketQueues(2);

  ASSERT_EQ(ghg->_pq.size(), 2);
  ASSERT_TRUE(!ghg->_pq.contains(2));
  ASSERT_TRUE(ghg->_pq.contains(4));
  ASSERT_TRUE(ghg->_pq.contains(6));
}

/*TEST_F(AGreedyHypergraphGrowingFunctionalityTest, CheckIfAllEnabledPQContainsAtLeastOneHypernode) {
  hypergraph.resetPartitioning();
  config.initial_partitioning.unassigned_part = -1;

  ghg->insertInAllEmptyEnabledQueuesAnUnassignedHypernode();
  ASSERT_EQ(ghg->_pq.size(), 2);
  ASSERT_TRUE(ghg->_pq.contains(0, 0));
  ASSERT_TRUE(ghg->_pq.contains(0, 1));

  ghg->deleteNodeInAllBucketQueues(0);
  ghg->_partEnabled[0] = false;
  ASSERT_EQ(ghg->_pq.size(), 0);
  ghg->insertInAllEmptyEnabledQueuesAnUnassignedHypernode();
  ASSERT_EQ(ghg->_pq.size(), 1);
  ASSERT_TRUE(!ghg->_pq.contains(0, 0));
  ASSERT_TRUE(ghg->_pq.contains(0, 1));
}*/
}
