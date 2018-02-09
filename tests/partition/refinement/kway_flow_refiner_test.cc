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

#include "kahypar/io/hypergraph_io.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/registrar.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/flow/kway_flow_refiner.h"
#include "kahypar/partition/refinement/flow/policies/flow_network_policy.h"
#include "kahypar/partition/refinement/flow/policies/flow_execution_policy.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {

#define REGISTER_POLICY(policy, id, policy_class)                                  \
  static meta::Registrar<meta::PolicyRegistry<policy> > register_ ## policy_class( \
    id, new policy_class())

#define REGISTER_FLOW_ALGORITHM_FOR_NETWORK(id, flow, network)                                           \
  static meta::Registrar<FlowAlgorithmFactory<network> > register_ ## flow ## network(                   \
    id,                                                                                                  \
    [](Hypergraph& hypergraph, const Context& context, network& flow_network) -> MaximumFlow<network>* { \
    return new flow<network>(hypergraph, context, flow_network);                                         \
  })

REGISTER_POLICY(FlowNetworkType, FlowNetworkType::lawler,
                LawlerNetworkPolicy);
REGISTER_POLICY(FlowNetworkType, FlowNetworkType::heuer,
                HeuerNetworkPolicy);
REGISTER_POLICY(FlowNetworkType, FlowNetworkType::wong,
                WongNetworkPolicy);
REGISTER_POLICY(FlowNetworkType, FlowNetworkType::hybrid,
                HybridNetworkPolicy);

REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::edmond_karp, EdmondKarp, LawlerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::edmond_karp, EdmondKarp, HeuerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::edmond_karp, EdmondKarp, WongNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::edmond_karp, EdmondKarp, HybridNetwork);

REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::goldberg_tarjan, GoldbergTarjan, LawlerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::goldberg_tarjan, GoldbergTarjan, HeuerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::goldberg_tarjan, GoldbergTarjan, WongNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::goldberg_tarjan, GoldbergTarjan, HybridNetwork);

REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, LawlerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, HeuerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, WongNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::boykov_kolmogorov, BoykovKolmogorov, HybridNetwork);

REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, LawlerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, HeuerNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, WongNetwork);
REGISTER_FLOW_ALGORITHM_FOR_NETWORK(FlowAlgorithm::ibfs, IBFS, HybridNetwork);

class KWayFlowRefinerTest : public::testing::TestWithParam<FlowAlgorithm> {
 public:
  KWayFlowRefinerTest() :
    hypergraph(nullptr),
    kWayFlowRefiner(nullptr),
    context() {
    std::string hg_file =
      "test_instances/ibm01.hgr";

    HypernodeID num_hypernodes;
    HyperedgeID num_hyperedges;
    HyperedgeIndexVector index_vector;
    HyperedgeVector edge_vector;
    HyperedgeWeightVector hyperedge_weights;
    HypernodeWeightVector hypernode_weights;

    PartitionID k = 4;
    io::readHypergraphFile(
      hg_file,
      num_hypernodes, num_hyperedges, index_vector, edge_vector,
      &hyperedge_weights, &hypernode_weights);
    hypergraph = std::make_unique<Hypergraph>(num_hypernodes, num_hyperedges,
                                              index_vector, edge_vector, k, &hyperedge_weights,
                                              &hypernode_weights);
    setupContext();
    setupPartition();
    kWayFlowRefiner = std::make_unique<KWayFlowRefiner<HybridNetworkPolicy,
                                                       ExponentialFlowExecution>>(*hypergraph, context);
    kWayFlowRefiner->initialize(0);
  }

  void testRefiner() {
    std::vector<HypernodeID>* refinement_nodes = nullptr;
    std::array<HypernodeWeight, 2>* max_allowed_part_weights = nullptr;
    UncontractionGainChanges* changes = nullptr;

    Metrics best_metrics;
    best_metrics.cut = metrics::hyperedgeCut(*hypergraph);
    best_metrics.km1 = metrics::km1(*hypergraph);
    best_metrics.imbalance = metrics::imbalance(*hypergraph, context);
    Metrics old_metrics = best_metrics;

    kWayFlowRefiner->storeOriginalPartitionIDs();

    kWayFlowRefiner->refine(*refinement_nodes, *max_allowed_part_weights,
                              *changes, best_metrics);

    // 2way flow refiner should update metrics correctly
    ASSERT_EQ(best_metrics.getMetric(context.partition.objective),
              metrics::objective(*hypergraph, context.partition.objective));
    ASSERT_EQ(best_metrics.imbalance, metrics::imbalance(*hypergraph, context));

    // 2way flow refiner should rollback the partition correctly
    kWayFlowRefiner->restoreOriginalPartitionIDs();
    ASSERT_EQ(old_metrics.getMetric(context.partition.objective),
              metrics::objective(*hypergraph, context.partition.objective));
    ASSERT_EQ(old_metrics.imbalance, metrics::imbalance(*hypergraph, context));

    // If move the hypernodes also moved by the flow refiner during
    // refinement, the partition should be the same as before the
    // rollback
    for (const HypernodeID& hn : kWayFlowRefiner->movedHypernodes()) {
      PartitionID from = hypergraph->partID(hn);
      PartitionID to = kWayFlowRefiner->getDestinationPartID(hn);
      hypergraph->changeNodePart(hn, from, to);
    }
    ASSERT_EQ(best_metrics.getMetric(context.partition.objective),
              metrics::objective(*hypergraph, context.partition.objective));
    ASSERT_EQ(best_metrics.imbalance, metrics::imbalance(*hypergraph, context));
  }

 private:
  void setupContext() {
    context.partition.k = 4;
    context.partition.epsilon = 0.03;
    context.partition.objective = Objective::km1;
    context.partition.total_graph_weight = hypergraph->totalWeight();

    context.partition.perfect_balance_part_weights[0] = ceil(
      context.partition.total_graph_weight
      / static_cast<double>(context.partition.k));
    context.partition.perfect_balance_part_weights[1] =
      context.partition.perfect_balance_part_weights[0];

    context.partition.max_part_weights[0] = (1 + context.partition.epsilon)
                                            * context.partition.perfect_balance_part_weights[0];
    context.partition.max_part_weights[1] = context.partition.max_part_weights[0];

    context.local_search.flow.use_most_balanced_minimum_cut = true;
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
  std::unique_ptr<KWayFlowRefiner<HybridNetworkPolicy, ExponentialFlowExecution>> kWayFlowRefiner;
  Context context;
};

INSTANTIATE_TEST_CASE_P(FlowAlgorithmRefinerTest,
                        KWayFlowRefinerTest,
                        ::testing::Values(FlowAlgorithm::edmond_karp,
                                          FlowAlgorithm::goldberg_tarjan,
                                          FlowAlgorithm::boykov_kolmogorov,
                                          FlowAlgorithm::ibfs));

TEST_P(KWayFlowRefinerTest, Km1Objective) {
    context.partition.objective = Objective::km1;
    context.local_search.flow.algorithm = GetParam();
    testRefiner();
}

TEST_P(KWayFlowRefinerTest, CutObjective) {
    context.partition.objective = Objective::cut;
    context.local_search.flow.algorithm = GetParam();
    testRefiner();
}


}  // namespace kahypar
