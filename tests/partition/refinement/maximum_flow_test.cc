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
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/flow/maximum_flow.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
template <typename NetworkType,
          typename FlowAlgorithm = MaximumFlow<NetworkType> >
struct MaximumFlowTemplateStruct {
  typedef FlowAlgorithm Flow;
  typedef NetworkType Network;
};

template <typename MaximumFlowConfig>
class AMaximumFlow : public Test {
  using FlowAlgorithm = typename MaximumFlowConfig::Flow;
  using NetworkType = typename MaximumFlowConfig::Network;

 public:
  AMaximumFlow() :
    context(),
    hypergraph(10, 8, HyperedgeIndexVector { 0, 5, 7, 9, 11, 14, 16, 18, 20 },
               HyperedgeVector { 0, 1, 2, 3, 4, 4, 5, 5, 6, 5, 7, 5, 6, 7, 6, 8, 7, 9, 0, 2 }),
    flowNetwork(hypergraph, context),
    maximumFlow(hypergraph, context, flowNetwork) { }

  void setupFlowNetwork() {
    std::vector<HypernodeID> block0 = { 0, 2, 4, 9 };
    std::vector<HypernodeID> block1 = { 1, 3, 5, 6, 7, 8 };
    for (const HypernodeID& hn : block0) {
      hypergraph.setNodePart(hn, 0);
    }
    for (const HypernodeID& hn : block1) {
      hypergraph.setNodePart(hn, 1);
    }
    for (HypernodeID node = 2; node <= 7; ++node) {
      flowNetwork.addHypernode(node);
    }
    flowNetwork.build(0, 1);
  }

  Context context;
  Hypergraph hypergraph;
  NetworkType flowNetwork;
  FlowAlgorithm maximumFlow;
};

template <class Network>
using MaxFlowEdmondKarp = MaximumFlowTemplateStruct<Network, EdmondKarp<Network> >;

template <class Network>
using MaxFlowGoldbergTarjan = MaximumFlowTemplateStruct<Network, GoldbergTarjan<Network> >;

template <class Network>
using MaxFlowBoykovKolmogorov = MaximumFlowTemplateStruct<Network, BoykovKolmogorov<Network> >;

template <class Network>
using MaxFlowIBFS = MaximumFlowTemplateStruct<Network, IBFS<Network> >;

typedef ::testing::Types<MaxFlowEdmondKarp<LawlerNetwork>,
                         MaxFlowEdmondKarp<HeuerNetwork>,
                         MaxFlowEdmondKarp<WongNetwork>,
                         MaxFlowEdmondKarp<HybridNetwork>,
                         MaxFlowGoldbergTarjan<LawlerNetwork>,
                         MaxFlowGoldbergTarjan<HeuerNetwork>,
                         MaxFlowGoldbergTarjan<WongNetwork>,
                         MaxFlowGoldbergTarjan<HybridNetwork>,
                         MaxFlowBoykovKolmogorov<LawlerNetwork>,
                         MaxFlowBoykovKolmogorov<HeuerNetwork>,
                         MaxFlowBoykovKolmogorov<WongNetwork>,
                         MaxFlowBoykovKolmogorov<HybridNetwork>,
                         MaxFlowIBFS<LawlerNetwork>,
                         MaxFlowIBFS<HeuerNetwork>,
                         MaxFlowIBFS<WongNetwork>,
                         MaxFlowIBFS<HybridNetwork> > MaximumFlowTestTemplates;

TYPED_TEST_CASE(AMaximumFlow,
                MaximumFlowTestTemplates);

TYPED_TEST(AMaximumFlow, ChecksIfAugmentingPathExist) {
  this->setupFlowNetwork();

  bool pathExist = this->maximumFlow.bfs();
  ASSERT_TRUE(pathExist);

  NodeID cur = 18;
  while (!this->flowNetwork.isSource(cur)) {
    cur = this->maximumFlow._parent.get(cur)->source;
  }
  ASSERT_EQ(cur, 10);
}

TYPED_TEST(AMaximumFlow, AugmentAlongPath) {
  this->setupFlowNetwork();
  for (const NodeID& sink : this->flowNetwork.sinks()) {
    if (this->maximumFlow._parent.get(sink)) {
      NodeID t = this->maximumFlow.bfs();
      Flow f = this->maximumFlow.augment(t);
      ASSERT_EQ(f, 1);
      break;
    }
  }
}

TYPED_TEST(AMaximumFlow, CalculationOnASubhypergraph) {
  this->setupFlowNetwork();
  Flow f = this->maximumFlow.maximumFlow();
  ASSERT_EQ(f, 2);
}

TYPED_TEST(AMaximumFlow, MultiExecution) {
  for (size_t i = 0; i < 5; ++i) {
    this->flowNetwork.reset();
    this->setupFlowNetwork();
    Flow f = this->maximumFlow.maximumFlow();
    ASSERT_EQ(f, 2);
    this->hypergraph.resetPartitioning();
  }
}

TYPED_TEST(AMaximumFlow, FindMinimumSTCut) {
  this->context.local_search.flow.use_most_balanced_minimum_cut = false;
  this->setupFlowNetwork();
  HyperedgeWeight cut = this->maximumFlow.minimumSTCut(0, 1);
  ASSERT_EQ(metrics::hyperedgeCut(this->hypergraph), cut);
}

TYPED_TEST(AMaximumFlow, RollbackAfterMinimumSTCut) {
  this->context.local_search.flow.use_most_balanced_minimum_cut = false;
  std::vector<PartitionID> part_before = { 0, 1, 0, 1, 0, 1, 1, 1, 1, 0 };
  std::vector<PartitionID> part_after = { 0, 1, 0, 1, 1, 1, 1, 1, 1, 0 };
  this->setupFlowNetwork();
  HyperedgeWeight cut = this->maximumFlow.minimumSTCut(0, 1);
  ASSERT_EQ(metrics::hyperedgeCut(this->hypergraph), cut);
  for (const HypernodeID& hn : this->hypergraph.nodes()) {
    ASSERT_EQ(part_after[hn], this->hypergraph.partID(hn));
  }
  this->maximumFlow.rollback(true);
  for (const HypernodeID& hn : this->hypergraph.nodes()) {
    ASSERT_EQ(part_before[hn], this->hypergraph.partID(hn));
  }
  this->maximumFlow.rollback();
  ASSERT_EQ(metrics::hyperedgeCut(this->hypergraph), cut);
  for (const HypernodeID& hn : this->hypergraph.nodes()) {
    ASSERT_EQ(part_after[hn], this->hypergraph.partID(hn));
  }
}
}  // namespace kahypar
