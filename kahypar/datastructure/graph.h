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
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "kahypar/macros.h"

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_map.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/configuration.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
namespace ds {
struct Edge {
  Edge() :
    targetNode(0),
    weight(0.0),
    bfs_cnt(0),
    reverse_edge(nullptr) { }

  NodeID targetNode;
  EdgeWeight weight;
  size_t bfs_cnt;
  Edge* reverse_edge;
};

struct IncidentClusterWeight {
  ClusterID clusterID;
  EdgeWeight weight;

  IncidentClusterWeight(ClusterID clusterID, EdgeWeight weight) :
    clusterID(clusterID),
    weight(weight) { }
};


class Graph {
 private:
  static constexpr NodeID kInvalidNode = std::numeric_limits<NodeID>::max();
  static constexpr long double kEpsilon = 1e-5;

 public:
  using NodeIterator = std::vector<NodeID>::const_iterator;
  using EdgeIterator = std::vector<Edge>::const_iterator;
  using IncidentClusterWeightIterator = std::vector<IncidentClusterWeight>::const_iterator;

  Graph(const Hypergraph& hypergraph, const Configuration& config) :
    _num_nodes(hypergraph.currentNumNodes() +
               (config.preprocessing.louvain_community_detection.use_bipartite_graph ?
                hypergraph.currentNumEdges() : 0)),
    _config(config),
    _adj_array(_num_nodes + 1),
    _nodes(_num_nodes),
    _shuffle_nodes(_num_nodes),
    _edges(),
    _selfloop(_num_nodes, 0.0L),
    _weighted_degree(_num_nodes, 0.0L),
    _cluster_id(_num_nodes),
    _cluster_size(_num_nodes, 1),
    _num_comm(_num_nodes),
    _incident_cluster_weight(_num_nodes, IncidentClusterWeight(0, 0.0L)),
    _total_weight(0.0L),
    _incident_cluster_weight_position(_num_nodes),
    _hypernode_mapping(hypergraph.initialNumNodes() + hypergraph.initialNumEdges(), kInvalidNode) {
    std::iota(_nodes.begin(), _nodes.end(), 0);
    std::iota(_shuffle_nodes.begin(), _shuffle_nodes.end(), 0);
    std::iota(_cluster_id.begin(), _cluster_id.end(), 0);
    if (_config.preprocessing.louvain_community_detection.use_bipartite_graph) {
      switch (_config.preprocessing.louvain_community_detection.edge_weight) {
        case LouvainEdgeWeight::degree:
          constructBipartiteGraph(hypergraph,
                                  [&](const Hypergraph& hg,
                                      const HyperedgeID he,
                                      const HypernodeID hn) {
              return (static_cast<EdgeWeight>(hg.edgeWeight(he)) *
                      static_cast<EdgeWeight>(hg.nodeDegree(hn))) /
              static_cast<EdgeWeight>(hg.edgeSize(he));
            });
          break;
        case LouvainEdgeWeight::non_uniform:
          constructBipartiteGraph(hypergraph,
                                  [&](const Hypergraph& hg,
                                      const HyperedgeID he,
                                      const HypernodeID) {
              return static_cast<EdgeWeight>(hg.edgeWeight(he)) /
              static_cast<EdgeWeight>(hg.edgeSize(he));
            });

          break;
        case LouvainEdgeWeight::uniform:
          constructBipartiteGraph(hypergraph,
                                  [&](const Hypergraph& hg,
                                      const HyperedgeID he,
                                      const HypernodeID) {
              return static_cast<EdgeWeight>(hg.edgeWeight(he));
            });
          break;
        case LouvainEdgeWeight::hybrid:
          LOG("Hybrid edge weight should only be used implicitly");
          break;
      }
    } else {
      switch (_config.preprocessing.louvain_community_detection.edge_weight) {
        case LouvainEdgeWeight::degree:
          constructCliqueGraph(hypergraph,
                               [&](const Hypergraph& hg,
                                   const HyperedgeID he,
                                   const HypernodeID hn) {
              return static_cast<EdgeWeight>(hg.edgeWeight(he) *
                                             hg.nodeDegree(hn)) /
              (static_cast<EdgeWeight>(hg.edgeSize(he) *
                                       (static_cast<EdgeWeight>(hg.edgeSize(he) - 1.0) / 2.0)));
            });
          break;
        case LouvainEdgeWeight::non_uniform:
          constructCliqueGraph(hypergraph,
                               [&](const Hypergraph& hg,
                                   const HyperedgeID he,
                                   const HypernodeID) {
              return static_cast<EdgeWeight>(hg.edgeWeight(he)) /
              (static_cast<EdgeWeight>(hg.edgeSize(he)
                                       * (static_cast<EdgeWeight>(hg.edgeSize(he) - 1.0) / 2.0)));
            });

          break;
        case LouvainEdgeWeight::uniform:
          constructCliqueGraph(hypergraph,
                               [&](const Hypergraph& hg,
                                   const HyperedgeID he,
                                   const HypernodeID) {
              return static_cast<EdgeWeight>(hg.edgeWeight(he));
            });
          break;
        case LouvainEdgeWeight::hybrid:
          LOG("Hybrid edge weight should only be used implicitly");
          break;
      }
    }
  }

  Graph(const std::vector<NodeID>& adj_array, const std::vector<Edge>& edges,
        const Configuration& config) :
    _num_nodes(adj_array.size() - 1),
    _config(config),
    _adj_array(adj_array),
    _nodes(_num_nodes),
    _shuffle_nodes(_num_nodes),
    _edges(edges),
    _selfloop(_num_nodes, 0.0L),
    _weighted_degree(_num_nodes, 0.0L),
    _cluster_id(_num_nodes),
    _cluster_size(_num_nodes, 1),
    _num_comm(_num_nodes),
    _incident_cluster_weight(_num_nodes, IncidentClusterWeight(0, 0.0L)),
    _total_weight(0.0L),
    _incident_cluster_weight_position(_num_nodes),
    _hypernode_mapping(_num_nodes, kInvalidNode) {
    std::iota(_nodes.begin(), _nodes.end(), 0);
    std::iota(_shuffle_nodes.begin(), _shuffle_nodes.end(), 0);
    std::iota(_cluster_id.begin(), _cluster_id.end(), 0);
    std::iota(_hypernode_mapping.begin(), _hypernode_mapping.end(), 0);

    for (NodeID node : nodes()) {
      for (Edge e : incidentEdges(node)) {
        if (node == e.targetNode) {
          _selfloop[node] = e.weight;
        }
        _total_weight += e.weight;
        _weighted_degree[node] += e.weight;
      }
    }
  }

  Graph(const std::vector<NodeID>& adj_array, const std::vector<Edge>& edges,
        const std::vector<NodeID> hypernodeMapping,
        const std::vector<ClusterID> cluster_id, const Configuration& config) :
    _num_nodes(adj_array.size() - 1),
    _config(config),
    _adj_array(adj_array),
    _nodes(_num_nodes),
    _shuffle_nodes(_num_nodes),
    _edges(edges),
    _selfloop(_num_nodes, 0.0L),
    _weighted_degree(_num_nodes, 0.0L),
    _cluster_id(cluster_id),
    _cluster_size(_num_nodes, 0),
    _num_comm(0),
    _incident_cluster_weight(_num_nodes, IncidentClusterWeight(0, 0.0L)),
    _total_weight(0.0L),
    _incident_cluster_weight_position(_num_nodes),
    _hypernode_mapping(hypernodeMapping) {
    std::iota(_nodes.begin(), _nodes.end(), 0);
    std::iota(_shuffle_nodes.begin(), _shuffle_nodes.end(), 0);

    for (NodeID node : nodes()) {
      if (_cluster_size[_cluster_id[node]] == 0) _num_comm++;
      _cluster_size[_cluster_id[node]]++;
      for (Edge e : incidentEdges(node)) {
        if (node == e.targetNode) {
          _selfloop[node] = e.weight;
        }
        _total_weight += e.weight;
        _weighted_degree[node] += e.weight;
      }
    }
  }

  Graph(Graph&& other) :
    _num_nodes(std::move(other._num_nodes)),
    _config(other._config),
    _adj_array(std::move(other._adj_array)),
    _nodes(std::move(other._nodes)),
    _shuffle_nodes(std::move(other._shuffle_nodes)),
    _edges(std::move(other._edges)),
    _selfloop(std::move(other._selfloop)),
    _weighted_degree(std::move(other._weighted_degree)),
    _cluster_id(std::move(other._cluster_id)),
    _cluster_size(std::move(other._cluster_size)),
    _num_comm(std::move(other._num_comm)),
    _incident_cluster_weight(_num_nodes, IncidentClusterWeight(0, 0.0L)),
    _total_weight(std::move(other._total_weight)),
    _incident_cluster_weight_position(std::move(other._incident_cluster_weight_position)),
    _hypernode_mapping(std::move(other._hypernode_mapping)) { }

  Graph(const Graph& other) :
    _num_nodes(other._num_nodes),
    _config(other._config),
    _adj_array(other._adj_array),
    _nodes(other._nodes),
    _shuffle_nodes(other._shuffle_nodes),
    _edges(other._edges),
    _selfloop(other._selfloop),
    _weighted_degree(other._weighted_degree),
    _cluster_id(other._cluster_id),
    _cluster_size(other._cluster_size),
    _num_comm(other._num_comm),
    _incident_cluster_weight(_num_nodes, IncidentClusterWeight(0, 0.0L)),
    _total_weight(other._total_weight),
    _incident_cluster_weight_position(_num_nodes),
    _hypernode_mapping(other._hypernode_mapping) { }

  Graph& operator= (Graph& other) {
    _num_nodes = other._num_nodes;
    _adj_array = other._adj_array;
    _nodes = other._nodes;
    _shuffle_nodes = other._shuffle_nodes;
    _edges = other._edges;
    _selfloop = other._selfloop;
    _weighted_degree = other._weighted_degree;
    _cluster_id = other._cluster_id;
    _cluster_size = other._cluster_size;
    _num_comm = other._num_comm;
    _incident_cluster_weight.assign(_num_nodes, IncidentClusterWeight(0, 0.0L));
    _total_weight = other._total_weight;
    _incident_cluster_weight_position = other._incident_cluster_weight_position;
    _hypernode_mapping = other._hypernode_mapping;
    return *this;
  }

  Graph& operator= (Graph&& other) {
    _num_nodes = std::move(other._num_nodes);
    _adj_array = std::move(other._adj_array);
    _nodes = std::move(other._nodes);
    _shuffle_nodes = std::move(other._shuffle_nodes);
    _edges = std::move(other._edges);
    _selfloop = std::move(other._selfloop);
    _weighted_degree = std::move(other._weighted_degree);
    _cluster_id = std::move(other._cluster_id);
    _cluster_size = std::move(other._cluster_size);
    _num_comm = std::move(other._num_comm);
    _incident_cluster_weight.assign(_num_nodes, IncidentClusterWeight(0, 0.0L));
    _total_weight = std::move(other._total_weight);
    _incident_cluster_weight_position = other._incident_cluster_weight_position;
    _hypernode_mapping = std::move(other._hypernode_mapping);
    _incident_cluster_weight_position.clear();
    return *this;
  }


  std::pair<NodeIterator, NodeIterator> nodes() const {
    return std::make_pair(_nodes.begin(), _nodes.end());
  }

  std::pair<NodeIterator, NodeIterator> randomNodeOrder() {
    Randomize::instance().shuffleVector(_shuffle_nodes, _shuffle_nodes.size());
    return std::make_pair(_shuffle_nodes.begin(), _shuffle_nodes.end());
  }

  void shuffleNodes() {
    Randomize::instance().shuffleVector(_shuffle_nodes, _shuffle_nodes.size());
  }


  std::pair<EdgeIterator, EdgeIterator> incidentEdges(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
    return std::make_pair(_edges.cbegin() + _adj_array[node], _edges.cbegin() + _adj_array[node + 1]);
  }

  size_t numNodes() const {
    return static_cast<size_t>(_num_nodes);
  }

  size_t numEdges() const {
    return _edges.size();
  }

  size_t degree(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
    return static_cast<size_t>(_adj_array[node + 1] - _adj_array[node]);
  }

  EdgeWeight weightedDegree(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
    return _weighted_degree[node];
  }

  EdgeWeight selfloopWeight(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID " << node << " doesn't exist!");
    return _selfloop[node];
  }


  EdgeWeight totalWeight() const {
    return _total_weight;
  }

  void setHypernodeClusterID(const HypernodeID hn, const ClusterID c_id) {
    ASSERT(_hypernode_mapping[hn] != kInvalidNode);
    _cluster_id[_hypernode_mapping[hn]] = c_id;
  }

  void setHyperedgeClusterID(const HyperedgeID he, const ClusterID c_id, const size_t N) {
    ASSERT(_hypernode_mapping[N + he] != kInvalidNode);
    _cluster_id[_hypernode_mapping[N + he]] = c_id;
  }

  ClusterID hypernodeClusterID(const HypernodeID hn) const {
    ASSERT(_hypernode_mapping[hn] != kInvalidNode);
    return _cluster_id[_hypernode_mapping[hn]];
  }


  ClusterID hyperedgeClusterID(const HyperedgeID he, const size_t N) const {
    ASSERT(_hypernode_mapping[N + he] != kInvalidNode);
    return _cluster_id[_hypernode_mapping[N + he]];
  }

  size_t numCommunities() const {
    return _num_comm;
  }

  size_t clusterSize(const ClusterID cid) const {
    return _cluster_size[cid];
  }

  ClusterID clusterID(const NodeID node) const {
    ASSERT(node < numNodes());
    return _cluster_id[node];
  }

  void setClusterID(const NodeID node, const ClusterID c_id) {
    ASSERT(node < numNodes());

    const ClusterID from = _cluster_id[node];
    const ClusterID to = c_id;

    if (from != -1 && from != to && _cluster_size[from] == 1) {
      _num_comm--;
    }
    if (to != -1 && from != to && _cluster_size[to] == 0) {
      _num_comm++;
    }

    if (to != -1) {
      _cluster_size[to]++;
    }
    if (from != -1) {
      _cluster_size[from]--;
    }

    _cluster_id[node] = to;

    ASSERT([&]() {
          std::set<ClusterID> distinct_comm;
          size_t from_size = 0;
          size_t to_size = 0;

          for (NodeID node : nodes()) {
            if (clusterID(node) != -1) distinct_comm.insert(clusterID(node));
            if (from != -1 && clusterID(node) == from) from_size++;
            if (to != -1 && clusterID(node) == to) to_size++;
          }
          if (distinct_comm.size() != _num_comm) {
            LOGVAR(_num_comm);
            LOGVAR(distinct_comm.size());
            return false;
          } else if (to != -1 && to_size != _cluster_size[to]) {
            LOGVAR(to_size);
            LOGVAR(_cluster_size[to]);
            return false;
          } else if (from != -1 && from_size != _cluster_size[from]) {
            LOGVAR(from_size);
            LOGVAR(_cluster_size[from]);
            return false;
          }
          return true;
        } ());
  }


  /**
   * Creates an iterator to all incident Clusters of Node node. Iterator points to an
   * IncidentClusterWeight-Struct which contains the incident Cluster ID and the sum of
   * the weight of all incident edges to that cluster.
   *
   * @param node NodeID, which incident clusters should be evaluated
   * @return Iterator to all incident clusters of NodeID node
   */
  std::pair<IncidentClusterWeightIterator,
            IncidentClusterWeightIterator> incidentClusterWeightOfNode(const NodeID node);


  /**
   * Contracts the Graph based on the nodes ClusterIDs. Nodes with same ClusterID are contracted
   * in a single node in the contracted graph. Edges are inserted based on the sum of the weight
   * of all edges which connects two clusters. Also a mapping is created which maps the nodes of
   * the graph to the corresponding contracted nodes.
   *
   * @return Pair which contains the contracted graph and a mapping from current to nodes to its
   * corresponding contrated nodes.
   */
  std::pair<Graph, std::vector<NodeID> > contractCluster();

  void printGraph() {
    std::cout << "Number Nodes: " << numNodes() << std::endl;
    std::cout << "Number Edges: " << numEdges() << std::endl;

    for (NodeID n : nodes()) {
      std::cout << "Node ID: " << n << "(Comm.: " << clusterID(n) << "), Adj. List: ";
      for (Edge e : incidentEdges(n)) {
        std::cout << "(" << e.targetNode << ",w=" << e.weight << ") ";
      }
      std::cout << "\n";
    }
  }

 protected:
  NodeID _num_nodes;
  const Configuration& _config;
  std::vector<NodeID> _adj_array;
  std::vector<NodeID> _nodes;
  std::vector<NodeID> _shuffle_nodes;
  std::vector<Edge> _edges;
  std::vector<EdgeWeight> _selfloop;
  std::vector<EdgeWeight> _weighted_degree;
  std::vector<ClusterID> _cluster_id;
  std::vector<size_t> _cluster_size;
  size_t _num_comm;
  std::vector<IncidentClusterWeight> _incident_cluster_weight;
  EdgeWeight _total_weight;
  SparseMap<ClusterID, size_t> _incident_cluster_weight_position;
  std::vector<NodeID> _hypernode_mapping;

 private:
  FRIEND_TEST(ABipartiteGraph, DeterminesIncidentClusterWeightsOfAClusterCorrect);
  FRIEND_TEST(ACliqueGraph, DeterminesIncidentClusterWeightsOfAClusterCorrect);


  /**
   * Creates an iterator to all incident Clusters of ClusterID cid. Iterator points to an
   * IncidentClusterWeight-Struct which contains the incident ClusterID clusterID and the sum of
   * the weights of all incident edges from cid to clusterID.
   *
   * @param cid ClusterID, which incident clusters should be evaluated
   * @return Iterator to all incident clusters of ClusterID cid
   */
  std::pair<IncidentClusterWeightIterator, IncidentClusterWeightIterator> incidentClusterWeightOfCluster(const std::pair<NodeIterator, NodeIterator>& cluster_range);

  template <typename EdgeWeightFunction>
  void constructBipartiteGraph(const Hypergraph& hg, const EdgeWeightFunction& edgeWeight) {
    NodeID sum_edges = 0;

    const size_t num_nodes = static_cast<size_t>(hg.initialNumNodes());

    NodeID cur_node_id = 0;

    // Construct adj. array for all hypernodes.
    // Number of edges is equal to the degree of the corresponding hypernode.
    for (HypernodeID hn : hg.nodes()) {
      _hypernode_mapping[hn] = cur_node_id;
      _adj_array[cur_node_id++] = sum_edges;
      sum_edges += hg.nodeDegree(hn);
    }

    // Construct adj. array for all hyperedges.
    // Number of edges is equal to the size of the corresponding hyperedge.
    for (HyperedgeID he : hg.edges()) {
      _hypernode_mapping[num_nodes + he] = cur_node_id;
      _adj_array[cur_node_id++] = sum_edges;
      sum_edges += hg.edgeSize(he);
    }

    _adj_array[_num_nodes] = sum_edges;
    _edges.resize(sum_edges);

    for (HypernodeID hn : hg.nodes()) {
      size_t pos = 0;
      const NodeID graph_node = _hypernode_mapping[hn];
      for (HyperedgeID he : hg.incidentEdges(hn)) {
        Edge e;
        e.targetNode = _hypernode_mapping[num_nodes + he];
        e.weight = edgeWeight(hg, he, hn);
        _total_weight += e.weight;
        _weighted_degree[graph_node] += e.weight;
        _edges[_adj_array[graph_node] + pos++] = e;
      }
    }

    for (HyperedgeID he : hg.edges()) {
      size_t pos = 0;
      for (HypernodeID hn : hg.pins(he)) {
        Edge e;
        e.targetNode = _hypernode_mapping[hn];
        e.weight = edgeWeight(hg, he, hn);
        const NodeID cur_node = _hypernode_mapping[num_nodes + he];
        _total_weight += e.weight;
        _weighted_degree[cur_node] += e.weight;
        _edges[_adj_array[cur_node] + pos++] = e;
        for (size_t i = _adj_array[e.targetNode]; i < _adj_array[e.targetNode + 1]; ++i) {
          if (_edges[i].targetNode == cur_node) {
            _edges[i].reverse_edge = &_edges[_adj_array[cur_node] + pos - 1];
            _edges[_adj_array[cur_node] + pos - 1].reverse_edge = &_edges[i];
            break;
          }
        }
      }
    }


    ASSERT([&]() {
          //Check Hypernodes in Graph
          for (HypernodeID hn : hg.nodes()) {
            if (hg.nodeDegree(hn) != degree(_hypernode_mapping[hn])) {
              LOGVAR(hg.nodeDegree(hn));
              LOGVAR(degree(_hypernode_mapping[hn]));
              return false;
            }
            std::set<HyperedgeID> incident_edges;
            for (HyperedgeID he : hg.incidentEdges(hn)) {
              incident_edges.insert(_hypernode_mapping[num_nodes + he]);
            }
            for (Edge e : incidentEdges(_hypernode_mapping[hn])) {
              HyperedgeID he = e.targetNode;
              if (incident_edges.find(he) == incident_edges.end()) {
                LOGVAR(_hypernode_mapping[hn]);
                LOGVAR(he);
                return false;
              }
            }
          }

          //Checks Hyperedges in Graph
          for (HyperedgeID he : hg.edges()) {
            if (hg.edgeSize(he) != degree(_hypernode_mapping[he + num_nodes])) {
              LOGVAR(hg.edgeSize(he));
              LOGVAR(degree(_hypernode_mapping[he + num_nodes]));
              return false;
            }
            std::set<HypernodeID> pins;
            for (HypernodeID hn : hg.pins(he)) {
              pins.insert(_hypernode_mapping[hn]);
            }
            for (Edge e : incidentEdges(_hypernode_mapping[he + num_nodes])) {
              if (pins.find(e.targetNode) == pins.end()) {
                LOGVAR(_hypernode_mapping[he + num_nodes]);
                LOGVAR(e.targetNode);
                return false;
              }
            }
          }
          return true;
        } (), "Bipartite Graph is not equivalent with hypergraph");
  }

  template <typename EdgeWeightFunction>
  void constructCliqueGraph(const Hypergraph& hg, const EdgeWeightFunction& edgeWeight) {
    NodeID sum_edges = 0;
    NodeID cur_node_id = 0;

    for (HypernodeID hn : hg.nodes()) {
      _hypernode_mapping[hn] = cur_node_id++;
    }

    for (HypernodeID hn : hg.nodes()) {
      const NodeID cur_node = _hypernode_mapping[hn];
      _adj_array[cur_node] = sum_edges;
      std::vector<Edge> tmp_edges;

      for (HyperedgeID he : hg.incidentEdges(hn)) {
        for (HypernodeID pin : hg.pins(he)) {
          if (hn == pin) continue;
          Edge e;
          NodeID v = _hypernode_mapping[pin];
          e.targetNode = v;
          e.weight = edgeWeight(hg, he, hn);
          tmp_edges.push_back(e);
          _total_weight += e.weight;
          _weighted_degree[cur_node] += e.weight;
        }
      }

      std::sort(tmp_edges.begin(), tmp_edges.end(), [&](const Edge& e1, const Edge& e2) {
            return e1.targetNode < e2.targetNode;
          });
      Edge sentinel;
      sentinel.targetNode = kInvalidNode;
      tmp_edges.push_back(sentinel);
      if (tmp_edges.size() > 1) {
        Edge cur_edge;
        cur_edge.targetNode = tmp_edges[0].targetNode;
        cur_edge.weight = tmp_edges[0].weight;
        for (size_t i = 1; i < tmp_edges.size(); ++i) {
          if (cur_edge.targetNode == tmp_edges[i].targetNode) {
            cur_edge.weight += tmp_edges[i].weight;
          } else {
            _edges.push_back(cur_edge);
            cur_edge.targetNode = tmp_edges[i].targetNode;
            cur_edge.weight = tmp_edges[i].weight;
          }
        }
        sum_edges = _edges.size();
      }
    }

    _adj_array[_num_nodes] = sum_edges;

    ASSERT([&]() {
          for (HypernodeID hn : hg.nodes()) {
            std::set<HypernodeID> incident_nodes;
            for (HyperedgeID he : hg.incidentEdges(hn)) {
              for (HypernodeID pin : hg.pins(he)) {
                incident_nodes.insert(pin);
              }
            }
            incident_nodes.erase(hn);
            if (incident_nodes.size() != degree(hn)) {
              LOGVAR(incident_nodes.size());
              LOGVAR(degree(hn));
              return false;
            }
            for (Edge e : incidentEdges(hn)) {
              if (incident_nodes.find(e.targetNode) == incident_nodes.end()) {
                LOGVAR(hn);
                LOGVAR(e.targetNode);
                return false;
              }
            }
          }
          return true;
        } (), "Clique Graph is not equivalent with Hypergraph");
  }
};

std::pair<Graph::IncidentClusterWeightIterator,
          Graph::IncidentClusterWeightIterator> Graph::incidentClusterWeightOfNode(const NodeID node) {
  _incident_cluster_weight_position.clear();
  size_t idx = 0;

  if (clusterID(node) != -1) {
    _incident_cluster_weight[idx] = IncidentClusterWeight(clusterID(node), 0.0L);
    _incident_cluster_weight_position[clusterID(node)] = idx++;
  }

  for (Edge e : incidentEdges(node)) {
    const NodeID id = e.targetNode;
    const EdgeWeight w = e.weight;
    const ClusterID c_id = clusterID(id);
    if (c_id != -1) {
      if (_incident_cluster_weight_position.contains(c_id)) {
        size_t i = _incident_cluster_weight_position[c_id];
        _incident_cluster_weight[i].weight += w;
      } else {
        _incident_cluster_weight[idx] = IncidentClusterWeight(c_id, w);
        _incident_cluster_weight_position[c_id] = idx++;
      }
    }
  }

  ASSERT([&]() {
        const auto it = std::make_pair(_incident_cluster_weight.begin(),
                                       _incident_cluster_weight.begin() + idx);
        std::set<ClusterID> incidentCluster;
        if (clusterID(node) != -1) incidentCluster.insert(clusterID(node));
        for (Edge e : incidentEdges(node)) {
          ClusterID cid = clusterID(e.targetNode);
          if (cid != -1) incidentCluster.insert(cid);
        }
        for (auto incCluster : it) {
          ClusterID cid = incCluster.clusterID;
          EdgeWeight weight = incCluster.weight;
          if (incidentCluster.find(cid) == incidentCluster.end()) {
            LOG("ClusterID " << cid << " occur multiple or is not incident to node " << node << "!");
            return false;
          }
          EdgeWeight incWeight = 0.0L;
          for (Edge e : incidentEdges(node)) {
            ClusterID inc_cid = clusterID(e.targetNode);
            if (inc_cid == cid) incWeight += e.weight;
          }
          if (abs(incWeight - weight) > kEpsilon) {
            LOG("Weight calculation of incident cluster " << cid << " failed!");
            LOGVAR(incWeight);
            LOGVAR(weight);
            return false;
          }
          incidentCluster.erase(cid);
        }

        if (incidentCluster.size() > 0) {
          LOG("Missing cluster ids in iterator!");
          for (ClusterID cid : incidentCluster) {
            LOGVAR(cid);
          }
          return false;
        }

        return true;
      } (), "Incident cluster weight calculation of node " << node << " failed!");

  return std::make_pair(_incident_cluster_weight.begin(), _incident_cluster_weight.begin() + idx);
}

std::pair<Graph::IncidentClusterWeightIterator,
          Graph::IncidentClusterWeightIterator> Graph::incidentClusterWeightOfCluster(const std::pair<NodeIterator, NodeIterator>& cluster_range) {
  _incident_cluster_weight_position.clear();
  size_t idx = 0;

  for (NodeID node : cluster_range) {
    for (Edge e : incidentEdges(node)) {
      const NodeID id = e.targetNode;
      const EdgeWeight w = e.weight;
      const ClusterID c_id = clusterID(id);
      if (_incident_cluster_weight_position.contains(c_id)) {
        const size_t i = _incident_cluster_weight_position[c_id];
        _incident_cluster_weight[i].weight += w;
      } else {
        _incident_cluster_weight[idx] = IncidentClusterWeight(c_id, w);
        _incident_cluster_weight_position[c_id] = idx++;
      }
    }
  }

  auto it = std::make_pair(_incident_cluster_weight.begin(), _incident_cluster_weight.begin() + idx);

  ASSERT([&]() {
        std::set<ClusterID> incidentCluster;
        for (NodeID node : cluster_range) {
          for (Edge e : incidentEdges(node)) {
            ClusterID cid = clusterID(e.targetNode);
            if (cid != -1) incidentCluster.insert(cid);
          }
        }
        for (auto incCluster : it) {
          ClusterID cid = incCluster.clusterID;
          EdgeWeight weight = incCluster.weight;
          if (incidentCluster.find(cid) == incidentCluster.end()) {
            LOG("ClusterID " << cid << " occur multiple or is not incident to cluster!");
            return false;
          }
          EdgeWeight incWeight = 0.0L;
          for (NodeID node : cluster_range) {
            for (Edge e : incidentEdges(node)) {
              ClusterID inc_cid = clusterID(e.targetNode);
              if (inc_cid == cid) incWeight += e.weight;
            }
          }
          if (abs(incWeight - weight) > kEpsilon) {
            LOG("Weight calculation of incident cluster " << cid << " failed!");
            LOGVAR(incWeight);
            LOGVAR(weight);
            return false;
          }
          incidentCluster.erase(cid);
        }

        if (incidentCluster.size() > 0) {
          LOG("Missing cluster ids in iterator!");
          for (ClusterID cid : incidentCluster) {
            LOGVAR(cid);
          }
          return false;
        }

        return true;
      } (), "Incident cluster weight calculation failed!");

  return it;
}


std::pair<Graph, std::vector<NodeID> > Graph::contractCluster() {
  std::vector<NodeID> cluster2Node(numNodes(), kInvalidNode);
  std::vector<NodeID> node2contractedNode(numNodes(), kInvalidNode);
  ClusterID new_cid = 0;
  for (NodeID node : nodes()) {
    const ClusterID cid = clusterID(node);
    if (cluster2Node[cid] == kInvalidNode) {
      cluster2Node[cid] = new_cid++;
    }
    node2contractedNode[node] = cluster2Node[cid];
    setClusterID(node, node2contractedNode[node]);
  }

  std::vector<NodeID> hypernodeMapping(_hypernode_mapping.size(), kInvalidNode);
  for (HypernodeID hn = 0; hn < _hypernode_mapping.size(); ++hn) {
    if (_hypernode_mapping[hn] != kInvalidNode) {
      hypernodeMapping[hn] = node2contractedNode[_hypernode_mapping[hn]];
    }
  }

  ASSERT([&]() {
        for (HypernodeID hn = 0; hn < _hypernode_mapping.size(); ++hn) {
          if (_hypernode_mapping[hn] != kInvalidNode && clusterID(_hypernode_mapping[hn]) != hypernodeMapping[hn]) {
            LOGVAR(clusterID(_hypernode_mapping[hn]));
            LOGVAR(hypernodeMapping[hn]);
            return false;
          }
        }
        return true;
      } (), "Hypernodes are not correctly mapped to contracted graph");


  std::vector<ClusterID> clusterID(new_cid);
  std::iota(clusterID.begin(), clusterID.end(), 0);


  std::sort(_shuffle_nodes.begin(), _shuffle_nodes.end());
  std::sort(_shuffle_nodes.begin(), _shuffle_nodes.end(), [&](const NodeID& n1, const NodeID& n2) {
        return _cluster_id[n1] < _cluster_id[n2] || (_cluster_id[n1] == _cluster_id[n2] && n1 < n2);
      });

  //Add Sentinels
  _shuffle_nodes.push_back(_cluster_id.size());
  _cluster_id.push_back(new_cid);

  std::vector<NodeID> new_adj_array(new_cid + 1, 0);
  std::vector<Edge> new_edges;
  size_t start_idx = 0;
  for (size_t i = 0; i < _num_nodes + 1; ++i) {
    if (_cluster_id[_shuffle_nodes[start_idx]] != _cluster_id[_shuffle_nodes[i]]) {
      const ClusterID cid = _cluster_id[_shuffle_nodes[start_idx]];
      new_adj_array[cid] = new_edges.size();
      std::pair<NodeIterator, NodeIterator> cluster_range = std::make_pair(_shuffle_nodes.begin() + start_idx,
                                                                           _shuffle_nodes.begin() + i);
      for (auto incident_cluster_weight : incidentClusterWeightOfCluster(cluster_range)) {
        Edge e;
        e.targetNode = static_cast<NodeID>(incident_cluster_weight.clusterID);
        e.weight = incident_cluster_weight.weight;
        new_edges.push_back(e);
      }
      start_idx = i;
    }
  }

  //Remove Sentinels
  _shuffle_nodes.pop_back();
  _cluster_id.pop_back();

  new_adj_array[new_cid] = new_edges.size();
  Graph graph(new_adj_array, new_edges, hypernodeMapping, clusterID, _config);

  return std::make_pair(std::move(graph), node2contractedNode);
}

constexpr NodeID Graph::kInvalidNode;
}  // namespace ds
}  // namespace kahypar
