/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
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


#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/flow/2way_flow_refiner.h"
#include "kahypar/partition/refinement/flow/policies/flow_execution_policy.h"
#include "kahypar/partition/refinement/flow/policies/flow_network_policy.h"
#include "kahypar/partition/registries/register_flow_networks.h"
#include "kahypar/partition/registries/register_refinement_algorithms.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
class TwoWayFlowRefinerTest : public ::testing::TestWithParam<FlowAlgorithm>{
 public:
  TwoWayFlowRefinerTest() :
    hypergraph(nullptr),
    twoWayFlowRefiner(nullptr),
    context() {
    std::string hg_file =
      "test_instances/ibm01.hgr";

    HypernodeID num_hypernodes;
    HyperedgeID num_hyperedges;
    HyperedgeIndexVector index_vector;
    HyperedgeVector edge_vector;
    HyperedgeWeightVector hyperedge_weights;
    HypernodeWeightVector hypernode_weights;

    PartitionID k = 2;
    io::readHypergraphFile(
      hg_file,
      num_hypernodes, num_hyperedges, index_vector, edge_vector,
      &hyperedge_weights, &hypernode_weights);
    hypergraph = std::make_unique<Hypergraph>(num_hypernodes, num_hyperedges,
                                              index_vector, edge_vector, k, &hyperedge_weights,
                                              &hypernode_weights);
    setupContext();
    setupPartition();
    twoWayFlowRefiner = std::make_unique<TwoWayFlowRefiner<HybridNetworkPolicy,
                                                           ExponentialFlowExecution> >(*hypergraph, context);
    twoWayFlowRefiner->initialize(0);
  }

  void testRefiner() {
    std::vector<HypernodeID>* refinement_nodes = nullptr;
    std::array<HypernodeWeight, 2>* max_allowed_part_weights = nullptr;
    UncontractionGainChanges* changes = nullptr;

    Metrics best_metrics;
    best_metrics.cut = metrics::hyperedgeCut(*hypergraph);
    best_metrics.km1 = metrics::km1(*hypergraph);
    best_metrics.imbalance = metrics::imbalance(*hypergraph, context);

    twoWayFlowRefiner->refine(*refinement_nodes, *max_allowed_part_weights,
                              *changes, best_metrics);

    // 2way flow refiner should update metrics correctly
    ASSERT_EQ(best_metrics.getMetric(context.partition.mode, context.partition.objective),
              metrics::objective(*hypergraph, context.partition.objective));
    ASSERT_EQ(best_metrics.imbalance, metrics::imbalance(*hypergraph, context));
  }

 private:
  void setupContext() {
    context.partition.k = 2;
    context.partition.epsilon = 0.03;
    context.partition.objective = Objective::km1;
    context.partition.mode = Mode::direct_kway;

    context.partition.perfect_balance_part_weights.push_back(ceil(
                                                               hypergraph->totalWeight()
                                                               / static_cast<double>(context.partition.k)));
    context.partition.perfect_balance_part_weights.push_back(
      context.partition.perfect_balance_part_weights[0]);

    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[0]);
    context.partition.max_part_weights.push_back(context.partition.max_part_weights[0]);

    context.local_search.flow.use_most_balanced_minimum_cut = true;
    context.local_search.flow.alpha = 4;
    context.local_search.flow.algorithm = FlowAlgorithm::ibfs;
    context.local_search.flow.network = FlowNetworkType::hybrid;
    context.local_search.flow.execution_policy = FlowExecutionMode::exponential;
    context.local_search.flow.use_adaptive_alpha_stopping_rule = true;
    context.local_search.flow.ignore_small_hyperedge_cut = true;
    context.local_search.flow.use_improvement_history = true;
  }

  void setupPartition() {
    PartitionID part = std::rand() % context.partition.k;
    for (const HypernodeID& hn : hypergraph->nodes()) {
      hypergraph->setNodePart(hn, part);
      part = (part + 1) % context.partition.k;
    }
  }

 public:
  std::unique_ptr<Hypergraph> hypergraph;
  std::unique_ptr<TwoWayFlowRefiner<HybridNetworkPolicy, ExponentialFlowExecution> > twoWayFlowRefiner;
  Context context;
};

INSTANTIATE_TEST_CASE_P(FlowAlgorithmRefinerTest,
                        TwoWayFlowRefinerTest,
                        ::testing::Values(FlowAlgorithm::edmond_karp,
                                          FlowAlgorithm::goldberg_tarjan,
                                          FlowAlgorithm::boykov_kolmogorov,
                                          FlowAlgorithm::ibfs));

TEST_P(TwoWayFlowRefinerTest, Km1Objective) {
  context.partition.objective = Objective::km1;
  context.local_search.flow.algorithm = GetParam();
  testRefiner();
}

TEST_P(TwoWayFlowRefinerTest, CutObjective) {
  context.partition.objective = Objective::cut;
  context.local_search.flow.algorithm = GetParam();
  testRefiner();
}
}  // namespace kahypar
