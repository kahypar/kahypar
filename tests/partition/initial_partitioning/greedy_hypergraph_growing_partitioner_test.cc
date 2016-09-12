/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <memory>
#include <queue>
#include <unordered_map>
#include <vector>

#include "gmock/gmock.h"

#include "io/hypergraph_io.h"
#include "partition/configuration.h"
#include "partition/initial_partitioning/greedy_hypergraph_growing_initial_partitioner.h"
#include "partition/initial_partitioning/i_initial_partitioner.h"
#include "partition/initial_partitioning/initial_partitioner_base.h"
#include "partition/initial_partitioning/policies/ip_gain_computation_policy.h"
#include "partition/initial_partitioning/policies/ip_greedy_queue_selection_policy.h"
#include "partition/initial_partitioning/policies/ip_start_node_selection_policy.h"
#include "partition/metrics.h"

using::testing::Eq;
using::testing::Test;

using datastructure::BinaryMaxHeap;

namespace partition {
void initializeConfiguration(Hypergraph& hg, Configuration& config,
                             PartitionID k) {
  config.initial_partitioning.k = k;
  config.partition.k = k;
  config.initial_partitioning.epsilon = 0.05;
  config.partition.epsilon = 0.05;
  config.initial_partitioning.unassigned_part = 1;
  config.initial_partitioning.nruns = 1;
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
  Randomize::setSeed(config.partition.seed);
}

template <typename StartNodeSelection, typename GainComputation,
          typename QueueSelection>
struct GreedyTemplateStruct {
  typedef StartNodeSelection Type1;
  typedef GainComputation Type2;
  typedef QueueSelection Type3;
};

template <class T>
class AKWayGreedyHypergraphGrowingPartitionerTest : public Test {
 public:
  AKWayGreedyHypergraphGrowingPartitionerTest() :
    ghg(nullptr),
    hypergraph(nullptr),
    config() {
    std::string hypergraph_filename = "test_instances/test_instance.hgr";
    PartitionID k = 4;
    HypernodeID num_hypernodes;
    HyperedgeID num_hyperedges;
    HyperedgeIndexVector index_vector;
    HyperedgeVector edge_vector;
    HyperedgeWeightVector hyperedge_weights;
    HypernodeWeightVector hypernode_weights;
    io::readHypergraphFile(hypergraph_filename, num_hypernodes,
                           num_hyperedges, index_vector, edge_vector, &hyperedge_weights,
                           &hypernode_weights);
    hypergraph = std::make_shared<Hypergraph>(num_hypernodes, num_hyperedges,
                                              index_vector, edge_vector, k, &hyperedge_weights,
                                              &hypernode_weights);


    initializeConfiguration(*hypergraph, config, k);

    ghg = std::make_shared<GreedyHypergraphGrowingInitialPartitioner<typename T::Type1,
                                                                     typename T::Type2, typename T::Type3> >
            (*hypergraph, config);
  }

  std::shared_ptr<GreedyHypergraphGrowingInitialPartitioner<typename T::Type1,
                                                            typename T::Type2, typename T::Type3> > ghg;
  std::shared_ptr<Hypergraph> hypergraph;
  Configuration config;
};

typedef::testing::Types<
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         FMGainComputationPolicy, GlobalQueueSelectionPolicy>,
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         FMGainComputationPolicy, RoundRobinQueueSelectionPolicy>,
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         FMGainComputationPolicy, SequentialQueueSelectionPolicy>,
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         MaxPinGainComputationPolicy, GlobalQueueSelectionPolicy>,
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         MaxPinGainComputationPolicy, RoundRobinQueueSelectionPolicy>,
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         MaxPinGainComputationPolicy, SequentialQueueSelectionPolicy>,
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         MaxNetGainComputationPolicy, GlobalQueueSelectionPolicy>,
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         MaxNetGainComputationPolicy, RoundRobinQueueSelectionPolicy>,
    GreedyTemplateStruct<BFSStartNodeSelectionPolicy<>,
                         MaxNetGainComputationPolicy, SequentialQueueSelectionPolicy> > GreedyTestTemplates;

TYPED_TEST_CASE(AKWayGreedyHypergraphGrowingPartitionerTest,
                GreedyTestTemplates);

TYPED_TEST(AKWayGreedyHypergraphGrowingPartitionerTest, HasValidImbalance) {
  this->ghg->partition(*(this->hypergraph), this->config);
  ASSERT_LE(metrics::imbalance(*(this->hypergraph), this->config), this->config.partition.epsilon);
}

TYPED_TEST(AKWayGreedyHypergraphGrowingPartitionerTest, HasNoSignificantLowPartitionWeights) {
  this->ghg->partition(*(this->hypergraph), this->config);

  // Upper bounds of maximum partition weight should not be exceeded.
  HypernodeWeight heaviest_part = 0;
  for (PartitionID k = 0; k < this->config.initial_partitioning.k; k++) {
    if (heaviest_part < this->hypergraph->partWeight(k)) {
      heaviest_part = this->hypergraph->partWeight(k);
    }
  }

  // No partition weight should fall below under "lower_bound_factor"
  // percent of the heaviest partition weight.
  double lower_bound_factor = 50.0;
  for (PartitionID k = 0; k < this->config.initial_partitioning.k; k++) {
    ASSERT_GE(this->hypergraph->partWeight(k), (lower_bound_factor / 100.0) * heaviest_part);
  }
}

TYPED_TEST(AKWayGreedyHypergraphGrowingPartitionerTest, LeavesNoHypernodeUnassigned) {
  this->ghg->partition(*(this->hypergraph), this->config);

  for (HypernodeID hn : this->hypergraph->nodes()) {
    ASSERT_NE(this->hypergraph->partID(hn), -1);
  }
}

TYPED_TEST(AKWayGreedyHypergraphGrowingPartitionerTest, MultipleRun) {
  this->config.initial_partitioning.nruns = 3;
  this->ghg->partition(*(this->hypergraph), this->config);
}
}  // namespace partition
