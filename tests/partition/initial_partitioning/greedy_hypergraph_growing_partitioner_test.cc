/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "kahypar/partition/context.h"
#include "kahypar/partition/initial_partitioning/greedy_hypergraph_growing_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
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
  context.initial_partitioning.nruns = 1;
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
    context() {
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


    initializeContext(*hypergraph, context, k);

    ghg = std::make_shared<GreedyHypergraphGrowingInitialPartitioner<typename T::Type1,
                                                                     typename T::Type2, typename T::Type3> >
            (*hypergraph, context);
  }

  std::shared_ptr<GreedyHypergraphGrowingInitialPartitioner<typename T::Type1,
                                                            typename T::Type2, typename T::Type3> > ghg;
  std::shared_ptr<Hypergraph> hypergraph;
  Context context;
};

typedef ::testing::Types<
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
  this->ghg->partition();
  ASSERT_LE(metrics::imbalance(*(this->hypergraph), this->context), this->context.partition.epsilon);
}

TYPED_TEST(AKWayGreedyHypergraphGrowingPartitionerTest, HasNoSignificantLowPartitionWeights) {
  this->ghg->partition();

  // Upper bounds of maximum partition weight should not be exceeded.
  HypernodeWeight heaviest_part = 0;
  for (PartitionID k = 0; k < this->context.initial_partitioning.k; k++) {
    if (heaviest_part < this->hypergraph->partWeight(k)) {
      heaviest_part = this->hypergraph->partWeight(k);
    }
  }

  // No partition weight should fall below under "lower_bound_factor"
  // percent of the heaviest partition weight.
  double lower_bound_factor = 50.0;
  for (PartitionID k = 0; k < this->context.initial_partitioning.k; k++) {
    ASSERT_GE(this->hypergraph->partWeight(k), (lower_bound_factor / 100.0) * heaviest_part);
  }
}

TYPED_TEST(AKWayGreedyHypergraphGrowingPartitionerTest, LeavesNoHypernodeUnassigned) {
  this->ghg->partition();

  for (const HypernodeID& hn : this->hypergraph->nodes()) {
    ASSERT_NE(this->hypergraph->partID(hn), -1);
  }
}

TYPED_TEST(AKWayGreedyHypergraphGrowingPartitionerTest, MultipleRun) {
  this->context.initial_partitioning.nruns = 3;
  this->ghg->partition();
}

TYPED_TEST(AKWayGreedyHypergraphGrowingPartitionerTest, SetCorrectFixedVertexPart) {
  generateRandomFixedVertices(*(this->hypergraph), 0.1, 4);
  ASSERT_GE(this->hypergraph->numFixedVertices(), 0);

  this->ghg->partition();

  for (const HypernodeID& hn : this->hypergraph->fixedVertices()) {
    ASSERT_EQ(this->hypergraph->partID(hn), this->hypergraph->fixedVertexPartID(hn));
  }
}
}  // namespace kahypar
