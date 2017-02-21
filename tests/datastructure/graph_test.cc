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

#include <set>
#include <vector>

#include "gmock/gmock.h"

#include "kahypar/datastructure/graph.h"
#include "kahypar/definitions.h"

using::testing::Eq;
using::testing::Test;


namespace kahypar {
namespace ds {
#define INVALID -1
#define EPS 1e-5
#define INVALID_NODE std::numeric_limits<NodeID>::max()

class ABipartiteGraph : public Test {
 public:
  ABipartiteGraph() :
    graph(nullptr),
    config(),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) {
    config.preprocessing.louvain_community_detection.edge_weight = LouvainEdgeWeight::non_uniform;
    graph = std::make_shared<Graph>(hypergraph, config);
  }

  std::shared_ptr<Graph> graph;
  Configuration config;
  Hypergraph hypergraph;
};


bool bicoloring(NodeID cur, int cur_col, std::vector<int>& col, std::shared_ptr<Graph>& graph) {
  col[cur] = cur_col;
  for (Edge e : graph->incidentEdges(cur)) {
    NodeID id = e.target_node;
    if (col[id] == INVALID) return bicoloring(id, 1 - cur_col, col, graph);
    else if (col[id] == col[cur]) return false;
  }
  return true;
}


TEST_F(ABipartiteGraph, ConstructedFromAHypergraphIsBipartite) {
  bool isBipartite = true;
  std::vector<int> col(graph->numNodes(), INVALID);
  for (NodeID id : graph->nodes()) {
    if (col[id] == INVALID) isBipartite && bicoloring(id, 0, col, graph);
  }
  ASSERT_TRUE(isBipartite);
}


TEST_F(ABipartiteGraph, ConstructedFromAHypergraphIsEquivalentToHypergraph) {
  HypernodeID numNodes = hypergraph.initialNumNodes();
  for (HypernodeID hn : hypergraph.nodes()) {
    std::set<NodeID> edges;
    for (HyperedgeID he : hypergraph.incidentEdges(hn)) {
      edges.insert(numNodes + he);
    }
    ASSERT_EQ(edges.size(), graph->degree(hn));
    for (Edge e : graph->incidentEdges(hn)) {
      ASSERT_FALSE(edges.find(e.target_node) == edges.end());
    }
  }

  for (HyperedgeID he : hypergraph.edges()) {
    std::set<NodeID> edges;
    for (HypernodeID pin : hypergraph.pins(he)) {
      edges.insert(pin);
    }
    ASSERT_EQ(edges.size(), graph->degree(he + numNodes));
    for (Edge e : graph->incidentEdges(he + numNodes)) {
      ASSERT_FALSE(edges.find(e.target_node) == edges.end());
      ASSERT_EQ(e.weight, static_cast<EdgeWeight>(hypergraph.edgeWeight(he)) /
                static_cast<EdgeWeight>(hypergraph.edgeSize(he)));
    }
  }
}

TEST_F(ABipartiteGraph, HasCorrectTotalWeight) {
  ASSERT_LE(std::abs(8.0L - graph->totalWeight()), EPS);
}

TEST_F(ABipartiteGraph, HasCorrectInitializedClusterIDs) {
  for (NodeID id : graph->nodes()) {
    ASSERT_EQ(id, graph->clusterID(id));
  }
}

TEST_F(ABipartiteGraph, DeterminesIncidentClusterWeightsCorrect) {
  NodeID node = 8;
  std::vector<EdgeWeight> cluster_weight = { 0.25L, 0.25L, 0.0L, 0.25L, 0.25L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L, 0.0L };
  std::vector<bool> incident_cluster = { true, true, false, true, true, false, false, false, true, false, false };
  for (IncidentClusterWeight incidentClusterWeight : graph->incidentClusterWeightOfNode(node)) {
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
  for (IncidentClusterWeight incidentClusterWeight : graph->incidentClusterWeightOfNode(node)) {
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

  std::sort(graph->_shuffle_nodes.begin(), graph->_shuffle_nodes.end());
  std::sort(graph->_shuffle_nodes.begin(), graph->_shuffle_nodes.end(), [&](const NodeID& n1, const NodeID& n2) {
        return graph->_cluster_id[n1] < graph->_cluster_id[n2] || (graph->_cluster_id[n1] == graph->_cluster_id[n2] && n1 < n2);
      });

  auto cluster0 = std::make_pair(graph->_shuffle_nodes.begin(),
                                 graph->_shuffle_nodes.begin() + 3);
  auto cluster1 = std::make_pair(graph->_shuffle_nodes.begin() + 3,
                                 graph->_shuffle_nodes.begin() + 8);
  auto cluster2 = std::make_pair(graph->_shuffle_nodes.begin() + 8,
                                 graph->_shuffle_nodes.begin() + 11);

  //Checks incident clusters of cluster with ID 0
  std::vector<EdgeWeight> cluster_weight = { 2.0L, 0.25L, 1.0L / 3.0L };
  std::vector<bool> incident_cluster = { true, true, true };
  for (auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster0)) {
    ClusterID c_id = incidentClusterWeight.clusterID;
    EdgeWeight weight = incidentClusterWeight.weight;
    ASSERT_TRUE(incident_cluster[c_id]);
    ASSERT_LE(std::abs(cluster_weight[c_id] - weight), EPS);
  }

  //Checks incident clusters of cluster with ID 1
  cluster_weight = { 1.0L / 4.0L, 1.5L + 4.0L / 3.0L, 1.0L / 3.0L };
  incident_cluster = { true, true, true };
  for (auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster1)) {
    ClusterID c_id = incidentClusterWeight.clusterID;
    EdgeWeight weight = incidentClusterWeight.weight;
    ASSERT_TRUE(incident_cluster[c_id]);
    ASSERT_LE(std::abs(cluster_weight[c_id] - weight), EPS);
  }

  //Checks incident clusters of cluster with ID 2
  cluster_weight = { 1.0L / 3.0L, 1.0L / 3.0L, 4.0L / 3.0L };
  incident_cluster = { true, true, true };
  for (auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster2)) {
    ClusterID c_id = incidentClusterWeight.clusterID;
    EdgeWeight weight = incidentClusterWeight.weight;
    ASSERT_TRUE(incident_cluster[c_id]);
    ASSERT_LE(std::abs(cluster_weight[c_id] - weight), EPS);
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
  auto contractedGraph = graph->contractCluster();
  Graph graph(std::move(contractedGraph.first));
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
  auto contractedGraph = graph->contractCluster();
  Graph graph(std::move(contractedGraph.first));
  std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;

  ASSERT_EQ(3, graph.numNodes());

  //Check cluster 0
  std::vector<EdgeWeight> edge_weight = { 2.0L, 0.25L, 1.0L / 3.0L };
  std::vector<bool> incident_nodes = { true, true, true };
  for (Edge e : graph.incidentEdges(0)) {
    NodeID n_id = e.target_node;
    EdgeWeight weight = e.weight;
    ASSERT_TRUE(incident_nodes[n_id]);
    ASSERT_LE(std::abs(edge_weight[n_id] - weight), EPS);
  }

  //Check cluster 1
  edge_weight = { 1.0L / 4.0L, 1.5L + 4.0L / 3.0L, 1.0L / 3.0L };
  incident_nodes = { true, true, true };
  for (Edge e : graph.incidentEdges(1)) {
    NodeID n_id = e.target_node;
    EdgeWeight weight = e.weight;
    ASSERT_TRUE(incident_nodes[n_id]);
    ASSERT_LE(std::abs(edge_weight[n_id] - weight), EPS);
  }

  //Check cluster 2
  edge_weight = { 1.0L / 3.0L, 1.0L / 3.0L, 4.0L / 3.0L };
  incident_nodes = { true, true, true };
  for (Edge e : graph.incidentEdges(2)) {
    NodeID n_id = e.target_node;
    EdgeWeight weight = e.weight;
    ASSERT_TRUE(incident_nodes[n_id]);
    ASSERT_LE(std::abs(edge_weight[n_id] - weight), EPS);
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
  auto contractedGraph = graph->contractCluster();
  Graph graph(std::move(contractedGraph.first));
  std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;

  ASSERT_LE(std::abs(2.0L - graph.selfloopWeight(0)), EPS);
  ASSERT_LE(std::abs((1.5L + 4.0L / 3.0L) - graph.selfloopWeight(1)), EPS);
  ASSERT_LE(std::abs(4.0L / 3.0L - graph.selfloopWeight(2)), EPS);
}
}  //namespace ds
}  //namespace kahypar
