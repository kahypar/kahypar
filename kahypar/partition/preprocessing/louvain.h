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
#include <limits>
#include <vector>

#include "kahypar/datastructure/graph.h"
#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/preprocessing/modularity.h"
#include "kahypar/utils/randomize.h"
#include "kahypar/utils/stats.h"
#include "kahypar/utils/timer.h"

static constexpr bool debug = false;

namespace kahypar {
template <class QualityMeasure = Mandatory,
          bool RandomizeNodes = true>
class Louvain {
 private:
  using Edge = ds::Edge;
  using Graph = ds::Graph;

 public:
  Louvain(const Hypergraph& hypergraph,
          const Context& context) :
    _graph_hierarchy(),
    _random_node_order(),
    _context(context) {
    _graph_hierarchy.emplace_back(hypergraph, context);
  }

  Louvain(const std::vector<NodeID>& adj_array,
          const std::vector<Edge>& edges,
          const Context& context) :
    _graph_hierarchy(),
    _random_node_order(),
    _context(context) {
    _graph_hierarchy.emplace_back(adj_array, edges);
  }

  EdgeWeight run() {
    bool improvement = false;
    size_t iteration = 0;
    EdgeWeight old_quality = -1.0L;
    EdgeWeight cur_quality = -1.0L;

    const size_t max_passes = std::numeric_limits<size_t>::max();


    std::vector<std::vector<NodeID> > mapping_stack;
    ASSERT(_graph_hierarchy.size() == 1);
    int cur_idx = 0;

    do {
      DBG << "Graph Number Nodes:" << _graph_hierarchy[cur_idx].numNodes();
      DBG << "Graph Number Edges:" << _graph_hierarchy[cur_idx].numEdges();
      QualityMeasure quality(_graph_hierarchy[cur_idx]);
      if (iteration == 0) {
        cur_quality = quality.quality();
      }

      ++iteration;
      DBG << "######## Starting Louvain-Pass #" << iteration << "########";

      // Checks if quality of the coarse graph is equal with the quality of next level finer graph
      ASSERT([&]() {
          if (cur_idx == 0) return true;
          if (std::abs(cur_quality - quality.quality()) > Graph::kEpsilon) {
            LOG << V(cur_quality);
            LOG << V(quality.quality());
            return false;
          }
          return true;
        } (), "Quality of contracted graph does not match quality of uncontracted graph");

      old_quality = cur_quality;
      HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
      cur_quality = louvain_pass(_graph_hierarchy[cur_idx], quality);
      HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed_seconds = end - start;
      DBG << "Louvain-Pass #" << iteration << "Time:" << elapsed_seconds.count() << "s";
      improvement = cur_quality - old_quality > _context.preprocessing.community_detection.min_eps_improvement;

      DBG << "Louvain-Pass #" << iteration << "improved quality from" << old_quality
          << "to" << cur_quality;

      if (improvement) {
        cur_quality = quality.quality();
        DBG << "Starting Contraction of communities...";
        start = std::chrono::high_resolution_clock::now();
        auto contraction = _graph_hierarchy[cur_idx++].contractClusters();
        end = std::chrono::high_resolution_clock::now();
        elapsed_seconds = end - start;
        DBG << "Contraction Time:" << elapsed_seconds.count() << "s";
        _graph_hierarchy.push_back(std::move(contraction.first));
        mapping_stack.push_back(std::move(contraction.second));
        DBG << "Current number of communities:" << _graph_hierarchy[cur_idx].numCommunities();
      }

      DBG << "";
    } while (improvement && iteration < max_passes);

    ASSERT((mapping_stack.size() + 1) == _graph_hierarchy.size());
    while (!mapping_stack.empty()) {
      assignClusterToNextLevelFinerGraph(_graph_hierarchy[cur_idx - 1], _graph_hierarchy[cur_idx],
                                         mapping_stack[cur_idx - 1]);
      _graph_hierarchy.pop_back();
      mapping_stack.pop_back();
      cur_idx--;
    }

    ASSERT(_graph_hierarchy.size() == 1);
    return cur_quality;
  }


  Graph getGraph() {
    return _graph_hierarchy[0];
  }

  ClusterID clusterID(const NodeID node) const {
    return _graph_hierarchy[0].clusterID(node);
  }

  ClusterID hypernodeClusterID(const HypernodeID hn) const {
    return _graph_hierarchy[0].hypernodeClusterID(hn);
  }

  size_t numCommunities() const {
    return _graph_hierarchy[0].numCommunities();
  }

 private:
  FRIEND_TEST(ALouvainAlgorithm, DoesOneLouvainPass);
  FRIEND_TEST(ALouvainAlgorithm, AssingsMappingToNextLevelFinerGraph);
  FRIEND_TEST(ALouvainKarateClub, DoesLouvainAlgorithm);

  void assignClusterToNextLevelFinerGraph(Graph& fine_graph, const Graph& coarse_graph,
                                          const std::vector<NodeID>& mapping) {
    for (const NodeID& node : fine_graph.nodes()) {
      fine_graph.setClusterID(node, coarse_graph.clusterID(mapping[node]));
    }
  }

  EdgeWeight louvain_pass(Graph& graph, QualityMeasure& quality) {
    size_t node_moves = 0;
    uint32_t iterations = 0;

    _random_node_order.clear();
    for (const NodeID& node : graph.nodes()) {
      _random_node_order.push_back(node);
    }

    // only false for testing purposes
    if (RandomizeNodes) {
      Randomize::instance().shuffleVector(_random_node_order, _random_node_order.size());
    }

    // PERFORMANCE TUNING:
    // A node can only change its cluster if some of its incident clusters
    // changed their weight. Therefore we introduce a time stamp which tracks
    // at which time a node was processed for the last time and at which time
    // a cluster changed its weight. Consider a node u is incident to a cluster
    // id, then u might change its cluster if node_time_stamp[u] <
    // cluster_time_stamp[id].
    std::vector<size_t> node_time_stamp(graph.numNodes(), 0);
    std::vector<size_t> cluster_time_stamp(graph.numNodes(), graph.numNodes());
    size_t time_stamp = 0;

    do {
      ++iterations;
      DBG << "######## Starting Louvain-Pass-Iteration #" << iterations << "########";
      node_moves = 0;
      for (const NodeID& node : _random_node_order) {
        const ClusterID cur_cid = graph.clusterID(node);
        EdgeWeight cur_incident_cluster_weight = 0.0L;
        ClusterID best_cid = cur_cid;
        EdgeWeight best_incident_cluster_weight = 0.0L;
        EdgeWeight best_gain = 0.0L;

        bool incident_cluster_changed = false;
        for (const Edge& e : graph.incidentEdges(node)) {
          const NodeID v = e.target_node;
          const ClusterID v_cid = graph.clusterID(v);
          if (v_cid == cur_cid && v != node) {
            cur_incident_cluster_weight += e.weight;
          }
          if (node_time_stamp[node] < cluster_time_stamp[v_cid]) {
            incident_cluster_changed = true;
          }
        }
        best_incident_cluster_weight = cur_incident_cluster_weight;

        if (incident_cluster_changed) {
          quality.remove(node, cur_incident_cluster_weight);

          for (const auto& cluster : graph.incidentClusterWeightOfNode(node)) {
            const ClusterID cid = cluster.clusterID;
            const EdgeWeight weight = cluster.weight;
            const EdgeWeight gain = quality.gain(node, cid, weight);
            if (gain > best_gain) {
              best_gain = gain;
              best_incident_cluster_weight = weight;
              best_cid = cid;
            }
          }

          quality.insert(node, best_cid, best_incident_cluster_weight);

          if (best_cid != cur_cid) {
            cluster_time_stamp[cur_cid] = std::max(time_stamp, cluster_time_stamp[cur_cid]);
            cluster_time_stamp[best_cid] = std::max(time_stamp, cluster_time_stamp[best_cid]);
            ++node_moves;
          }
        }
        node_time_stamp[node] = time_stamp++;
      }

      DBG << "Iteration #" << iterations << ": Moving" << node_moves << "nodes to new communities.";
    } while (node_moves > 0 &&
             iterations < _context.preprocessing.community_detection.max_pass_iterations);


    return quality.quality();
  }

  std::vector<Graph> _graph_hierarchy;
  std::vector<NodeID> _random_node_order;
  const Context& _context;
};

namespace internal {
inline std::vector<ClusterID> detectCommunities(const Hypergraph& hypergraph,
                                                const Context& context) {
  const bool verbose_output = (context.type == ContextType::main &&
                               context.partition.verbose_output) ||
                              (context.type == ContextType::initial_partitioning &&
                               context.initial_partitioning.verbose_output);
  if (verbose_output) {
    LOG << "Performing community detection:";
  }

  Louvain<Modularity> louvain(hypergraph, context);
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  const EdgeWeight quality = louvain.run();
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  Timer::instance().add(context, Timepoint::pre_community_detection,
                        std::chrono::duration<double>(end - start).count());
  context.stats.set(StatTag::Preprocessing, "Communities", louvain.numCommunities());
  context.stats.set(StatTag::Preprocessing, "Modularity", quality);

  if (verbose_output) {
    LOG << "  # communities         =" << louvain.numCommunities();
    LOG << "  modularity            =" << quality;
  }

  std::vector<ClusterID> communities(hypergraph.initialNumNodes(), -1);
  for (const HypernodeID& hn : hypergraph.nodes()) {
    communities[hn] = louvain.hypernodeClusterID(hn);
  }
  ASSERT(std::none_of(communities.cbegin(), communities.cend(),
                      [](ClusterID i) {
        return i == -1;
      }));
  return communities;
}
}  // namespace internal

inline void detectCommunities(Hypergraph& hypergraph, const Context& context) {
  hypergraph.setCommunities(internal::detectCommunities(hypergraph, context));
}
}  // namespace kahypar
