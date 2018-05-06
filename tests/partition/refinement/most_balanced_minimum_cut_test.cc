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
#include "kahypar/partition/refinement/flow/maximum_flow.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
class MostBalancedMinimumCutTest : public Test {
 public:
  MostBalancedMinimumCutTest() :
    hypergraph(nullptr),
    flowNetwork(nullptr),
    maximumFlow(nullptr),
    context() {
    /**
      * Hypergraph:
      * 1 - 3 - 5 - 7 - 9  - 11
      * |   |   |   |   |    |
      * 2 - 4 - 6 - 8 - 10 - 12
      */
    std::string mbmc_hg_file =
      "test_instances/PerfectBalancedMBMC.hgr";

    HypernodeID num_hypernodes;
    HyperedgeID num_hyperedges;
    HyperedgeIndexVector index_vector;
    HyperedgeVector edge_vector;
    HyperedgeWeightVector hyperedge_weights;
    HypernodeWeightVector hypernode_weights;

    PartitionID k = 2;
    io::readHypergraphFile(
      mbmc_hg_file,
      num_hypernodes, num_hyperedges, index_vector, edge_vector,
      &hyperedge_weights, &hypernode_weights);
    hypergraph = std::make_unique<Hypergraph>(num_hypernodes, num_hyperedges,
                                              index_vector, edge_vector, k, &hyperedge_weights,
                                              &hypernode_weights);
    flowNetwork = std::make_unique<LawlerNetwork>(*hypergraph, context);
    maximumFlow = std::make_unique<IBFS<LawlerNetwork> >(*hypergraph, context, *flowNetwork);
    setupContext();
  }

  void setupFlowNetwork() {
    /**
     * Bipartition:
     *    V_1 | V_2
     * 1 - 3 -|- 5 - 7 - 9  - 11
     * |   |  |  |   |   |    |
     * 2 - 4 -|- 6 - 8 - 10 - 12
     *        |
     */
    for (const HypernodeID& hn : hypergraph->nodes()) {
      if (hn <= 4) {
        hypergraph->setNodePart(hn, 0);
      } else {
        hypergraph->setNodePart(hn, 1);
      }
    }
    for (HypernodeID hn = 3; hn <= 10; ++hn) {
      flowNetwork->addHypernode(hn);
    }
    flowNetwork->build(0, 1);
  }

 private:
  void setupContext() {
    context.partition.k = 2;
    context.partition.epsilon = 0.00;
    context.partition.objective = Objective::km1;

    context.partition.perfect_balance_part_weights.push_back(ceil(
                                                               hypergraph->totalWeight()
                                                               / static_cast<double>(context.partition.k)));
    context.partition.perfect_balance_part_weights.push_back(
      context.partition.perfect_balance_part_weights[0]);

    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[0]);
    context.partition.max_part_weights.push_back(context.partition.max_part_weights[0]);

    context.local_search.flow.use_most_balanced_minimum_cut = true;
  }

 public:
  std::unique_ptr<Hypergraph> hypergraph;
  std::unique_ptr<LawlerNetwork> flowNetwork;
  std::unique_ptr<IBFS<LawlerNetwork> > maximumFlow;
  Context context;
};


/**
 * Without Most Balanced Minimum Cut:
 *    V_1 | V_2
 * 1 - 3 -|- 5 - 7 - 9  - 11
 * |   |  |  |   |   |    |
 * 2 - 4 -|- 6 - 8 - 10 - 12
 *        |
 *
 * With Most Balanced Minimum Cut:
 *        V_1 | V_2
 * 1 - 3 - 5 -|- 7 - 9  - 11
 * |   |   |  |  |   |    |
 * 2 - 4 - 6 -|- 8 - 10 - 12
 *            |
 */
TEST_F(MostBalancedMinimumCutTest, WithPerfectBalancedBipartition) {
  setupFlowNetwork();
  HyperedgeWeight cut = maximumFlow->minimumSTCut(0, 1);
  ASSERT_EQ(cut, 2);
  ASSERT_EQ(metrics::imbalance(*hypergraph, context), 0);
}
}  // namespace kahypar
