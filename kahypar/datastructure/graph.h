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

#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <vector>
#include <set>
#include <queue>
#include <map>

#include "gtest/gtest_prod.h"

#include "kahypar/macros.h"

#include "kahypar/definitions.h"
#include "kahypar/datastructure/sparse_map.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/utils/randomize.h"
#include "kahypar/partition/configuration.h"

namespace kahypar {
namespace ds {

#define INVALID_NODE std::numeric_limits<NodeID>::max()
#define INVALID_CLUSTER std::numeric_limits<ClusterID>::max()
#define EPS 1e-5

class UnionFind {
public:
    std::vector<NodeID> parent;
        
    UnionFind(size_t n) : parent(n) {
        std::iota(parent.begin(),parent.end(),0);
    }
        
    NodeID findSet(NodeID n) { // Pfadkompression
        if (parent[n] != n) parent[n] = findSet(parent[n]);
        return parent[n];
    }
        
    void linkSets(NodeID a, NodeID b) { // Union by rank.
        parent[b] = a;
    }
        
    void unionSets(NodeID a, NodeID b) { // Diese Funktion aufrufen.
        if (findSet(a) != findSet(b)) linkSets(findSet(a), findSet(b));
    }
    
    void reset() {
        std::iota(parent.begin(),parent.end(),0);   
    }
    
};    

struct Edge {
    
    Edge() : targetNode(0), weight(0.0), bfs_cnt(0), reverse_edge(nullptr) { }
    
    NodeID targetNode;
    EdgeWeight weight;
    size_t bfs_cnt;
    Edge* reverse_edge;
};

struct IncidentClusterWeight {
    ClusterID clusterID;
    EdgeWeight weight;
    
    IncidentClusterWeight(ClusterID clusterID, EdgeWeight weight) 
                         : clusterID(clusterID), weight(weight) { }
};


using NodeIterator = std::vector<NodeID>::const_iterator;
using EdgeIterator = std::vector<Edge>::const_iterator;
using IncidentClusterWeightIterator = std::vector<IncidentClusterWeight>::const_iterator;

    
class Graph {
    
public:
    
    Graph(const Hypergraph& hypergraph, const Configuration& config) 
    : _N(hypergraph.currentNumNodes()+(config.preprocessing.louvain_community_detection.use_bipartite_graph ? hypergraph.currentNumEdges() : 0)),
                                          _config(config), _adj_array(_N+1), _nodes(_N), _shuffleNodes(_N), _edges(), 
                                          _selfloop(_N,0.0L), _weightedDegree(_N,0.0L), _cluster_id(_N), _cluster_size(_N,1), _num_comm(_N),
                                          _incidentClusterWeight(_N,IncidentClusterWeight(0,0.0L)),
                                          _total_weight(0.0L), _posInIncidentClusterWeightVector(_N),
                                          _hypernodeMapping(hypergraph.initialNumNodes()+hypergraph.initialNumEdges(),INVALID_NODE) {
        std::iota(_nodes.begin(),_nodes.end(),0);
        std::iota(_shuffleNodes.begin(),_shuffleNodes.end(),0);
        std::iota(_cluster_id.begin(),_cluster_id.end(),0);
        if(_config.preprocessing.louvain_community_detection.use_bipartite_graph) {
            constructBipartiteGraph(hypergraph);
        }
        else {
            constructCliqueGraph(hypergraph);
        }
    }
    
    Graph(const std::vector<NodeID>& adj_array, const std::vector<Edge>& edges, const Configuration& config) 
                : _N(adj_array.size()-1), _config(config), _adj_array(adj_array), _nodes(_N), _shuffleNodes(_N), _edges(edges), _selfloop(_N,0.0L),
                  _weightedDegree(_N,0.0L) , _cluster_id(_N), _cluster_size(_N,1), _num_comm(_N), _incidentClusterWeight(_N,IncidentClusterWeight(0,0.0L)),
                  _total_weight(0.0L), _posInIncidentClusterWeightVector(_N), _hypernodeMapping(_N,INVALID_NODE)  {
        std::iota(_nodes.begin(),_nodes.end(),0);
        std::iota(_shuffleNodes.begin(),_shuffleNodes.end(),0);
        std::iota(_cluster_id.begin(),_cluster_id.end(),0);
        std::iota(_hypernodeMapping.begin(),_hypernodeMapping.end(),0);
        
        for(NodeID node : nodes()) {
            for(Edge e : adjacentNodes(node)) {
                if(node == e.targetNode) {
                    _selfloop[node] = e.weight;
                }
                _total_weight += e.weight;
                _weightedDegree[node] += e.weight;
            }
        }
    }
    
    Graph(const std::vector<NodeID>& adj_array, const std::vector<Edge>& edges, const std::vector<NodeID> hypernodeMapping, const std::vector<ClusterID> cluster_id, const Configuration& config) 
                : _N(adj_array.size()-1), _config(config), _adj_array(adj_array), _nodes(_N), _shuffleNodes(_N), _edges(edges), _selfloop(_N,0.0L),
                  _weightedDegree(_N,0.0L) , _cluster_id(cluster_id), _cluster_size(_N,0), _num_comm(0), _incidentClusterWeight(_N,IncidentClusterWeight(0,0.0L)),
                  _total_weight(0.0L), _posInIncidentClusterWeightVector(_N), _hypernodeMapping(hypernodeMapping) {
        std::iota(_nodes.begin(),_nodes.end(),0);
        std::iota(_shuffleNodes.begin(),_shuffleNodes.end(),0);
        
        for(NodeID node : nodes()) {
            if(_cluster_size[_cluster_id[node]] == 0) _num_comm++;
            _cluster_size[_cluster_id[node]]++;
            for(Edge e : adjacentNodes(node)) {
                if(node == e.targetNode) {
                    _selfloop[node] = e.weight;
                }
                _total_weight += e.weight;
                _weightedDegree[node] += e.weight;
            }
        }
    }
    
    Graph(Graph&& other) : _N(std::move(other._N)), _config(other._config), _adj_array(std::move(other._adj_array)), _nodes(std::move(other._nodes)),
                           _shuffleNodes(std::move(other._shuffleNodes)),_edges(std::move(other._edges)), _selfloop(std::move(other._selfloop)),
                           _weightedDegree(std::move(other._weightedDegree)),_cluster_id(std::move(other._cluster_id)), _cluster_size(std::move(other._cluster_size)),
                           _num_comm(std::move(other._num_comm)), _incidentClusterWeight(_N,IncidentClusterWeight(0,0.0L)), _total_weight(std::move(other._total_weight)), 
                           _posInIncidentClusterWeightVector(std::move(other._posInIncidentClusterWeightVector)),
                           _hypernodeMapping(std::move(other._hypernodeMapping)) { }
    
    Graph(const Graph& other): _N(other._N), _config(other._config), _adj_array(other._adj_array), _nodes(other._nodes), _shuffleNodes(other._shuffleNodes),
                               _edges(other._edges), _selfloop(other._selfloop),
                               _weightedDegree(other._weightedDegree),_cluster_id(other._cluster_id),_cluster_size(other._cluster_size),
                               _num_comm(other._num_comm),  _incidentClusterWeight(_N,IncidentClusterWeight(0,0.0L)),
                               _total_weight(other._total_weight), _posInIncidentClusterWeightVector(_N), _hypernodeMapping(other._hypernodeMapping)  { }
                               
    Graph& operator=(Graph& other) {
        _N = other._N;
        _adj_array = other._adj_array;
        _nodes = other._nodes;
        _shuffleNodes = other._shuffleNodes;
        _edges = other._edges;
        _selfloop = other._selfloop;
        _weightedDegree = other._weightedDegree;
        _cluster_id = other._cluster_id;
        _cluster_size = other._cluster_size;
        _num_comm = other._num_comm;
        _incidentClusterWeight.assign(_N,IncidentClusterWeight(0,0.0L));
        _total_weight = other._total_weight;
        _posInIncidentClusterWeightVector = other._posInIncidentClusterWeightVector;
        _hypernodeMapping = other._hypernodeMapping;
        return *this;
    }

    Graph& operator=(Graph&& other) {
        _N = std::move(other._N);
        _adj_array = std::move(other._adj_array);
        _nodes = std::move(other._nodes);
        _shuffleNodes = std::move(other._shuffleNodes);
        _edges = std::move(other._edges);
        _selfloop = std::move(other._selfloop);
        _weightedDegree = std::move(other._weightedDegree);
        _cluster_id = std::move(other._cluster_id);
        _cluster_size = std::move(other._cluster_size);
        _num_comm = std::move(other._num_comm);
        _incidentClusterWeight.assign(_N,IncidentClusterWeight(0,0.0L));
        _total_weight = std::move(other._total_weight);
        _posInIncidentClusterWeightVector = other._posInIncidentClusterWeightVector;
        _hypernodeMapping = std::move(other._hypernodeMapping);   
        _posInIncidentClusterWeightVector.clear();
        return *this;
    }
    
    
    
    std::pair<NodeIterator,NodeIterator> nodes() const {
        return std::make_pair(_nodes.begin(),_nodes.end());
    }
    
    std::pair<NodeIterator,NodeIterator> randomNodeOrder() {
        Randomize::instance().shuffleVector(_shuffleNodes, _shuffleNodes.size());
        return std::make_pair(_shuffleNodes.begin(),_shuffleNodes.end());
    }
    
    void shuffleNodes() {
        Randomize::instance().shuffleVector(_shuffleNodes, _shuffleNodes.size());
    }

    
    std::pair<EdgeIterator,EdgeIterator> adjacentNodes(const NodeID node) const {
        ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
        return std::make_pair(_edges.cbegin()+_adj_array[node],_edges.cbegin()+_adj_array[node+1]);
    }
    
    size_t numNodes() const {
        return static_cast<size_t>(_N);
    }
    
    size_t numEdges() const {
        return _edges.size();
    }
    
    size_t degree(const NodeID node) const  {
        ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
        return static_cast<size_t>(_adj_array[node+1]-_adj_array[node]);
    }
    
    EdgeWeight weightedDegree(const NodeID node) const {
        ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
        return _weightedDegree[node];
    }
    
    EdgeWeight selfloopWeight(const NodeID node) const {
        ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
        return _selfloop[node];
    }
    
    
    EdgeWeight totalWeight() const {
        return _total_weight;
    }
    
    void setHypernodeClusterID(const HypernodeID hn, const ClusterID c_id) {
        ASSERT(_hypernodeMapping[hn] != INVALID_NODE, "Hypernode " << hn << " isn't part of the current graph!");
        _cluster_id[_hypernodeMapping[hn]] = c_id;
    }
    
    void setHyperedgeClusterID(const HyperedgeID he, const ClusterID c_id, const size_t N) {
        ASSERT(_hypernodeMapping[N+he] != INVALID_NODE, "Hyperedge " << he << " isn't part of the current graph!");
        _cluster_id[_hypernodeMapping[N+he]] = c_id;
    }
    
    ClusterID hypernodeClusterID(const HypernodeID hn) const {
        ASSERT(_hypernodeMapping[hn] != INVALID_NODE, "Hypernode " << hn << " isn't part of the current graph!");
        return _cluster_id[_hypernodeMapping[hn]];
    }
    
    
    ClusterID hyperedgeClusterID(const HyperedgeID he, const size_t N) const {
        ASSERT(_hypernodeMapping[N+he] != INVALID_NODE, "Hyperedge " << he << " isn't part of the current graph!");  
        return _cluster_id[_hypernodeMapping[N+he]];
    }
    
    size_t numCommunities() const {
      return _num_comm;
    }
    
    size_t clusterSize(const ClusterID cid) const {
      return _cluster_size[cid];
    }
    
    ClusterID clusterID(const NodeID node) const  {
        ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
        return _cluster_id[node];
    }
    
    void setClusterID(const NodeID node, const ClusterID c_id) {
        ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
        
        ClusterID from = _cluster_id[node];
        ClusterID to = c_id;
        
        if(from != -1 && from != to && _cluster_size[from] == 1 ) {
          _num_comm--;
        } 
        if(to != -1 && from != to && _cluster_size[to] == 0) {
          _num_comm++;
        }
        
        if(to != -1) {
          _cluster_size[to]++;
        }
        if(from != -1) {
          _cluster_size[from]--; 
        }
        
        _cluster_id[node] = to;
        
        ASSERT([&]() {
          std::set<ClusterID> distinct_comm;
          size_t from_size = 0; size_t to_size = 0;
          
          for(NodeID node : nodes()) {
            if(clusterID(node) != -1) distinct_comm.insert(clusterID(node));
            if(from != -1 && clusterID(node) == from) from_size++;
            if(to != -1 && clusterID(node) == to) to_size++;
          }
          if(distinct_comm.size() != _num_comm) {
            LOGVAR(_num_comm);
            LOGVAR(distinct_comm.size());
            return false;
          }
          else if(to != -1 && to_size != _cluster_size[to]) {
            LOGVAR(to_size);
            LOGVAR(_cluster_size[to]);
            return false;
          }
          else if(from != -1 && from_size != _cluster_size[from]) {
            LOGVAR(from_size);
            LOGVAR(_cluster_size[from]);
            return false;
          }
          return true;
        }());
        
    }
   
    
    /**
     * Creates an iterator to all incident Clusters of Node node. Iterator points to an 
     * IncidentClusterWeight-Struct which contains the incident Cluster ID and the sum of
     * the weight of all incident edges to that cluster.
     * 
     * @param node NodeID, which incident clusters should be evaluated
     * @return Iterator to all incident clusters of NodeID node
     */
    std::pair<IncidentClusterWeightIterator,IncidentClusterWeightIterator> incidentClusterWeightOfNode(const NodeID node);
    
   
    
    /**
     * Contracts the Graph based on the nodes ClusterIDs. Nodes with same ClusterID are contracted
     * in a single node in the contracted graph. Edges are inserted based on the sum of the weight
     * of all edges which connects two clusters. Also a mapping is created which maps the nodes of
     * the graph to the corresponding contracted nodes.
     * 
     * @return Pair which contains the contracted graph and a mapping from current to nodes to its
     * corresponding contrated nodes.
     */
    std::pair<Graph,std::vector<NodeID>> contractCluster();
    
    void printGraph() {
        
        std::cout << "Number Nodes: " << numNodes() << std::endl;
        std::cout << "Number Edges: " << numEdges() << std::endl;
        
        for(NodeID n : nodes()) {
            std::cout << "Node ID: " << n << "(Comm.: "<<clusterID(n)<<"), Adj. List: ";
            for(Edge e : adjacentNodes(n)) {
                std::cout << "(" << e.targetNode << ",w=" << e.weight<<") ";
            }
            std::cout << "\n";
        }
        
    }
    
protected:
    
    NodeID _N;
    const Configuration& _config;
    std::vector<NodeID> _adj_array;
    std::vector<NodeID> _nodes;
    std::vector<NodeID> _shuffleNodes;
    std::vector<Edge> _edges;
    std::vector<EdgeWeight> _selfloop;
    std::vector<EdgeWeight> _weightedDegree;
    std::vector<ClusterID> _cluster_id;
    std::vector<size_t> _cluster_size;
    size_t _num_comm;
    std::vector<IncidentClusterWeight> _incidentClusterWeight;
    EdgeWeight _total_weight;
    SparseMap<ClusterID,size_t> _posInIncidentClusterWeightVector;
    std::vector<NodeID> _hypernodeMapping;
    
private:
    FRIEND_TEST(ABipartiteGraph,DeterminesIncidentClusterWeightsOfAClusterCorrect);
    FRIEND_TEST(ACliqueGraph,DeterminesIncidentClusterWeightsOfAClusterCorrect);
    
    
    /**
     * Creates an iterator to all incident Clusters of ClusterID cid. Iterator points to an 
     * IncidentClusterWeight-Struct which contains the incident ClusterID clusterID and the sum of
     * the weights of all incident edges from cid to clusterID.
     * 
     * @param cid ClusterID, which incident clusters should be evaluated
     * @return Iterator to all incident clusters of ClusterID cid
     */
    std::pair<IncidentClusterWeightIterator,IncidentClusterWeightIterator> incidentClusterWeightOfCluster(std::pair<NodeIterator,NodeIterator>& cluster_range);
    
    
    void constructBipartiteGraph(const Hypergraph& hg) {
        NodeID sum_edges = 0;
        
        size_t N = static_cast<size_t>(hg.initialNumNodes());
        
        NodeID cur_node_id = 0;
        
        //Construct adj. array for all hypernode. Amount of edges is equal to the degree of the corresponding hypernode.
        for(HypernodeID hn : hg.nodes()) {
            _hypernodeMapping[hn] = cur_node_id;
            _adj_array[cur_node_id++] = sum_edges;
            sum_edges += hg.nodeDegree(hn);
        }
        
        //Construct adj. array for all hyperedges. Amount of edges is equal to the size of the corresponding hyperedge.
        for(HyperedgeID he : hg.edges()) {
            _hypernodeMapping[N+he] = cur_node_id;
            _adj_array[cur_node_id++] = sum_edges;
            sum_edges += hg.edgeSize(he);
        }
        
        _adj_array[_N] = sum_edges;
        _edges.resize(sum_edges);
        
        for(HypernodeID hn : hg.nodes()) {
            
            size_t pos = 0;
            for(HyperedgeID he : hg.incidentEdges(hn)) {
                Edge e;
                e.targetNode = _hypernodeMapping[N + he];
                if(_config.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::degree) {
                    e.weight = (static_cast<EdgeWeight>(hg.edgeWeight(he))*static_cast<EdgeWeight>(hg.nodeDegree(hn)))/
                               static_cast<EdgeWeight>(hg.edgeSize(he));
                }
                else if(_config.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::non_uniform) {
                    e.weight = (static_cast<EdgeWeight>(hg.edgeWeight(he)))/
                               static_cast<EdgeWeight>(hg.edgeSize(he));
                } else {
                    e.weight = static_cast<EdgeWeight>(hg.edgeWeight(he));
                }
                _total_weight += e.weight;
                _weightedDegree[_hypernodeMapping[hn]] += e.weight;
                _edges[_adj_array[_hypernodeMapping[hn]] + pos++] = e;
            }
        }
        
        for(HyperedgeID he : hg.edges()) {
           size_t pos = 0;
           for(HypernodeID hn : hg.pins(he)) {
                Edge e;
                e.targetNode = _hypernodeMapping[hn];
                if(_config.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::degree) {
                    e.weight = (static_cast<EdgeWeight>(hg.edgeWeight(he))*static_cast<EdgeWeight>(hg.nodeDegree(hn)))/
                               static_cast<EdgeWeight>(hg.edgeSize(he));
                }
                else if(_config.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::non_uniform) {
                    e.weight = (static_cast<EdgeWeight>(hg.edgeWeight(he)))/
                               static_cast<EdgeWeight>(hg.edgeSize(he));
                } else {
                    e.weight = static_cast<EdgeWeight>(hg.edgeWeight(he));
                }
                NodeID cur_node = _hypernodeMapping[N + he];
                _total_weight += e.weight;
                _weightedDegree[cur_node] += e.weight;
                _edges[_adj_array[cur_node]+pos++] = e;
                for(size_t i = _adj_array[e.targetNode]; i < _adj_array[e.targetNode+1]; ++i) {
                    if(_edges[i].targetNode == cur_node) {
                        _edges[i].reverse_edge = &_edges[_adj_array[cur_node]+pos-1];
                        _edges[_adj_array[cur_node]+pos-1].reverse_edge = &_edges[i];
                        break;
                    }
                }
           }
        }
       
        
        ASSERT([&]() {
          //Check Hypernodes in Graph
          for(HypernodeID hn : hg.nodes()) {
            if(hg.nodeDegree(hn) != degree(_hypernodeMapping[hn])) { 
              LOGVAR(hg.nodeDegree(hn));
              LOGVAR(degree(_hypernodeMapping[hn]));
              return false;
            }
            std::set<HyperedgeID> incident_edges;
            for(HyperedgeID he : hg.incidentEdges(hn)) {
                incident_edges.insert(_hypernodeMapping[N + he]);
            }
            for(Edge e : adjacentNodes(_hypernodeMapping[hn])) {
              HyperedgeID he = e.targetNode;
              if(incident_edges.find(he) == incident_edges.end()) {
                LOGVAR(_hypernodeMapping[hn]);
                LOGVAR(he);
                return false;
              }
            }
          }
          
          //Checks Hyperedges in Graph
          for(HyperedgeID he : hg.edges()) {
            if(hg.edgeSize(he) != degree(_hypernodeMapping[he+N])) { 
              LOGVAR(hg.edgeSize(he));
              LOGVAR(degree(_hypernodeMapping[he+N]));
              return false;
            }
            std::set<HypernodeID> pins;
            for(HypernodeID hn : hg.pins(he)) {
              pins.insert(_hypernodeMapping[hn]);
            }
            for(Edge e : adjacentNodes(_hypernodeMapping[he+N])) {
              if(pins.find(e.targetNode) == pins.end()) {
                LOGVAR(_hypernodeMapping[he+N]);
                LOGVAR(e.targetNode);
                return false;
              }
            }
          }
          return true;
        }(),"Bipartite Graph is not equivalent with hypergraph");
        
    }
    
    void constructCliqueGraph(const Hypergraph& hg) {
        NodeID sum_edges = 0;
        NodeID cur_node_id = 0;
        
        for(HypernodeID hn : hg.nodes()) {
            _hypernodeMapping[hn] = cur_node_id++;
        }
        
        for(HypernodeID hn : hg.nodes()) {
            
            NodeID cur_node = _hypernodeMapping[hn];
            _adj_array[cur_node] = sum_edges;
            std::vector<Edge> tmp_edges;
            
            for(HyperedgeID he : hg.incidentEdges(hn)) {
                for(HypernodeID pin : hg.pins(he)) {
                    if(hn == pin) continue;
                    Edge e; NodeID v = _hypernodeMapping[pin];
                    e.targetNode = v;
                    if(_config.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::degree) {
                        e.weight = static_cast<EdgeWeight>(hg.edgeWeight(he)*hg.nodeDegree(hn))/
                        (static_cast<EdgeWeight>(hg.edgeSize(he)
                        *(static_cast<EdgeWeight>(hg.edgeSize(he)-1.0)/2.0)));
                    }
                    else if(_config.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::non_uniform) {
                        e.weight = static_cast<EdgeWeight>(hg.edgeWeight(he))/
                        (static_cast<EdgeWeight>(hg.edgeSize(he)
                        *(static_cast<EdgeWeight>(hg.edgeSize(he)-1.0)/2.0)));
                    } else {
                        e.weight = static_cast<EdgeWeight>(hg.edgeWeight(he));
                    }
                    tmp_edges.push_back(e);
                    _total_weight += e.weight;
                    _weightedDegree[cur_node] += e.weight;
                }
            }
            
            std::sort(tmp_edges.begin(),tmp_edges.end(),[&](const Edge& e1, const Edge& e2) {
                return e1.targetNode < e2.targetNode;
            });
            Edge sentinel; sentinel.targetNode = INVALID_NODE;
            tmp_edges.push_back(sentinel);
            if(tmp_edges.size() > 1) {
                Edge cur_edge;
                cur_edge.targetNode = tmp_edges[0].targetNode;
                cur_edge.weight = tmp_edges[0].weight;
                for(size_t i = 1; i < tmp_edges.size(); ++i) {
                    if(cur_edge.targetNode == tmp_edges[i].targetNode) {
                        cur_edge.weight += tmp_edges[i].weight;
                    }
                    else {
                        _edges.push_back(cur_edge);
                        cur_edge.targetNode = tmp_edges[i].targetNode;
                        cur_edge.weight = tmp_edges[i].weight;
                    }
                }
                sum_edges = _edges.size();
            }
            
        }
        
        _adj_array[_N] = sum_edges;
  
        ASSERT([&]() {
          for(HypernodeID hn : hg.nodes()) {
            std::set<HypernodeID> incident_nodes;
            for(HyperedgeID he : hg.incidentEdges(hn)) {
              for(HypernodeID pin : hg.pins(he)) {
                incident_nodes.insert(pin);
              }
            }
            incident_nodes.erase(hn); 
            if(incident_nodes.size() != degree(hn)) {
              LOGVAR(incident_nodes.size());
              LOGVAR(degree(hn));
              return false;
            }
            for(Edge e : adjacentNodes(hn)) {
              if(incident_nodes.find(e.targetNode) == incident_nodes.end()) {
                LOGVAR(hn);
                LOGVAR(e.targetNode);
                return false;
              }
            }
          }
          return true;
        }(), "Clique Graph is not equivalent with Hypergraph");
        
    }
    
};

std::pair<IncidentClusterWeightIterator,IncidentClusterWeightIterator> Graph::incidentClusterWeightOfNode(const NodeID node) {
    
    _posInIncidentClusterWeightVector.clear();
    size_t idx = 0;
    
    if(clusterID(node) != -1) {
        _incidentClusterWeight[idx] = IncidentClusterWeight(clusterID(node),0.0L);
        _posInIncidentClusterWeightVector[clusterID(node)] = idx++;
    }
    
    for(Edge e : adjacentNodes(node)) {
        NodeID id = e.targetNode;
        EdgeWeight w = e.weight;
        ClusterID c_id = clusterID(id);
        if(c_id != -1) {
            if(_posInIncidentClusterWeightVector.contains(c_id)) {
                size_t i = _posInIncidentClusterWeightVector[c_id];
                _incidentClusterWeight[i].weight += w;
            }
            else {
                _incidentClusterWeight[idx] = IncidentClusterWeight(c_id,w);
                _posInIncidentClusterWeightVector[c_id] = idx++;
            }
        }
    }
    
    auto it = std::make_pair(_incidentClusterWeight.begin(),_incidentClusterWeight.begin()+idx);
    
    ASSERT([&]() {
        std::set<ClusterID> incidentCluster;
        if(clusterID(node) != -1) incidentCluster.insert(clusterID(node));
        for(Edge e : adjacentNodes(node)) {
            ClusterID cid = clusterID(e.targetNode);
            if(cid != -1) incidentCluster.insert(cid);
        }
        for(auto incCluster : it) {
            ClusterID cid = incCluster.clusterID;
            EdgeWeight weight = incCluster.weight;
            if(incidentCluster.find(cid) == incidentCluster.end()) {
                LOG("ClusterID "<<cid<<" occur multiple or is not incident to node " << node << "!");
                return false;
            }
            EdgeWeight incWeight = 0.0L;
            for(Edge e : adjacentNodes(node)) {
                ClusterID inc_cid = clusterID(e.targetNode);
                if(inc_cid == cid) incWeight += e.weight;
            }
            if(abs(incWeight-weight) > EPS) {
                LOG("Weight calculation of incident cluster " << cid << " failed!");
                LOGVAR(incWeight);
                LOGVAR(weight);
                return false;
            }
            incidentCluster.erase(cid);
        }
        
        if(incidentCluster.size() > 0) {
            LOG("Missing cluster ids in iterator!");
            for(ClusterID cid : incidentCluster) {
                LOGVAR(cid);
            }
            return false;
        }
        
        return true;
    }(), "Incident cluster weight calculation of node " << node << " failed!");
    
    return std::make_pair(_incidentClusterWeight.begin(),_incidentClusterWeight.begin()+idx);
}

std::pair<IncidentClusterWeightIterator,IncidentClusterWeightIterator> Graph::incidentClusterWeightOfCluster(std::pair<NodeIterator,NodeIterator>& cluster_range) {
    
    _posInIncidentClusterWeightVector.clear();
    size_t idx = 0;

    for(NodeID node : cluster_range) {
        for(Edge e : adjacentNodes(node)) {
            NodeID id = e.targetNode;
            EdgeWeight w = e.weight;
            ClusterID c_id = clusterID(id);
            if(_posInIncidentClusterWeightVector.contains(c_id)) {
                size_t i = _posInIncidentClusterWeightVector[c_id];
                _incidentClusterWeight[i].weight += w;
            }
            else {
                _incidentClusterWeight[idx] = IncidentClusterWeight(c_id,w);
                _posInIncidentClusterWeightVector[c_id] = idx++;
            }
        }
    }
    
    auto it = std::make_pair(_incidentClusterWeight.begin(),_incidentClusterWeight.begin()+idx);

    ASSERT([&]() {
        std::set<ClusterID> incidentCluster;
           for(NodeID node : cluster_range) {
             for(Edge e : adjacentNodes(node)) {
                ClusterID cid = clusterID(e.targetNode);
                if(cid != -1) incidentCluster.insert(cid);
             }
           }
           for(auto incCluster : it) {
               ClusterID cid = incCluster.clusterID;
               EdgeWeight weight = incCluster.weight;
               if(incidentCluster.find(cid) == incidentCluster.end()) {
                   LOG("ClusterID "<<cid<<" occur multiple or is not incident to cluster!");
                   return false;
               }
               EdgeWeight incWeight = 0.0L;
               for(NodeID node : cluster_range) {
                 for(Edge e : adjacentNodes(node)) {
                   ClusterID inc_cid = clusterID(e.targetNode);
                   if(inc_cid == cid) incWeight += e.weight;
                 }
               }
               if(abs(incWeight-weight) > EPS) {
                   LOG("Weight calculation of incident cluster " << cid << " failed!");
                   LOGVAR(incWeight);
                   LOGVAR(weight);
                   return false;
               }
               incidentCluster.erase(cid);
           }
           
           if(incidentCluster.size() > 0) {
               LOG("Missing cluster ids in iterator!");
               for(ClusterID cid : incidentCluster) {
                   LOGVAR(cid);
               }
               return false;
           }
           
           return true;
    }(), "Incident cluster weight calculation failed!");
    
    return it;
}



std::pair<Graph,std::vector<NodeID>> Graph::contractCluster() {
    std::vector<NodeID> cluster2Node(numNodes(),INVALID_NODE);
    std::vector<NodeID> node2contractedNode(numNodes(),INVALID_NODE);
    ClusterID new_cid = 0;
    for(NodeID node : nodes()) {
        ClusterID cid = clusterID(node);
        if(cluster2Node[cid] == INVALID_NODE) {
            cluster2Node[cid] = new_cid++;
        }
        node2contractedNode[node] = cluster2Node[cid];
        setClusterID(node,node2contractedNode[node]);
    }
    
    std::vector<NodeID> hypernodeMapping(_hypernodeMapping.size(),INVALID_NODE);
    for(HypernodeID hn = 0; hn < _hypernodeMapping.size(); ++hn) {
        if(_hypernodeMapping[hn] != INVALID_NODE) {
            hypernodeMapping[hn] = node2contractedNode[_hypernodeMapping[hn]];
        }
    }
    
    ASSERT([&]() {
        for(HypernodeID hn = 0; hn < _hypernodeMapping.size(); ++hn) {
            if(_hypernodeMapping[hn] != INVALID_NODE && clusterID(_hypernodeMapping[hn]) != hypernodeMapping[hn]) {
                LOGVAR(clusterID(_hypernodeMapping[hn]));
                LOGVAR(hypernodeMapping[hn]);
                return false;
            }
        } 
        return true;
    }(), "Hypernodes are not correctly mapped to contracted graph");
    
    
    
    std::vector<ClusterID> clusterID(new_cid);
    std::iota(clusterID.begin(),clusterID.end(),0);
    
    
    std::sort(_shuffleNodes.begin(),_shuffleNodes.end());
    std::sort(_shuffleNodes.begin(),_shuffleNodes.end(),[&](const NodeID& n1, const NodeID& n2) {
        return _cluster_id[n1] < _cluster_id[n2]  || (_cluster_id[n1] == _cluster_id[n2] && n1 < n2);
    });
    
    //Add Sentinels
    _shuffleNodes.push_back(_cluster_id.size());
    _cluster_id.push_back(new_cid);
    
    std::vector<NodeID> new_adj_array(new_cid+1,0);
    std::vector<Edge> new_edges;
    size_t start_idx = 0;
    for(size_t i = 0; i < _N+1; ++i) {
        if(_cluster_id[_shuffleNodes[start_idx]] != _cluster_id[_shuffleNodes[i]]) {
            ClusterID cid = _cluster_id[_shuffleNodes[start_idx]];
            new_adj_array[cid] = new_edges.size();
            std::pair<NodeIterator,NodeIterator> cluster_range = std::make_pair(_shuffleNodes.begin()+start_idx,
                                                                                _shuffleNodes.begin()+i);
            for(auto incidentClusterWeight : incidentClusterWeightOfCluster(cluster_range)) {
                Edge e;
                e.targetNode = static_cast<NodeID>(incidentClusterWeight.clusterID);
                e.weight = incidentClusterWeight.weight;
                new_edges.push_back(e);
            }
            start_idx = i;
        }
    }
    
    //Remove Sentinels
    _shuffleNodes.pop_back(); _cluster_id.pop_back();
    
    new_adj_array[new_cid] = new_edges.size();
    Graph graph(new_adj_array,new_edges,hypernodeMapping,clusterID,_config);
    
    return std::make_pair(std::move(graph),node2contractedNode);
}
        
}  // namespace ds
}  // namespace kahypar

