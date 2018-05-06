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
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/label_propagation_initial_partitioner.h"
#include "kahypar/partition/initial_partitioning/policies/ip_gain_computation_policy.h"
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
  context.initial_partitioning.nruns = 20;
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

template <typename StartNodeSelection, typename GainComputation>
struct LPTemplateStruct {
  typedef StartNodeSelection Type1;
  typedef GainComputation Type2;
};

template <class T>
class AKWayLabelPropagationInitialPartitionerTest : public Test {
 public:
  AKWayLabelPropagationInitialPartitionerTest() :
    lp(nullptr),
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


    lp = std::make_shared<LabelPropagationInitialPartitioner<typename T::Type1,
                                                             typename T::Type2> >(*hypergraph, context);
  }


  std::shared_ptr<LabelPropagationInitialPartitioner<typename T::Type1, typename T::Type2> > lp;
  std::shared_ptr<Hypergraph> hypergraph;
  Context context;
};

typedef ::testing::Types<
    LPTemplateStruct<BFSStartNodeSelectionPolicy<>, FMGainComputationPolicy>
    // LPTemplateStruct<BFSStartNodeSelectionPolicy<>,
    //                  MaxPinGainComputationPolicy>
    // LPTemplateStruct<BFSStartNodeSelectionPolicy<>,
    //                  MaxNetGainComputationPolicy>
    > LPTestTemplates;

TYPED_TEST_CASE(AKWayLabelPropagationInitialPartitionerTest, LPTestTemplates);

TYPED_TEST(AKWayLabelPropagationInitialPartitionerTest, HasValidImbalance) {
  this->lp->partition();
  ASSERT_LE(metrics::imbalance(*(this->hypergraph), this->context), this->context.partition.epsilon);
}

TYPED_TEST(AKWayLabelPropagationInitialPartitionerTest, HasNoSignificantLowPartitionWeights) {
  this->lp->partition();

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

TYPED_TEST(AKWayLabelPropagationInitialPartitionerTest, LeavesNoHypernodeUnassigned) {
  this->lp->partition();

  for (const HypernodeID& hn : this->hypergraph->nodes()) {
    ASSERT_NE(this->hypergraph->partID(hn), -1);
  }
}

TYPED_TEST(AKWayLabelPropagationInitialPartitionerTest, SetCorrectFixedVertexPart) {
  generateRandomFixedVertices(*(this->hypergraph), 0.1, 4);
  ASSERT_GE(this->hypergraph->numFixedVertices(), 0);

  this->lp->partition();

  for (const HypernodeID& hn : this->hypergraph->fixedVertices()) {
    ASSERT_EQ(this->hypergraph->partID(hn), this->hypergraph->fixedVertexPartID(hn));
  }
}
}  // namespace kahypar
