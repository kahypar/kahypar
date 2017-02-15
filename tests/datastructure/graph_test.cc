/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <vector>
#include <set>

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/datastructure/graph.h"

using::testing::Eq;
using::testing::Test;


namespace kahypar {
namespace ds {
    
#define INVALID -1
#define EPS 1e-5
#define INVALID_NODE std::numeric_limits<NodeID>::max()
   
class ABipartiteGraph : public Test {
public:
    ABipartiteGraph() : graph(nullptr), config(),
                        hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
                                   HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { 
        config.preprocessing.louvain_edge_weight = LouvainEdgeWeight::non_uniform;
        graph = std::make_shared<Graph>(hypergraph,config);
    }
                   
    std::shared_ptr<Graph> graph;
    Configuration config;
    Hypergraph hypergraph;
};

class ACliqueGraph : public Test {
public:
    ACliqueGraph() : graph(nullptr), config(),
                        hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
                                   HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { 
        config.preprocessing.louvain_use_bipartite_graph = false;
        config.preprocessing.louvain_edge_weight = LouvainEdgeWeight::non_uniform;
        graph = std::make_shared<Graph>(hypergraph,config);
    }
                   
    std::shared_ptr<Graph> graph;
    Configuration config;
    Hypergraph hypergraph;
};

bool bicoloring(NodeID cur, int cur_col, std::vector<int>& col, std::shared_ptr<Graph>& graph) {
    col[cur] = cur_col;
    for(Edge e : graph->adjacentNodes(cur)) {
        NodeID id = e.targetNode;
        if(col[id] == INVALID) return bicoloring(id,1-cur_col,col,graph);
        else if(col[id] == col[cur]) return false;
    }
    return true;
}


TEST_F(ABipartiteGraph, ConstructedFromAHypergraphIsBipartite) {
    bool isBipartite = true;
    std::vector<int> col(graph->numNodes(),INVALID);
    for(NodeID id : graph->nodes()) {
        if(col[id] == INVALID) isBipartite && bicoloring(id,0,col,graph);
    }
    ASSERT_TRUE(isBipartite);
}


TEST_F(ABipartiteGraph, ConstructedFromAHypergraphIsEquivalentToHypergraph) {
    HypernodeID numNodes = hypergraph.initialNumNodes();
    for(HypernodeID hn : hypergraph.nodes()) {
        std::set<NodeID> edges;
        for(HyperedgeID he : hypergraph.incidentEdges(hn)) {
            edges.insert(numNodes+he);
        }
        ASSERT_EQ(edges.size(),graph->degree(hn));
        for(Edge e : graph->adjacentNodes(hn)) {
            ASSERT_FALSE(edges.find(e.targetNode) == edges.end());
        }
    }
    
    for(HyperedgeID he : hypergraph.edges()) {
        std::set<NodeID> edges;
        for(HypernodeID pin : hypergraph.pins(he)) {
            edges.insert(pin);
        }
        ASSERT_EQ(edges.size(),graph->degree(he+numNodes));
        for(Edge e : graph->adjacentNodes(he+numNodes)) {
            ASSERT_FALSE(edges.find(e.targetNode) == edges.end());
            ASSERT_EQ(e.weight,static_cast<EdgeWeight>(hypergraph.edgeWeight(he))/
                               static_cast<EdgeWeight>(hypergraph.edgeSize(he)));
        }
    }
}

TEST_F(ACliqueGraph, ConstructedFromAHypergraphIsEquivalentToHypergraph) {
  std::vector<std::set<HypernodeID>> clique_graph(7,std::set<HypernodeID>());
  for(HypernodeID hn : hypergraph.nodes()) {
    for(HyperedgeID he : hypergraph.incidentEdges(hn)) {
      for(HypernodeID pin : hypergraph.pins(he)) {
        if(hn == pin) continue;
        clique_graph[hn].insert(pin);
      }
    }
  }
  for(NodeID node : graph->nodes()) {
    ASSERT_EQ(graph->degree(node),clique_graph[node].size());
    for(Edge e : graph->adjacentNodes(node)) {
      ASSERT_FALSE(clique_graph[node].find(e.targetNode) == clique_graph[node].end());
    }
  }
}

TEST_F(ABipartiteGraph,HasCorrectTotalWeight) {
    ASSERT_LE(std::abs(8.0L-graph->totalWeight()),EPS);
}

TEST_F(ACliqueGraph, HasCorrectTotalWeight) {
    ASSERT_LE(std::abs(8.0L-graph->totalWeight()),EPS);
}

TEST_F(ABipartiteGraph, HasCorrectInitializedClusterIDs) {
    for(NodeID id : graph->nodes()) {
        ASSERT_EQ(id,graph->clusterID(id));
    }
}

TEST_F(ACliqueGraph, HasCorrectInitializedClusterIDs) {
    for(NodeID id : graph->nodes()) {
        ASSERT_EQ(id,graph->clusterID(id));
    }
}

TEST_F(ABipartiteGraph, DeterminesIncidentClusterWeightsCorrect) {
    NodeID node = 8;
    std::vector<EdgeWeight> cluster_weight = {0.25L,0.25L,0.0L,0.25L,0.25L,0.0L,0.0L,0.0L,0.0L,0.0L,0.0L};
    std::vector<bool> incident_cluster = {true,true,false,true,true,false,false,false,true,false,false};
    for(IncidentClusterWeight incidentClusterWeight : graph->incidentClusterWeightOfNode(node)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight;
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_EQ(cluster_weight[c_id],weight);
    }
}

TEST_F(ACliqueGraph, DeterminesIncidentClusterWeightsCorrect) {
    NodeID node = 3;
    std::vector<EdgeWeight> cluster_weight = {1.0L/6.0L,1.0L/6.0L,0.0L,0.0L,3.0L/6.0L,0.0L,1.0L/3.0L};
    std::vector<bool> incident_cluster = {true,true,false,true,true,false,true};
    for(IncidentClusterWeight incidentClusterWeight : graph->incidentClusterWeightOfNode(node)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight;
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_EQ(cluster_weight[c_id],weight);
    }
}

TEST_F(ABipartiteGraph, DeterminesIncidentClusterWeightsCorrectAfterSomeNodesHasChangedTheirCluster) {
    NodeID node = 8;
    graph->setClusterID(1,0);
    graph->setClusterID(3,0);
    std::vector<EdgeWeight> cluster_weight = {0.75L,0.0L,0.0L,0.0L,0.25L,0.0L,0.0L,0.0L,0.0L,0.0L,0.0L};
    std::vector<bool> incident_cluster = {true,false,false,false,true,false,false,false,true,false,false};
    for(IncidentClusterWeight incidentClusterWeight : graph->incidentClusterWeightOfNode(node)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight;
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_EQ(cluster_weight[c_id],weight);
    }
} 

TEST_F(ACliqueGraph, DeterminesIncidentClusterWeightsCorrectAfterSomeNodesHasChangedTheirCluster) {
    NodeID node = 3;
    graph->setClusterID(1,0);
    graph->setClusterID(4,0);
    std::vector<EdgeWeight> cluster_weight = {5.0L/6.0L,0.0L,0.0L,0.0L,0.0L,0.0L,1.0L/3.0L};
    std::vector<bool> incident_cluster = {true,false,false,true,false,false,true};
    for(IncidentClusterWeight incidentClusterWeight : graph->incidentClusterWeightOfNode(node)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight;
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_LE(std::abs(cluster_weight[c_id]-weight), EPS);
    }
}

TEST_F(ABipartiteGraph, DeterminesIncidentClusterWeightsOfAClusterCorrect) {
    graph->setClusterID(2,0);
    graph->setClusterID(7,0);
    graph->setClusterID(1,1);
    graph->setClusterID(3,1);
    graph->setClusterID(4,1);
    graph->setClusterID(8,1);
    graph->setClusterID(9,1);
    graph->setClusterID(5,2);
    graph->setClusterID(6,2);
    graph->setClusterID(10,2);
    
    std::sort(graph->_shuffleNodes.begin(),graph->_shuffleNodes.end());
    std::sort(graph->_shuffleNodes.begin(),graph->_shuffleNodes.end(),[&](const NodeID& n1, const NodeID& n2) {
        return graph->_cluster_id[n1] < graph->_cluster_id[n2]  || (graph->_cluster_id[n1] == graph->_cluster_id[n2] && n1 < n2);
    });
    
    std::pair<NodeIterator,NodeIterator> cluster0 = std::make_pair(graph->_shuffleNodes.begin(),
                                                                   graph->_shuffleNodes.begin()+3);
    std::pair<NodeIterator,NodeIterator> cluster1 = std::make_pair(graph->_shuffleNodes.begin()+3,
                                                                   graph->_shuffleNodes.begin()+8);
    std::pair<NodeIterator,NodeIterator> cluster2 = std::make_pair(graph->_shuffleNodes.begin()+8,
                                                                   graph->_shuffleNodes.begin()+11);
    
    //Checks incident clusters of cluster with ID 0
    std::vector<EdgeWeight> cluster_weight = {2.0L,0.25L,1.0L/3.0L};
    std::vector<bool> incident_cluster = {true,true,true};
    for(auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster0)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight; 
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_LE(std::abs(cluster_weight[c_id]-weight),EPS);
    }
    
    //Checks incident clusters of cluster with ID 1
    cluster_weight = {1.0L/4.0L,1.5L+4.0L/3.0L,1.0L/3.0L};
    incident_cluster = {true,true,true};
    for(auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster1)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight; 
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_LE(std::abs(cluster_weight[c_id]-weight),EPS);
    }
    
    //Checks incident clusters of cluster with ID 2
    cluster_weight = {1.0L/3.0L,1.0L/3.0L,4.0L/3.0L};
    incident_cluster = {true,true,true};
    for(auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster2)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight; 
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_LE(std::abs(cluster_weight[c_id]-weight),EPS);
    }
}

TEST_F(ACliqueGraph, DeterminesIncidentClusterWeightsOfAClusterCorrect) {
    graph->setClusterID(2,0);
    graph->setClusterID(1,1);
    graph->setClusterID(3,1);
    graph->setClusterID(4,1);
    graph->setClusterID(5,2);
    graph->setClusterID(6,2);
    
    std::sort(graph->_shuffleNodes.begin(),graph->_shuffleNodes.end());
    std::sort(graph->_shuffleNodes.begin(),graph->_shuffleNodes.end(),[&](const NodeID& n1, const NodeID& n2) {
        return graph->_cluster_id[n1] < graph->_cluster_id[n2]  || (graph->_cluster_id[n1] == graph->_cluster_id[n2] && n1 < n2);
    });
    
    std::pair<NodeIterator,NodeIterator> cluster0 = std::make_pair(graph->_shuffleNodes.begin(),
                                                                   graph->_shuffleNodes.begin()+2);
    std::pair<NodeIterator,NodeIterator> cluster1 = std::make_pair(graph->_shuffleNodes.begin()+2,
                                                                   graph->_shuffleNodes.begin()+5);
    std::pair<NodeIterator,NodeIterator> cluster2 = std::make_pair(graph->_shuffleNodes.begin()+5,
                                                                   graph->_shuffleNodes.begin()+7);
    
    //Checks incident clusters of cluster with ID 0
    std::vector<EdgeWeight> cluster_weight = {2.0L,0.5L,2.0L/3.0L};
    std::vector<bool> incident_cluster = {true,true,true};
    for(auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster0)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight; 
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_LE(std::abs(cluster_weight[c_id]-weight),EPS);
    }
    
    //Checks incident clusters of cluster with ID 1
    cluster_weight = {0.5L,5.0L/3.0L,2.0L/3.0L};
    incident_cluster = {true,true,true};
    for(auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster1)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight; 
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_LE(std::abs(cluster_weight[c_id]-weight),EPS);
    }
    
    //Checks incident clusters of cluster with ID 2
    cluster_weight = {2.0L/3.0L,2.0L/3.0L,2.0L/3.0L};
    incident_cluster = {true,true,true};
    for(auto incidentClusterWeight : graph->incidentClusterWeightOfCluster(cluster2)) {
        ClusterID c_id = incidentClusterWeight.clusterID;
        EdgeWeight weight = incidentClusterWeight.weight; 
        ASSERT_TRUE(incident_cluster[c_id]);
        ASSERT_LE(std::abs(cluster_weight[c_id]-weight),EPS);
    }
}

TEST_F(ABipartiteGraph, ReturnsCorrectMappingToContractedGraph) {
    graph->setClusterID(2,0);
    graph->setClusterID(7,0);
    graph->setClusterID(1,3);
    graph->setClusterID(4,3);
    graph->setClusterID(8,3);
    graph->setClusterID(9,3);
    graph->setClusterID(5,6);
    graph->setClusterID(10,6);
    auto contractedGraph = graph->contractCluster();
    Graph graph(std::move(contractedGraph.first));
    std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;
    std::vector<NodeID> correctMappingToOriginalGraph = {0,1,0,1,1,2,2,0,1,1,2};
    for(size_t i = 0; i < mappingToOriginalGraph.size(); ++i) {
        ASSERT_EQ(mappingToOriginalGraph[i],correctMappingToOriginalGraph[i]);
    }
}

TEST_F(ACliqueGraph, ReturnsCorrectMappingToContractedGraph) {
    graph->setClusterID(2,0);
    graph->setClusterID(1,3);
    graph->setClusterID(4,3);
    graph->setClusterID(5,6);
    auto contractedGraph = graph->contractCluster();
    Graph graph(std::move(contractedGraph.first));
    std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;
    std::vector<NodeID> correctMappingToOriginalGraph = {0,1,0,1,1,2,2};
    for(size_t i = 0; i < mappingToOriginalGraph.size(); ++i) {
        ASSERT_EQ(mappingToOriginalGraph[i],correctMappingToOriginalGraph[i]);
    }
}

TEST_F(ABipartiteGraph, ReturnCorrectContractedGraph) {
    graph->setClusterID(2,0);
    graph->setClusterID(7,0);
    graph->setClusterID(1,3);
    graph->setClusterID(4,3);
    graph->setClusterID(8,3);
    graph->setClusterID(9,3);
    graph->setClusterID(5,6);
    graph->setClusterID(10,6);
    auto contractedGraph = graph->contractCluster();
    Graph graph(std::move(contractedGraph.first));
    std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;
    
    ASSERT_EQ(3,graph.numNodes());
    
    //Check cluster 0
    std::vector<EdgeWeight> edge_weight = {2.0L,0.25L,1.0L/3.0L};
    std::vector<bool> incident_nodes = {true,true,true};
    for(Edge e : graph.adjacentNodes(0)) {
        NodeID n_id = e.targetNode;
        EdgeWeight weight = e.weight; 
        ASSERT_TRUE(incident_nodes[n_id]);
        ASSERT_LE(std::abs(edge_weight[n_id]-weight),EPS);           
    }
    
    //Check cluster 1
    edge_weight = {1.0L/4.0L,1.5L+4.0L/3.0L,1.0L/3.0L};
    incident_nodes = {true,true,true};
    for(Edge e : graph.adjacentNodes(1)) {
        NodeID n_id = e.targetNode;
        EdgeWeight weight = e.weight; 
        ASSERT_TRUE(incident_nodes[n_id]);
        ASSERT_LE(std::abs(edge_weight[n_id]-weight),EPS);           
    }
    
    //Check cluster 2
    edge_weight = {1.0L/3.0L,1.0L/3.0L,4.0L/3.0L};
    incident_nodes = {true,true,true};
    for(Edge e : graph.adjacentNodes(2)) {
        NodeID n_id = e.targetNode;
        EdgeWeight weight = e.weight; 
        ASSERT_TRUE(incident_nodes[n_id]);
        ASSERT_LE(std::abs(edge_weight[n_id]-weight),EPS);           
    }
    
}

TEST_F(ACliqueGraph, ReturnCorrectContractedGraph) {
    graph->setClusterID(2,0);
    graph->setClusterID(1,3);
    graph->setClusterID(4,3);
    graph->setClusterID(5,6);
    auto contractedGraph = graph->contractCluster();
    Graph graph(std::move(contractedGraph.first));
    std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;
    
    ASSERT_EQ(3,graph.numNodes());
    
    //Check cluster 0
    std::vector<EdgeWeight> edge_weight = {2.0L,0.5L,2.0L/3.0L};
    std::vector<bool> incident_nodes = {true,true,true};
    for(Edge e : graph.adjacentNodes(0)) {
        NodeID n_id = e.targetNode;
        EdgeWeight weight = e.weight; 
        ASSERT_TRUE(incident_nodes[n_id]);
        ASSERT_LE(std::abs(edge_weight[n_id]-weight),EPS);           
    }
    
    //Check cluster 1
    edge_weight = {0.5L,5.0L/3.0L,2.0L/3.0L};
    incident_nodes = {true,true,true};
    for(Edge e : graph.adjacentNodes(1)) {
        NodeID n_id = e.targetNode;
        EdgeWeight weight = e.weight; 
        ASSERT_TRUE(incident_nodes[n_id]);
        ASSERT_LE(std::abs(edge_weight[n_id]-weight),EPS);           
    }
    
    //Check cluster 2
    edge_weight = {2.0L/3.0L,2.0L/3.0L,2.0L/3.0L};
    incident_nodes = {true,true,true};
    for(Edge e : graph.adjacentNodes(2)) {
        NodeID n_id = e.targetNode;
        EdgeWeight weight = e.weight; 
        ASSERT_TRUE(incident_nodes[n_id]);
        ASSERT_LE(std::abs(edge_weight[n_id]-weight),EPS);           
    }
}

TEST_F(ABipartiteGraph, HasCorrectSelfloopWeights) {
    graph->setClusterID(2,0);
    graph->setClusterID(7,0);
    graph->setClusterID(1,3);
    graph->setClusterID(4,3);
    graph->setClusterID(8,3);
    graph->setClusterID(9,3);
    graph->setClusterID(5,6);
    graph->setClusterID(10,6);
    auto contractedGraph = graph->contractCluster();
    Graph graph(std::move(contractedGraph.first));
    std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;
    
    ASSERT_LE(std::abs(2.0L-graph.selfloopWeight(0)),EPS);
    ASSERT_LE(std::abs((1.5L+4.0L/3.0L)-graph.selfloopWeight(1)),EPS);
    ASSERT_LE(std::abs(4.0L/3.0L-graph.selfloopWeight(2)),EPS);
}

TEST_F(ACliqueGraph, HasCorrectSelfloopWeights) {
    graph->setClusterID(2,0);
    graph->setClusterID(1,3);
    graph->setClusterID(4,3);
    graph->setClusterID(5,6);
    auto contractedGraph = graph->contractCluster();
    Graph graph(std::move(contractedGraph.first));
    std::vector<NodeID> mappingToOriginalGraph = contractedGraph.second;
    
    ASSERT_LE(std::abs(2.0L-graph.selfloopWeight(0)),EPS);
    ASSERT_LE(std::abs((5.0L/3.0L)-graph.selfloopWeight(1)),EPS);
    ASSERT_LE(std::abs(2.0L/3.0L-graph.selfloopWeight(2)),EPS);
}



} //namespace ds
} //namespace kahypar