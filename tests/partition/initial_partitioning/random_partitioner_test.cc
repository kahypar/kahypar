/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "gmock/gmock.h"

#include "io/HypergraphIO.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/RandomInitialPartitioner.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"

using::testing::Eq;
using::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using defs::PartitionID;

namespace partition {
void initializeConfiguration(Configuration& config, PartitionID k,
                             HypernodeWeight hypergraph_weight) {
  config.initial_partitioning.k = k;
  config.partition.k = k;
  config.initial_partitioning.epsilon = 0.05;
  config.partition.epsilon = 0.05;
  config.initial_partitioning.refinement = false;
  config.initial_partitioning.upper_allowed_partition_weight.resize(
    config.initial_partitioning.k);
  config.initial_partitioning.perfect_balance_partition_weight.resize(
    config.initial_partitioning.k);
  for (int i = 0; i < config.initial_partitioning.k; i++) {
    config.initial_partitioning.perfect_balance_partition_weight[i] = ceil(
      hypergraph_weight
      / static_cast<double>(config.initial_partitioning.k));
    config.initial_partitioning.upper_allowed_partition_weight[i] = ceil(
      hypergraph_weight
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
}

class ARandomBisectionInitialPartitionerTest : public Test {
 public:
  ARandomBisectionInitialPartitionerTest() :
    partitioner(nullptr),
    hypergraph(nullptr),
    config() {
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
    for (HypernodeID hn : hypergraph->nodes()) {
      hypergraph_weight += hypergraph->nodeWeight(hn);
    }
    initializeConfiguration(config, k, hypergraph_weight);

    partitioner = std::make_shared<RandomInitialPartitioner>(*hypergraph, config);
  }

  std::shared_ptr<RandomInitialPartitioner> partitioner;
  std::shared_ptr<Hypergraph> hypergraph;
  Configuration config;
};

class AKWayRandomInitialPartitionerTest : public Test {
 public:
  AKWayRandomInitialPartitionerTest() :
    partitioner(nullptr),
    hypergraph(nullptr),
    config() { }

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
    for (HypernodeID hn : hypergraph->nodes()) {
      hypergraph_weight += hypergraph->nodeWeight(hn);
    }
    initializeConfiguration(config, k, hypergraph_weight);

    partitioner = std::make_shared<RandomInitialPartitioner>(*hypergraph, config);
  }

  std::shared_ptr<RandomInitialPartitioner> partitioner;
  std::shared_ptr<Hypergraph> hypergraph;
  Configuration config;
};

TEST_F(ARandomBisectionInitialPartitionerTest, HasValidImbalance) {
  partitioner->partition(*hypergraph, config);

  ASSERT_LE(metrics::imbalance(*hypergraph, config),
            config.partition.epsilon);
}

TEST_F(ARandomBisectionInitialPartitionerTest, LeavesNoHypernodeUnassigned) {
  partitioner->partition(*hypergraph, config);

  for (HypernodeID hn : hypergraph->nodes()) {
    ASSERT_NE(hypergraph->partID(hn), -1);
  }
}

TEST_F(AKWayRandomInitialPartitionerTest, HasValidImbalance) {
  PartitionID k = 4;
  initializePartitioning(k);
  partitioner->partition(*hypergraph, config);

  ASSERT_LE(metrics::imbalance(*hypergraph, config),
            config.partition.epsilon);
}

TEST_F(AKWayRandomInitialPartitionerTest, HasNoSignificantLowPartitionWeights) {
  PartitionID k = 4;
  initializePartitioning(k);
  partitioner->partition(*hypergraph, config);

  // Upper bounds of maximum partition weight should not be exceeded.
  HypernodeWeight heaviest_part = 0;
  for (PartitionID k = 0; k < config.initial_partitioning.k; k++) {
    if (heaviest_part < hypergraph->partWeight(k)) {
      heaviest_part = hypergraph->partWeight(k);
    }
  }

  // No partition weight should fall below under "lower_bound_factor"
  // percent of the heaviest partition weight.
  double lower_bound_factor = 50.0;
  for (PartitionID k = 0; k < config.initial_partitioning.k; k++) {
    ASSERT_GE(hypergraph->partWeight(k),
              (lower_bound_factor / 100.0) * heaviest_part);
  }
}

TEST_F(AKWayRandomInitialPartitionerTest, LeavesNoHypernodeUnassigned) {
  PartitionID k = 4;
  initializePartitioning(k);
  partitioner->partition(*hypergraph, config);

  for (HypernodeID hn : hypergraph->nodes()) {
    ASSERT_NE(hypergraph->partID(hn), -1);
  }
}
}  // namespace partition
