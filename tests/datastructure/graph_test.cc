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

#include <numeric>
#include <set>
#include <vector>

#include "gmock/gmock.h"

#include "kahypar/datastructure/graph.h"
#include "kahypar/definitions.h"

using ::testing::Eq;
using ::testing::Test;


namespace kahypar {
namespace ds {
class ABipartiteGraph : public Test {
 public:
  ABipartiteGraph() :
    graph(nullptr),
    context(),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) {
    context.preprocessing.community_detection.edge_weight = LouvainEdgeWeight::non_uniform;
    graph = std::make_shared<Graph>(hypergraph, context);
  }

  std::shared_ptr<Graph> graph;
  Context context;
  Hypergraph hypergraph;
};


bool bicoloring(const NodeID cur, const int cur_col, std::vector<int>& col,
                const std::shared_ptr<Graph>& graph) {
  col[cur] = cur_col;
  for (const Edge& e : graph->incidentEdges(cur)) {
    const NodeID id = e.target_node;
    if (col[id] == -1) {
      return bicoloring(id, 1 - cur_col, col, graph);
    } else if (col[id] == col[cur]) {
      return false;
    }
  }
  return true;
}


TEST_F(ABipartiteGraph, ConstructedFromAHypergraphIsBipartite) {
  std::vector<int> col(graph->numNodes(), -1);
  for (const NodeID& id : graph->nodes()) {
    if (col[id] == -1) {
      ASSERT_TRUE(bicoloring(id, 0, col, graph));
    }
  }
}


TEST_F(ABipartiteGraph, ConstructedFromAHypergraphIsEquivalentToHypergraph) {
  HypernodeID numNodes = hypergraph.initialNumNodes();
  for (const HypernodeID& hn : hypergraph.nodes()) {
    std::set<NodeID> edges;
    for (const HyperedgeID& he : hypergraph.incidentEdges(hn)) {
      edges.insert(numNodes + he);
    }
    ASSERT_EQ(edges.size(), graph->degree(hn));
    for (const Edge& e : graph->incidentEdges(hn)) {
      ASSERT_FALSE(edges.find(e.target_node) == edges.end());
    }
  }

  for (const HyperedgeID& he : hypergraph.edges()) {
    std::set<NodeID> edges;
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      edges.insert(pin);
    }
    ASSERT_EQ(edges.size(), graph->degree(he + numNodes));
    for (const Edge& e : graph->incidentEdges(he + numNodes)) {
      ASSERT_FALSE(edges.find(e.target_node) == edges.end());
      ASSERT_EQ(e.weight, static_cast<EdgeWeight>(hypergraph.edgeWeight(he)) /
                static_cast<EdgeWeight>(hypergraph.edgeSize(he)));
    }
  }
}

TEST_F(ABipartiteGraph, HasCorrectTotalWeight) {
  ASSERT_LE(std::abs(8.0L - graph->totalWeight()), Graph::kEpsilon);
}

TEST_F(ABipartiteGraph, HasCorrectInitializedClusterIDs) {
  for (const NodeID& id : graph->nodes()) {
    ASSERT_EQ(id, graph->clusterID(id));
  }
}

TEST_F(ABipartiteGraph, DeterminesIncidentClusterWeightsCorrect) {
  NodeID node = 8;
  std::vector<EdgeWeight> cluster_weight = { 0.25L, 0.25L, 0.0L, 0.25L, 0.25L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L };
  std::vector<bool> incident_cluster = { true, true, false, true, true, false, false, false, true, false, false };
  for (const IncidentClusterWeight& incidentClusterWeight : graph->incidentClusterWeightOfNode(node)) {
    ClusterID c_id = incidentClusterWeight.clusterID;
    EdgeWeight weight = incidentClusterWeight.weight;
    ASSERT_TRUE(incident_cluster[c_id]);
    ASSERT_EQ(cluster_weight[c_id], weight);
  }
}

TEST_F(ABipartiteGraph, DeterminesIncidentClusterWeightsCorrectAfterSomeNodesHasChangedTheirCluster) {
  NodeID node = 8;
  graph->setClusterID(1, 0);
  graph->setClusterID(3, 0);
  std::vector<EdgeWeight> cluster_weight = { 0.75L, 0.0L, 0.0L, 0.0L, 0.25L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L };
  std::vector<bool> incident_cluster = { true, false, false, false, true, false, false, false, true, false, false };
  for (const IncidentClusterWeight& incidentClusterWeight : graph->incidentClusterWeightOfNode(node)) {
    ClusterID c_id = incidentClusterWeight.clusterID;
    EdgeWeight weight = incidentClusterWeight.weight;
    ASSERT_TRUE(incident_cluster[c_id]);
    ASSERT_EQ(cluster_weight[c_id], weight);
  }
}

TEST_F(ABipartiteGraph, DeterminesIncidentClusterWeightsOfAClusterCorrect) {
  graph->setClusterID(2, 0);
  graph->setClusterID(7, 0);
  graph->setClusterID(1, 1);
  graph->setClusterID(3, 1);
  graph->setClusterID(4, 1);
  graph->setClusterID(8, 1);
  graph->setClusterID(9, 1);
  graph->setClusterID(5, 2);
  graph->setClusterID(6, 2);
  graph->setClusterID(10, 2);

  std::vector<NodeID> node_ids(graph->numNodes());
  std::iota(node_ids.begin(), node_ids.end(), 0);

  std::sort(node_ids.begin(), node_ids.end(), [&](const NodeID& n1, const NodeID& n2) {
        return graph->_cluster_id[n1] < graph->_cluster_id[n2] || (graph->_cluster_id[n1] == graph->_cluster_id[n2] && n1 < n2);
      });

  auto cluster0 = std::make_pair(node_ids.begin(),
                                 node_ids.begin() + 3);
  auto cluster1 = std::make_pair(node_ids.begin() + 3,
                                 node_ids.begin() + 8);
  auto cluster2 = std::make_pair(node_ids.begin() + 8,
                                 node_ids.begin() + 11);

  // Checks incident clusters of cluster with ID 0
  std::vector<EdgeWeight> cluster_weight = { 2.0L, 0.25L, 1.0L / 3.0L };
  std::vector<bool> incident_cluster = { true, true, true };
  for (const auto& incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster0)) {
    const ClusterID c_id = incidentClusterWeight.clusterID;
    const EdgeWeight weight = incidentClusterWeight.weight;
    ASSERT_TRUE(incident_cluster[c_id]);
    ASSERT_LE(std::abs(cluster_weight[c_id] - weight), Graph::kEpsilon);
  }

  // Checks incident clusters of cluster with ID 1
  cluster_weight = { 1.0L / 4.0L, 1.5L + 4.0L / 3.0L, 1.0L / 3.0L };
  incident_cluster = { true, true, true };
  for (const auto& incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster1)) {
    const ClusterID c_id = incidentClusterWeight.clusterID;
    const EdgeWeight weight = incidentClusterWeight.weight;
    ASSERT_TRUE(incident_cluster[c_id]);
    ASSERT_LE(std::abs(cluster_weight[c_id] - weight), Graph::kEpsilon);
  }

  // Checks incident clusters of cluster with ID 2
  cluster_weight = { 1.0L / 3.0L, 1.0L / 3.0L, 4.0L / 3.0L };
  incident_cluster = { true, true, true };
  for (const auto& incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster2)) {
    const ClusterID c_id = incidentClusterWeight.clusterID;
    const EdgeWeight weight = incidentClusterWeight.weight;
    ASSERT_TRUE(incident_cluster[c_id]);
    ASSERT_LE(std::abs(cluster_weight[c_id] - weight), Graph::kEpsilon);
  }
}

TEST_F(ABipartiteGraph, ReturnsCorrectMappingToContractedGraph) {
  graph->setClusterID(2, 0);
  graph->setClusterID(7, 0);
  graph->setClusterID(1, 3);
  graph->setClusterID(4, 3);
  graph->setClusterID(8, 3);
  graph->setClusterID(9, 3);
  graph->setClusterID(5, 6);
  graph->setClusterID(10, 6);
  auto contractedGraph = graph->contractClusters();
  std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;
  std::vector<NodeID> correctMappingToOriginalGraph = { 0, 1, 0, 1, 1, 2, 2, 0, 1, 1, 2 };
  for (size_t i = 0; i < mappingToOriginalGraph.size(); ++i) {
    ASSERT_EQ(mappingToOriginalGraph[i], correctMappingToOriginalGraph[i]);
  }
}

TEST_F(ABipartiteGraph, ReturnCorrectContractedGraph) {
  graph->setClusterID(2, 0);
  graph->setClusterID(7, 0);
  graph->setClusterID(1, 3);
  graph->setClusterID(4, 3);
  graph->setClusterID(8, 3);
  graph->setClusterID(9, 3);
  graph->setClusterID(5, 6);
  graph->setClusterID(10, 6);
  Graph new_graph(std::move(graph->contractClusters().first));

  ASSERT_EQ(3, new_graph.numNodes());

  // Check cluster 0
  std::vector<EdgeWeight> edge_weight = { 2.0L, 0.25L, 1.0L / 3.0L };
  std::vector<bool> incident_nodes = { true, true, true };
  for (const Edge& e : new_graph.incidentEdges(0)) {
    const NodeID n_id = e.target_node;
    const EdgeWeight weight = e.weight;
    ASSERT_TRUE(incident_nodes[n_id]);
    ASSERT_LE(std::abs(edge_weight[n_id] - weight), Graph::kEpsilon);
  }

  // Check cluster 1
  edge_weight = { 1.0L / 4.0L, 1.5L + 4.0L / 3.0L, 1.0L / 3.0L };
  incident_nodes = { true, true, true };
  for (const Edge& e : new_graph.incidentEdges(1)) {
    const NodeID n_id = e.target_node;
    const EdgeWeight weight = e.weight;
    ASSERT_TRUE(incident_nodes[n_id]);
    ASSERT_LE(std::abs(edge_weight[n_id] - weight), Graph::kEpsilon);
  }

  // Check cluster 2
  edge_weight = { 1.0L / 3.0L, 1.0L / 3.0L, 4.0L / 3.0L };
  incident_nodes = { true, true, true };
  for (const Edge& e :  new_graph.incidentEdges(2)) {
    const NodeID n_id = e.target_node;
    const EdgeWeight weight = e.weight;
    ASSERT_TRUE(incident_nodes[n_id]);
    ASSERT_LE(std::abs(edge_weight[n_id] - weight), Graph::kEpsilon);
  }
}

TEST_F(ABipartiteGraph, HasCorrectSelfloopWeights) {
  graph->setClusterID(2, 0);
  graph->setClusterID(7, 0);
  graph->setClusterID(1, 3);
  graph->setClusterID(4, 3);
  graph->setClusterID(8, 3);
  graph->setClusterID(9, 3);
  graph->setClusterID(5, 6);
  graph->setClusterID(10, 6);
  Graph new_graph(std::move(graph->contractClusters().first));

  ASSERT_LE(std::abs(2.0L - new_graph.selfloopWeight(0)), Graph::kEpsilon);
  ASSERT_LE(std::abs((1.5L + 4.0L / 3.0L) - new_graph.selfloopWeight(1)), Graph::kEpsilon);
  ASSERT_LE(std::abs(4.0L / 3.0L - new_graph.selfloopWeight(2)), Graph::kEpsilon);
}
}  // namespace ds
}  // namespace kahypar
