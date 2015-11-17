/*
 * label_propagation_partitioner_test.cc
 *
 *  Created on: 16.11.2015
 *      Author: theuer
 */

#include "gmock/gmock.h"
#include <queue>
#include <unordered_map>
#include <vector>

#include "lib/io/HypergraphIO.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/LabelPropagationInitialPartitioner.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"

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
  config.initial_partitioning.rollback = false;
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

template <typename StartNodeSelection, typename GainComputation>
struct LPTemplateStruct {
  typedef StartNodeSelection Type1;
  typedef GainComputation Type2;
};

template <class T>
class AKWayLabelPropagationInitialPartitionerTest : public Test {
 public:
  AKWayLabelPropagationInitialPartitionerTest() :
    config(), lp(nullptr), hypergraph(nullptr) {
    std::string hypergraph_filename = "test_instances/ibm01.hgr";
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
    hypergraph = new Hypergraph(num_hypernodes, num_hyperedges,
                                index_vector, edge_vector, k, &hyperedge_weights,
                                &hypernode_weights);

    initializeConfiguration(*hypergraph, config, k);

    lp = new LabelPropagationInitialPartitioner<typename T::Type1,
                                                typename T::Type2>(*hypergraph, config);
  }

  virtual ~AKWayLabelPropagationInitialPartitionerTest() {
    delete lp;
    delete hypergraph;
  }

  LabelPropagationInitialPartitioner<typename T::Type1, typename T::Type2>* lp;
  Hypergraph* hypergraph;
  Configuration config;
};

typedef::testing::Types<
    LPTemplateStruct<BFSStartNodeSelectionPolicy, FMGainComputationPolicy>,
    LPTemplateStruct<BFSStartNodeSelectionPolicy,
                     MaxPinGainComputationPolicy>,
    LPTemplateStruct<BFSStartNodeSelectionPolicy,
                     MaxNetGainComputationPolicy> > LPTestTemplates;

TYPED_TEST_CASE(AKWayLabelPropagationInitialPartitionerTest, LPTestTemplates);

TYPED_TEST(AKWayLabelPropagationInitialPartitionerTest, HasValidImbalance) {
  this->lp->partition(*(this->hypergraph), this->config);
  ASSERT_LE(metrics::imbalance(*(this->hypergraph), this->config), this->config.partition.epsilon);
}

TYPED_TEST(AKWayLabelPropagationInitialPartitionerTest, HasNoSignificantLowPartitionWeights) {
  this->lp->partition(*(this->hypergraph), this->config);

  // Upper bounds of maximum partition weight should not be exceeded.
  HypernodeWeight heaviest_part = 0;
  for (PartitionID k = 0; k < this->config.initial_partitioning.k; k++) {
    if (heaviest_part < this->hypergraph->partWeight(k)) {
      heaviest_part = this->hypergraph->partWeight(k);
    }
  }

  // No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
  double lower_bound_factor = 50.0;
  for (PartitionID k = 0; k < this->config.initial_partitioning.k; k++) {
    ASSERT_GE(this->hypergraph->partWeight(k), (lower_bound_factor / 100.0) * heaviest_part);
  }
}

TYPED_TEST(AKWayLabelPropagationInitialPartitionerTest, LeavesNoHypernodeUnassigned) {
  this->lp->partition(*(this->hypergraph), this->config);

  for (HypernodeID hn : this->hypergraph->nodes()) {
    ASSERT_NE(this->hypergraph->partID(hn), -1);
  }
}
}
