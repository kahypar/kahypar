/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <memory>

#include "gmock/gmock.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"

using::testing::Eq;
using::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;

namespace partition {
class InitialPartitionerBaseTest : public Test {
 public:
  InitialPartitionerBaseTest() :
    partitioner(nullptr),
    hypergraph(7, 4,
               HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    config() {
    HypernodeWeight hypergraph_weight = 0;
    for (HypernodeID hn : hypergraph.nodes()) {
      hypergraph_weight += hypergraph.nodeWeight(hn);
    }

    initializeConfiguration(hypergraph_weight);
    partitioner = std::make_shared<InitialPartitionerBase>(hypergraph, config);
    partitioner->recalculateBalanceConstraints(config.initial_partitioning.epsilon);
  }

  void initializeConfiguration(HypernodeWeight hypergraph_weight) {
    config.initial_partitioning.k = 2;
    config.partition.k = 2;
    config.initial_partitioning.epsilon = 0.05;
    config.partition.epsilon = 0.05;
    config.initial_partitioning.seed = 1;
    config.initial_partitioning.upper_allowed_partition_weight.resize(2);
    config.initial_partitioning.perfect_balance_partition_weight.resize(2);
    for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
      config.initial_partitioning.perfect_balance_partition_weight[i] =
        ceil(
          hypergraph_weight
          / static_cast<double>(config.initial_partitioning.k));
    }
  }

  std::shared_ptr<InitialPartitionerBase> partitioner;
  Hypergraph hypergraph;
  Configuration config;
};

TEST_F(InitialPartitionerBaseTest, AssignHypernodesToPartition) {
  // Assign hypernodes
  ASSERT_TRUE(partitioner->assignHypernodeToPartition(0, 0));
  ASSERT_TRUE(partitioner->assignHypernodeToPartition(1, 0));
  ASSERT_TRUE(partitioner->assignHypernodeToPartition(2, 0));
  ASSERT_TRUE(partitioner->assignHypernodeToPartition(3, 0));
  ASSERT_TRUE(partitioner->assignHypernodeToPartition(4, 1));
  ASSERT_TRUE(partitioner->assignHypernodeToPartition(5, 1));
  ASSERT_TRUE(partitioner->assignHypernodeToPartition(6, 1));

  // Check, if all hypernodes are assigned correctly
  ASSERT_EQ(hypergraph.partID(0), 0);
  ASSERT_EQ(hypergraph.partID(1), 0);
  ASSERT_EQ(hypergraph.partID(2), 0);
  ASSERT_EQ(hypergraph.partID(3), 0);
  ASSERT_EQ(hypergraph.partID(4), 1);
  ASSERT_EQ(hypergraph.partID(5), 1);
  ASSERT_EQ(hypergraph.partID(6), 1);

  hypergraph.initializeNumCutHyperedges();
  // Changing hypernode partition id
  ASSERT_TRUE(partitioner->assignHypernodeToPartition(3, 1));
  ASSERT_FALSE(partitioner->assignHypernodeToPartition(3, 1));
}


TEST_F(InitialPartitionerBaseTest, ResetPartitionToMinusOne) {
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);

  config.initial_partitioning.unassigned_part = -1;
  partitioner->resetPartitioning();
  for (HypernodeID hn : hypergraph.nodes()) {
    ASSERT_EQ(hypergraph.partID(hn), -1);
  }
}

TEST_F(InitialPartitionerBaseTest, ResetPartitionToPartitionOne) {
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 1);
  hypergraph.setNodePart(3, 1);
  hypergraph.setNodePart(4, 1);
  hypergraph.setNodePart(5, 1);
  hypergraph.setNodePart(6, 1);

  config.initial_partitioning.unassigned_part = 0;
  partitioner->resetPartitioning();
  for (HypernodeID hn : hypergraph.nodes()) {
    ASSERT_EQ(hypergraph.partID(hn), 0);
  }
}
}  // namespace partition
