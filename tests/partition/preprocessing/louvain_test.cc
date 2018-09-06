/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2017 Tobias Heuer <tobias.heuer@gmx.net>
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

#include <fstream>
#include <set>
#include <vector>

#include "gmock/gmock.h"

#include "kahypar/datastructure/graph.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/macros.h"
#include "kahypar/partition/preprocessing/louvain.h"
#include "kahypar/partition/preprocessing/modularity.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
using ds::Graph;

class ALouvainAlgorithm : public Test {
 public:
  ALouvainAlgorithm() :
    louvain(nullptr),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    context() {
    context.preprocessing.community_detection.edge_weight = LouvainEdgeWeight::non_uniform;
    louvain = std::make_shared<Louvain<Modularity> >(hypergraph, context);
  }

  std::shared_ptr<Louvain<Modularity> > louvain;
  Hypergraph hypergraph;
  Context context;
};

class AModularityMeasure : public Test {
 public:
  AModularityMeasure() :
    modularity(nullptr),
    context(),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    graph(nullptr) {
    context.preprocessing.community_detection.edge_weight = LouvainEdgeWeight::non_uniform;
    graph = std::make_shared<Graph>(hypergraph, context);
    modularity = std::make_shared<Modularity>(*graph);
  }

  std::shared_ptr<Modularity> modularity;
  Context context;
  Hypergraph hypergraph;
  std::shared_ptr<Graph> graph;
};

TEST_F(AModularityMeasure, IsCorrectInitialized) {
  std::vector<EdgeWeight> expected_in = { 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L };
  std::vector<EdgeWeight> expected_tot = { 0.75L, 0.25L, 0.5L + 1.0L / 3.0L, 0.25L + 1.0L / 3.0L, 0.25L + 1.0L / 3.0L, 1.0L / 3.0L, 2.0L / 3.0L, 1.0L, 1.0L, 1.0L, 1.0L };
  for (const NodeID& node : graph->nodes()) {
    ASSERT_LE(std::abs(expected_in[node] - modularity->_internal_weight[node]), Graph::kEpsilon);
    ASSERT_LE(std::abs(expected_tot[node] - modularity->_total_weight[node]), Graph::kEpsilon);
  }
}

TEST_F(AModularityMeasure, RemoveNodeFromCommunity) {
  for (const NodeID& node : graph->nodes()) {
    modularity->remove(node, 0.0L);
    ASSERT_EQ(0.0L, modularity->_internal_weight[node]);
    ASSERT_EQ(0.0L, modularity->_total_weight[node]);
    ASSERT_EQ(graph->clusterID(node), -1);
  }
}

TEST_F(AModularityMeasure, RemoveNodeFromCommunityWithMoreThanOneNode) {
  modularity->remove(1, 0.0L);
  modularity->insert(1, 8, 0.25L);
  modularity->remove(3, 0.0L);
  modularity->insert(3, 8, 0.25L);

  modularity->remove(8, 0.5);

  ASSERT_EQ(0.0L, modularity->_internal_weight[8]);
  ASSERT_LE(std::abs(0.5L + 1.0L / 3.0L - modularity->_total_weight[8]), Graph::kEpsilon);
  ASSERT_EQ(graph->clusterID(8), -1);
}

TEST_F(AModularityMeasure, InsertNodeInCommunity) {
  modularity->remove(1, 0.0L);
  modularity->insert(1, 8, 0.25L);
  ASSERT_EQ(0.0L, modularity->_internal_weight[1]);
  ASSERT_EQ(0.0L, modularity->_total_weight[1]);
  ASSERT_EQ(0.5L, modularity->_internal_weight[8]);
  ASSERT_EQ(1.25L, modularity->_total_weight[8]);
  ASSERT_EQ(graph->clusterID(1), 8);

  modularity->remove(3, 0.0L);
  modularity->insert(3, 8, 0.25L);
  ASSERT_EQ(0.0L, modularity->_internal_weight[3]);
  ASSERT_EQ(0.0L, modularity->_total_weight[3]);
  ASSERT_EQ(1.0L, modularity->_internal_weight[8]);
  ASSERT_LE(std::abs(1.25L + (0.25L + 1.0L / 3.0L) - modularity->_total_weight[8]), Graph::kEpsilon);
  ASSERT_EQ(graph->clusterID(3), 8);
}

TEST_F(AModularityMeasure, CalculatesCorrectGainValuesForIsolatedNode) {
  modularity->remove(1, 0.0L);
  modularity->insert(1, 8, 0.25L);
  modularity->remove(3, 0.0L);
  modularity->insert(3, 8, 0.25L);

  modularity->remove(4, 0.0);

  for (const auto& incidentClusterWeight : graph->incidentClusterWeightOfNode(4)) {
    ClusterID cid = incidentClusterWeight.clusterID;
    EdgeWeight w = incidentClusterWeight.weight;
    EdgeWeight gain = modularity->gain(4, cid, w);
    EdgeWeight expected_gain = std::numeric_limits<EdgeWeight>::max() / 2.0L;
    if (cid == 8) {
      expected_gain = 0.116319444L;
    } else if (cid == 9) {
      expected_gain = 0.260416667L;
    }
    ASSERT_LE(std::abs(expected_gain - gain), Graph::kEpsilon);
  }
}

TEST_F(ALouvainAlgorithm, DoesOneLouvainPass) {
  Graph graph(hypergraph, context);
  Modularity modularity(graph);
  EdgeWeight quality_before = modularity.quality();
  EdgeWeight quality_after = louvain->louvain_pass(graph, modularity);
  ASSERT_LE(quality_before, quality_after);
}

TEST_F(ALouvainAlgorithm, AssingsMappingToNextLevelFinerGraph) {
  Graph graph(hypergraph, context);
  Modularity modularity(graph);
  louvain->louvain_pass(graph, modularity);
  auto contraction = graph.contractClusters();
  Graph coarseGraph = std::move(contraction.first);
  std::vector<NodeID> mapping = std::move(contraction.second);
  louvain->assignClusterToNextLevelFinerGraph(graph, coarseGraph, mapping);
  ASSERT_EQ(0, graph.clusterID(0));
  ASSERT_EQ(1, graph.clusterID(1));
  ASSERT_EQ(0, graph.clusterID(2));
  ASSERT_EQ(1, graph.clusterID(3));
  ASSERT_EQ(1, graph.clusterID(4));
  ASSERT_EQ(2, graph.clusterID(5));
  ASSERT_EQ(2, graph.clusterID(6));
  ASSERT_EQ(0, graph.clusterID(7));
  ASSERT_EQ(1, graph.clusterID(8));
  ASSERT_EQ(1, graph.clusterID(9));
  ASSERT_EQ(2, graph.clusterID(10));
}

namespace  ds {
TEST(ALouvainKarateClub, DoesLouvainAlgorithm) {
  std::string karate_club_file = "test_instances/karate_club.internal_graph";
  std::ifstream in(karate_club_file);
  NodeID num_nodes = 0;
  NodeID num_edges = 0;
  in >> num_nodes >> num_edges;
  ASSERT_EQ(num_nodes, 34);
  ASSERT_EQ(num_edges, 78);
  std::vector<std::vector<NodeID> > adj_list(num_nodes, std::vector<NodeID>());
  for (EdgeID i = 0; i < num_edges; ++i) {
    NodeID u = 0;
    NodeID v = 0;
    in >> u >> v;
    adj_list[--u].push_back(--v);
    adj_list[v].push_back(u);
  }
  std::vector<NodeID> adj_array(num_nodes + 1, 0);
  std::vector<Edge> edges;
  for (NodeID u = 0; u < num_nodes; ++u) {
    adj_array[u + 1] = adj_array[u] + adj_list[u].size();
    for (const NodeID& v : adj_list[u]) {
      Edge e;
      e.target_node = v;
      e.weight = 1.0L;
      edges.push_back(e);
    }
  }
  Context context;
  context.preprocessing.community_detection.max_pass_iterations = 100;
  context.preprocessing.community_detection.min_eps_improvement = 0.0001;
  context.preprocessing.community_detection.edge_weight = LouvainEdgeWeight::non_uniform;
  Graph graph(adj_array, edges);
  Louvain<Modularity, false> louvain(adj_array, edges, context);

  louvain.run();
  std::vector<ClusterID> expected_comm = { 0, 0, 0, 0, 1, 1, 1, 0, 2, 0, 1, 0, 0, 0, 2, 2, 1, 0,
                                           2, 0, 2, 0, 2, 3, 3, 3, 2, 3, 3, 2, 2, 3, 2, 2 };
  for (const NodeID& node : graph.nodes()) {
    ASSERT_EQ(louvain.clusterID(node), expected_comm[node]);
  }
}

TEST(Louvain, WorksOnGraphDSThatChangesHypergraphIntoGraph) {
  Context context;

  context.partition.k = 2;
  context.type = ContextType::main;
  context.partition.verbose_output = true;
  context.partition.graph_filename = "test_instances/karate_club.graph.hgr";
  context.preprocessing.community_detection.max_pass_iterations = 100;
  context.preprocessing.community_detection.min_eps_improvement = 0.0001;
  context.preprocessing.community_detection.edge_weight = LouvainEdgeWeight::uniform;

  Hypergraph hypergraph(
    io::createHypergraphFromFile(context.partition.graph_filename,
                                 context.partition.k));

  Louvain<Modularity, false> louvain(hypergraph, context);

  louvain.run();

  std::vector<ClusterID> expected_comm = { 0, 0, 0, 0, 1, 1, 1, 0, 2, 0, 1, 0, 0, 0, 2, 2, 1, 0,
                                           2, 0, 2, 0, 2, 3, 3, 3, 2, 3, 3, 2, 2, 3, 2, 2 };
  for (const NodeID& node : hypergraph.nodes()) {
    ASSERT_EQ(louvain.clusterID(node), expected_comm[node]);
  }
}
}  // namespace ds
}  // namespace kahypar
