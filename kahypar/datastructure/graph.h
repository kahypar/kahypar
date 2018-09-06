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
#include <numeric>
#include <set>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "kahypar/macros.h"

#include "kahypar/datastructure/sparse_map.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
namespace ds {
struct Edge {
  NodeID target_node = 0;
  EdgeWeight weight = 0.0;
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
  class NodeIDIterator : public std::iterator<
                           std::forward_iterator_tag,  // iterator_category
                           NodeID,  // value_type
                           std::ptrdiff_t,  // difference_type
                           NodeID*,  // pointer
                           NodeID>{  // reference
 public:
    explicit NodeIDIterator(const NodeID start) :
      _i(start) { }

    reference operator* () const {
      return _i;
    }
    NodeIDIterator& operator++ () {
      ++_i;
      return *this;
    }
    NodeIDIterator operator++ (int) {
      NodeIDIterator copy(*this);
      ++_i;
      return copy;
    }

    bool operator== (const NodeIDIterator& other) const { return _i == other._i; }
    bool operator!= (const NodeIDIterator& other) const { return _i != other._i; }

 private:
    NodeID _i;
  };

 public:
  static constexpr NodeID kInvalidNode = std::numeric_limits<NodeID>::max();
  static constexpr long double kEpsilon = 1e-5;
  using NodeIterator = std::vector<NodeID>::const_iterator;
  using EdgeIterator = std::vector<Edge>::const_iterator;
  using IncidentClusterWeightIterator = std::vector<IncidentClusterWeight>::const_iterator;

  Graph(const Hypergraph& hypergraph, const Context& context) :
    _num_nodes(0),
    _num_communities(0),
    _total_weight(0.0L),
    _is_graph(true),
    _adj_array(),
    _edges(),
    _selfloop_weight(),
    _weighted_degree(),
    _cluster_id(),
    _cluster_size(),
    _incident_cluster_weight(),
    _incident_cluster_weight_position(hypergraph.initialNumNodes() + hypergraph.initialNumEdges()),
    _hypernode_mapping() {
    for (const HyperedgeID he : hypergraph.edges()) {
      if (hypergraph.edgeSize(he) > 2) {
        _is_graph = false;
        break;
      }
    }

    const bool verbose_output = (context.type == ContextType::main &&
                                 context.partition.verbose_output) ||
                                (context.type == ContextType::initial_partitioning &&
                                 context.initial_partitioning.verbose_output);
    if (verbose_output) {
      LOG << "  hypergraph is a graph =" << std::boolalpha << _is_graph;
    }

    if (_is_graph) {
      _num_nodes = hypergraph.currentNumNodes();
      _hypernode_mapping.resize(hypergraph.initialNumNodes(), kInvalidNode);
    } else {
      _num_nodes = hypergraph.currentNumNodes() + hypergraph.currentNumEdges();
      _hypernode_mapping.resize(hypergraph.initialNumNodes() + hypergraph.initialNumEdges(),
                                kInvalidNode);
    }

    _num_communities = _num_nodes;
    _adj_array.resize(_num_nodes + 1);
    _selfloop_weight.resize(_num_nodes, 0.0L);
    _weighted_degree.resize(_num_nodes, 0.0L);
    _cluster_id.resize(_num_nodes);
    _cluster_size.resize(_num_nodes, 1);
    _incident_cluster_weight.resize(_num_nodes, IncidentClusterWeight(0, 0.0L));
    std::iota(_cluster_id.begin(), _cluster_id.end(), 0);

    if (_is_graph) {
      constructGraph(hypergraph, [&](const Hypergraph& hg,
                                     const HyperedgeID he,
                                     const HypernodeID) {
            return static_cast<EdgeWeight>(hg.edgeWeight(he));
          });
    } else {
      switch (context.preprocessing.community_detection.edge_weight) {
        case LouvainEdgeWeight::degree:
          constructBipartiteGraph(hypergraph, [&](const Hypergraph& hg,
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
          constructBipartiteGraph(hypergraph, [&](const Hypergraph& hg,
                                                  const HyperedgeID he,
                                                  const HypernodeID) {
              return static_cast<EdgeWeight>(hg.edgeWeight(he));
            });
          break;
        case LouvainEdgeWeight::hybrid:
          LOG << "Only uniform/non-uniform/degree edge weight is allowed at graph construction.";
          std::exit(-1);
        default:
          LOG << "Unknown edge weight for bipartite graph.";
          std::exit(-1);
      }
    }
  }

  Graph(const std::vector<NodeID>& adj_array, const std::vector<Edge>& edges) :
    _num_nodes(adj_array.size() - 1),
    _num_communities(_num_nodes),
    _total_weight(0.0L),
    _is_graph(true),
    _adj_array(adj_array),
    _edges(edges),
    _selfloop_weight(_num_nodes, 0.0L),
    _weighted_degree(_num_nodes, 0.0L),
    _cluster_id(_num_nodes),
    _cluster_size(_num_nodes, 1),
    _incident_cluster_weight(_num_nodes, IncidentClusterWeight(0, 0.0L)),
    _incident_cluster_weight_position(_num_nodes),
    _hypernode_mapping(_num_nodes, kInvalidNode) {
    std::iota(_cluster_id.begin(), _cluster_id.end(), 0);
    std::iota(_hypernode_mapping.begin(), _hypernode_mapping.end(), 0);

    for (const NodeID& node : nodes()) {
      for (const Edge& e : incidentEdges(node)) {
        if (node == e.target_node) {
          _selfloop_weight[node] = e.weight;
        }
        _total_weight += e.weight;
        _weighted_degree[node] += e.weight;
      }
    }
  }

  Graph(const Graph& other) = delete;
  Graph& operator= (const Graph& other) = delete;

  Graph(Graph&& other) = default;
  Graph& operator= (Graph&& other) = delete;

  ~Graph() = default;

  std::pair<NodeIDIterator, NodeIDIterator> nodes() const {
    return std::make_pair(NodeIDIterator(0), NodeIDIterator(_num_nodes));
  }

  std::pair<EdgeIterator, EdgeIterator> incidentEdges(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID" << node << "doesn't exist!");
    return std::make_pair(_edges.cbegin() + _adj_array[node],
                          _edges.cbegin() + _adj_array[static_cast<size_t>(node) + 1]);
  }

  EdgeIterator firstEdge(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID" << node << "doesn't exist!");
    return _edges.cbegin() + _adj_array[node];
  }

  EdgeIterator firstInvalidEdge(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID" << node << "doesn't exist!");
    return _edges.cbegin() + _adj_array[static_cast<size_t>(node) + 1];
  }


  size_t numNodes() const {
    return static_cast<size_t>(_num_nodes);
  }

  size_t numEdges() const {
    return _edges.size();
  }

  size_t degree(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID" << node << "doesn't exist!");
    return static_cast<size_t>(_adj_array[static_cast<size_t>(node) + 1] - _adj_array[node]);
  }

  EdgeWeight weightedDegree(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID" << node << "doesn't exist!");
    return _weighted_degree[node];
  }

  EdgeWeight selfloopWeight(const NodeID node) const {
    ASSERT(node < numNodes(), "NodeID" << node << "doesn't exist!");
    return _selfloop_weight[node];
  }


  EdgeWeight totalWeight() const {
    return _total_weight;
  }

  ClusterID hypernodeClusterID(const HypernodeID hn) const {
    ASSERT(_hypernode_mapping[hn] != kInvalidNode);
    return _cluster_id[_hypernode_mapping[hn]];
  }

  size_t numCommunities() const {
    return _num_communities;
  }

  ClusterID clusterID(const NodeID node) const {
    ASSERT(node < numNodes(), V(node));
    return _cluster_id[node];
  }

  void setClusterID(const NodeID node, const ClusterID c_id) {
    ASSERT(node < numNodes());

    const ClusterID from = _cluster_id[node];
    const ClusterID to = c_id;

    if (from != -1 && from != to && _cluster_size[from] == 1) {
      _num_communities--;
    }
    if (to != -1 && from != to && _cluster_size[to] == 0) {
      _num_communities++;
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

          for (const NodeID& node : nodes()) {
            if (clusterID(node) != -1) {
              distinct_comm.insert(clusterID(node));
            }
            if (from != -1 && clusterID(node) == from) {
              from_size++;
            }
            if (to != -1 && clusterID(node) == to) {
              to_size++;
            }
          }
          if (distinct_comm.size() != _num_communities) {
            LOG << V(_num_communities);
            LOG << V(distinct_comm.size());
            return false;
          } else if (to != -1 && to_size != _cluster_size[to]) {
            LOG << V(to_size);
            LOG << V(_cluster_size[to]);
            return false;
          } else if (from != -1 && from_size != _cluster_size[from]) {
            LOG << V(from_size);
            LOG << V(_cluster_size[from]);
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
            IncidentClusterWeightIterator> incidentClusterWeightOfNode(const NodeID node) {
    _incident_cluster_weight_position.clear();
    size_t idx = 0;

    if (clusterID(node) != -1) {
      _incident_cluster_weight[idx] = IncidentClusterWeight(clusterID(node), 0.0L);
      _incident_cluster_weight_position[clusterID(node)] = idx++;
    }

    for (const Edge& e : incidentEdges(node)) {
      const NodeID id = e.target_node;
      const EdgeWeight w = e.weight;
      const ClusterID c_id = clusterID(id);
      if (c_id != -1) {
        if (_incident_cluster_weight_position.contains(c_id)) {
          _incident_cluster_weight[_incident_cluster_weight_position[c_id]].weight += w;
        } else {
          _incident_cluster_weight[idx] = IncidentClusterWeight(c_id, w);
          _incident_cluster_weight_position[c_id] = idx++;
        }
      }
    }

    ASSERT([&]() {
          const auto incident_cluster_weight_range =
            std::make_pair(_incident_cluster_weight.begin(),
                           _incident_cluster_weight.begin() + idx);
          std::set<ClusterID> incident_cluster;
          if (clusterID(node) != -1) {
            incident_cluster.insert(clusterID(node));
          }
          for (const Edge& e : incidentEdges(node)) {
            const ClusterID cid = clusterID(e.target_node);
            if (cid != -1) {
              incident_cluster.insert(cid);
            }
          }
          for (const auto& cluster : incident_cluster_weight_range) {
            const ClusterID cid = cluster.clusterID;
            const EdgeWeight weight = cluster.weight;
            if (incident_cluster.find(cid) == incident_cluster.end()) {
              LOG << "ClusterID" << cid << "occurs multiple times or is not incident to node"
                  << node;
              return false;
            }
            EdgeWeight incident_weight = 0.0L;
            for (const Edge& e : incidentEdges(node)) {
              const ClusterID inc_cid = clusterID(e.target_node);
              if (inc_cid == cid) {
                incident_weight += e.weight;
              }
            }
            if (std::abs(incident_weight - weight) > kEpsilon) {
              LOG << "Weight calculation of incident cluster" << cid << "failed!";
              LOG << V(incident_weight);
              LOG << V(weight);
              return false;
            }
            incident_cluster.erase(cid);
          }

          if (incident_cluster.size() > 0) {
            LOG << "Missing cluster ids in iterator!";
            for (const ClusterID& cid : incident_cluster) {
              LOG << V(cid);
            }
            return false;
          }
          return true;
        } (), "Incident cluster weight calculation of node" << node << "failed!");

    return std::make_pair(_incident_cluster_weight.begin(), _incident_cluster_weight.begin() + idx);
  }


  /**
   * Contracts the Graph based on the nodes ClusterIDs. Nodes with same ClusterID are contracted
   * in a single node in the contracted graph. Edges are inserted based on the sum of the weight
   * of all edges which connects two clusters. Also a mapping is created which maps the nodes of
   * the graph to the corresponding contracted nodes.
   *
   * @return Pair which contains the contracted graph and a mapping from current to nodes to its
   * corresponding contrated nodes.
   */
  std::pair<Graph, std::vector<NodeID> > contractClusters() {
    std::vector<NodeID> cluster_to_node(numNodes(), kInvalidNode);
    std::vector<NodeID> node_to_contracted_node(numNodes(), kInvalidNode);
    ClusterID new_cid = 0;
    for (const NodeID& node : nodes()) {
      const ClusterID cid = clusterID(node);
      if (cluster_to_node[cid] == kInvalidNode) {
        cluster_to_node[cid] = new_cid++;
      }
      node_to_contracted_node[node] = cluster_to_node[cid];
      setClusterID(node, node_to_contracted_node[node]);
    }

    std::vector<NodeID> new_hypernode_mapping(_hypernode_mapping.size(), kInvalidNode);
    for (HypernodeID hn = 0; hn < _hypernode_mapping.size(); ++hn) {
      if (_hypernode_mapping[hn] != kInvalidNode) {
        new_hypernode_mapping[hn] = node_to_contracted_node[_hypernode_mapping[hn]];
      }
    }

    ASSERT([&]() {
          for (HypernodeID hn = 0; hn < _hypernode_mapping.size(); ++hn) {
            if (_hypernode_mapping[hn] != kInvalidNode &&
                static_cast<NodeID>(clusterID(_hypernode_mapping[hn])) !=
                new_hypernode_mapping[hn]) {
              LOG << V(clusterID(_hypernode_mapping[hn]));
              LOG << V(new_hypernode_mapping[hn]);
              return false;
            }
          }
          return true;
        } (), "Hypernodes are not correctly mapped to contracted graph");


    std::vector<ClusterID> clusterID(new_cid);
    std::iota(clusterID.begin(), clusterID.end(), 0);

    std::vector<NodeID> node_ids(_num_nodes);
    std::iota(node_ids.begin(), node_ids.end(), 0);

    std::sort(node_ids.begin(), node_ids.end(), [&](const NodeID& n1, const NodeID& n2) {
          return _cluster_id[n1] < _cluster_id[n2] || (_cluster_id[n1] == _cluster_id[n2] && n1 < n2);
        });

    // Add Sentinels
    node_ids.push_back(_cluster_id.size());
    _cluster_id.push_back(new_cid);

    std::vector<NodeID> new_adj_array(new_cid + 1, 0);
    std::vector<Edge> new_edges;
    size_t start_idx = 0;
    for (NodeID i = 0; i < _num_nodes + 1; ++i) {
      if (_cluster_id[node_ids[start_idx]] != _cluster_id[node_ids[i]]) {
        const ClusterID cid = _cluster_id[node_ids[start_idx]];
        new_adj_array[cid] = new_edges.size();
        auto cluster_range = std::make_pair(node_ids.begin() + start_idx,
                                            node_ids.begin() + i);
        for (const auto& incident_cluster_weight : incidentClusterWeightOfCluster(cluster_range)) {
          Edge e;
          e.target_node = static_cast<NodeID>(incident_cluster_weight.clusterID);
          e.weight = incident_cluster_weight.weight;
          new_edges.push_back(e);
        }
        start_idx = i;
      }
    }

    // Remove Sentinels
    node_ids.pop_back();
    _cluster_id.pop_back();

    new_adj_array[new_cid] = new_edges.size();

    return std::make_pair(Graph(new_adj_array, new_edges, new_hypernode_mapping, clusterID),
                          node_to_contracted_node);
  }

  void printGraph() {
    std::cout << "Number Nodes:" << numNodes() << std::endl;
    std::cout << "Number Edges:" << numEdges() << std::endl;

    for (const NodeID& n : nodes()) {
      std::cout << "Node ID:" << n << "(Comm.:" << clusterID(n) << "), Adj. List: ";
      for (const Edge& e : incidentEdges(n)) {
        std::cout << "(" << e.target_node << ",w=" << e.weight << ") ";
      }
      std::cout << "\n";
    }
  }

 private:
  FRIEND_TEST(ABipartiteGraph, DeterminesIncidentClusterWeightsOfAClusterCorrect);
  FRIEND_TEST(ACliqueGraph, DeterminesIncidentClusterWeightsOfAClusterCorrect);
  FRIEND_TEST(ALouvainKarateClub, DoesLouvainAlgorithm);


  Graph(const std::vector<NodeID>& adj_array, const std::vector<Edge>& edges,
        const std::vector<NodeID>& new_hypernode_mapping,
        const std::vector<ClusterID>& cluster_id) :
    _num_nodes(adj_array.size() - 1),
    _num_communities(0),
    _total_weight(0.0L),
    _is_graph(true),
    _adj_array(adj_array),
    _edges(edges),
    _selfloop_weight(_num_nodes, 0.0L),
    _weighted_degree(_num_nodes, 0.0L),
    _cluster_id(cluster_id),
    _cluster_size(_num_nodes, 0),
    _incident_cluster_weight(_num_nodes, IncidentClusterWeight(0, 0.0L)),
    _incident_cluster_weight_position(_num_nodes),
    _hypernode_mapping(new_hypernode_mapping) {
    for (const NodeID& node : nodes()) {
      if (_cluster_size[_cluster_id[node]] == 0) {
        _num_communities++;
      }
      _cluster_size[_cluster_id[node]]++;
      for (const Edge& e : incidentEdges(node)) {
        if (node == e.target_node) {
          _selfloop_weight[node] = e.weight;
        }
        _total_weight += e.weight;
        _weighted_degree[node] += e.weight;
      }
    }
  }

  /**
   * Creates an iterator pair to the weights of all clusters incident to the cluster
   * that is implicitly defined by the nodes in range cluster_range.
   * This is a helper method for contractCluster.
   */
  std::pair<IncidentClusterWeightIterator,
            IncidentClusterWeightIterator> incidentClusterWeightOfCluster(const std::pair<NodeIterator, NodeIterator>& cluster_range) {
    ASSERT(std::all_of(cluster_range.first, cluster_range.second,
                       [&](const NodeID i) { return clusterID(i) == clusterID(*cluster_range.first); }));
    _incident_cluster_weight_position.clear();
    size_t idx = 0;

    for (const NodeID& node : cluster_range) {
      for (const Edge& e : incidentEdges(node)) {
        const NodeID id = e.target_node;
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

    auto incident_cluster_weight_range = std::make_pair(_incident_cluster_weight.begin(),
                                                        _incident_cluster_weight.begin() + idx);

    ASSERT([&]() {
          std::set<ClusterID> incident_cluster;
          for (const NodeID& node : cluster_range) {
            for (const Edge& e : incidentEdges(node)) {
              const ClusterID cid = clusterID(e.target_node);
              if (cid != -1) {
                incident_cluster.insert(cid);
              }
            }
          }
          for (const auto& cluster : incident_cluster_weight_range) {
            const ClusterID cid = cluster.clusterID;
            const EdgeWeight weight = cluster.weight;
            if (incident_cluster.find(cid) == incident_cluster.end()) {
              LOG << "ClusterID" << cid << "occurs multiple times or is not incident to cluster!";
              return false;
            }
            EdgeWeight incident_weight = 0.0L;
            for (const NodeID& node : cluster_range) {
              for (const Edge& e : incidentEdges(node)) {
                const ClusterID inc_cid = clusterID(e.target_node);
                if (inc_cid == cid) {
                  incident_weight += e.weight;
                }
              }
            }
            if (std::abs(incident_weight - weight) > kEpsilon) {
              LOG << "Weight calculation of incident cluster" << cid << "failed!";
              LOG << V(incident_weight);
              LOG << V(weight);
              return false;
            }
            incident_cluster.erase(cid);
          }

          if (incident_cluster.size() > 0) {
            LOG << "Missing cluster ids in iterator!";
            for (const ClusterID& cid : incident_cluster) {
              LOG << V(cid);
            }
            return false;
          }
          return true;
        } (), "Incident cluster weight calculation failed!");

    return incident_cluster_weight_range;
  }

  template <typename EdgeWeightFunction>
  void constructGraph(const Hypergraph& hg, const EdgeWeightFunction& edgeWeight) {
    NodeID sum_edges = 0;
    NodeID cur_node_id = 0;

    // Construct adj. array for all hypernodes.
    // Number of edges is equal to the degree of the corresponding hypernode.
    for (const HypernodeID& hn : hg.nodes()) {
      _hypernode_mapping[hn] = cur_node_id;
      _adj_array[cur_node_id++] = sum_edges;
      sum_edges += hg.nodeDegree(hn);
    }

    _adj_array[_num_nodes] = sum_edges;
    _edges.resize(sum_edges);

    for (const HypernodeID& hn : hg.nodes()) {
      size_t pos = 0;
      const NodeID graph_node = _hypernode_mapping[hn];
      for (const HyperedgeID& he : hg.incidentEdges(hn)) {
        for (const HypernodeID& pin : hg.pins(he)) {
          if (pin != hn) {
            Edge e;
            e.target_node = _hypernode_mapping[pin];
            e.weight = edgeWeight(hg, he, hn);
            _total_weight += e.weight;
            _weighted_degree[graph_node] += e.weight;
            _edges[_adj_array[graph_node] + pos++] = e;
          }
        }
      }
    }

    ASSERT([&]() {
          // Check Hypernodes in Graph
          for (const HypernodeID& hn : hg.nodes()) {
            if (hg.nodeDegree(hn) != degree(_hypernode_mapping[hn])) {
              LOG << V(hg.nodeDegree(hn));
              LOG << V(degree(_hypernode_mapping[hn]));
              return false;
            }
            std::set<HypernodeID> pins;
            for (const HyperedgeID& he : hg.incidentEdges(hn)) {
              for (const HypernodeID& pin : hg.pins(he)) {
                if (pin != hn) {
                  pins.insert(_hypernode_mapping[pin]);
                }
              }
            }
            for (const Edge& e : incidentEdges(_hypernode_mapping[hn])) {
              if (pins.find(e.target_node) == pins.end()) {
                LOG << V(hn);
                LOG << V(_hypernode_mapping[hn]);
                LOG << V(e.target_node);
                return false;
              }
            }
          }
          return true;
        } (), "Graph is not equivalent with hypergraph");
  }


  template <typename EdgeWeightFunction>
  void constructBipartiteGraph(const Hypergraph& hg, const EdgeWeightFunction& edgeWeight) {
    NodeID sum_edges = 0;

    const auto num_nodes = static_cast<size_t>(hg.initialNumNodes());

    NodeID cur_node_id = 0;

    // Construct adj. array for all hypernodes.
    // Number of edges is equal to the degree of the corresponding hypernode.
    for (const HypernodeID& hn : hg.nodes()) {
      _hypernode_mapping[hn] = cur_node_id;
      _adj_array[cur_node_id++] = sum_edges;
      sum_edges += hg.nodeDegree(hn);
    }

    // Construct adj. array for all hyperedges.
    // Number of edges is equal to the size of the corresponding hyperedge.
    for (const HyperedgeID& he : hg.edges()) {
      _hypernode_mapping[num_nodes + he] = cur_node_id;
      _adj_array[cur_node_id++] = sum_edges;
      sum_edges += hg.edgeSize(he);
    }

    _adj_array[_num_nodes] = sum_edges;
    _edges.resize(sum_edges);

    for (const HypernodeID& hn : hg.nodes()) {
      size_t pos = 0;
      const NodeID graph_node = _hypernode_mapping[hn];
      for (const HyperedgeID& he : hg.incidentEdges(hn)) {
        Edge e;
        e.target_node = _hypernode_mapping[num_nodes + he];
        e.weight = edgeWeight(hg, he, hn);
        _total_weight += e.weight;
        _weighted_degree[graph_node] += e.weight;
        _edges[_adj_array[graph_node] + pos++] = e;
      }
    }

    for (const HyperedgeID& he : hg.edges()) {
      size_t pos = 0;
      for (const HypernodeID& hn : hg.pins(he)) {
        Edge e;
        e.target_node = _hypernode_mapping[hn];
        e.weight = edgeWeight(hg, he, hn);
        const NodeID cur_node = _hypernode_mapping[num_nodes + he];
        _total_weight += e.weight;
        _weighted_degree[cur_node] += e.weight;
        _edges[_adj_array[cur_node] + pos++] = e;
      }
    }


    ASSERT([&]() {
          // Check Hypernodes in Graph
          for (const HypernodeID& hn : hg.nodes()) {
            if (hg.nodeDegree(hn) != degree(_hypernode_mapping[hn])) {
              LOG << V(hg.nodeDegree(hn));
              LOG << V(degree(_hypernode_mapping[hn]));
              return false;
            }
            std::set<HyperedgeID> incident_edges;
            for (const HyperedgeID& he : hg.incidentEdges(hn)) {
              incident_edges.insert(_hypernode_mapping[num_nodes + he]);
            }
            for (const Edge& e : incidentEdges(_hypernode_mapping[hn])) {
              const HyperedgeID he = e.target_node;
              if (incident_edges.find(he) == incident_edges.end()) {
                LOG << V(_hypernode_mapping[hn]);
                LOG << V(he);
                return false;
              }
            }
          }

          // Checks Hyperedges in Graph
          for (const HyperedgeID& he : hg.edges()) {
            if (hg.edgeSize(he) != degree(_hypernode_mapping[he + num_nodes])) {
              LOG << V(hg.edgeSize(he));
              LOG << V(degree(_hypernode_mapping[he + num_nodes]));
              return false;
            }
            std::set<HypernodeID> pins;
            for (const HypernodeID& hn : hg.pins(he)) {
              pins.insert(_hypernode_mapping[hn]);
            }
            for (const Edge& e : incidentEdges(_hypernode_mapping[he + num_nodes])) {
              if (pins.find(e.target_node) == pins.end()) {
                LOG << V(_hypernode_mapping[he + num_nodes]);
                LOG << V(e.target_node);
                return false;
              }
            }
          }
          return true;
        } (), "Bipartite Graph is not equivalent with hypergraph");
  }

  NodeID _num_nodes;
  size_t _num_communities;
  EdgeWeight _total_weight;
  bool _is_graph;
  std::vector<NodeID> _adj_array;
  std::vector<Edge> _edges;
  std::vector<EdgeWeight> _selfloop_weight;
  std::vector<EdgeWeight> _weighted_degree;
  std::vector<ClusterID> _cluster_id;
  std::vector<size_t> _cluster_size;
  std::vector<IncidentClusterWeight> _incident_cluster_weight;
  SparseMap<ClusterID, size_t> _incident_cluster_weight_position;
  std::vector<NodeID> _hypernode_mapping;
};

constexpr NodeID Graph::kInvalidNode;
constexpr long double Graph::kEpsilon;
}  // namespace ds
}  // namespace kahypar
