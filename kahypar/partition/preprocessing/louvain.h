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
#include "kahypar/partition/configuration.h"

namespace kahypar {
using ds::Graph;
using ds::Edge;

#define EPS 1e-5

template <class QualityMeasure = Mandatory>
class Louvain {
 public:
  Louvain(const Hypergraph& hypergraph, const Configuration& config) :
    _graph(hypergraph, config),
    _config(config) { }

  Louvain(const std::vector<NodeID>& adj_array, const std::vector<Edge>& edges, const Configuration& config) :
    _graph(adj_array, edges),
    _config(config) { }

  EdgeWeight run() {
    return run(_graph);
  }

  Graph getGraph() {
    return _graph;
  }

  ClusterID clusterID(const HypernodeID hn) const {
    return _graph.hypernodeClusterID(hn);
  }

  ClusterID hypernodeClusterID(const HypernodeID hn) const {
    return _graph.hypernodeClusterID(hn);
  }

  ClusterID hyperedgeClusterID(const HyperedgeID he, const HypernodeID num_hns) const {
    return _graph.hyperedgeClusterID(he, num_hns);
  }

  size_t numCommunities() const {
    return _graph.numCommunities();
  }

 private:
  FRIEND_TEST(ALouvainAlgorithm, DoesOneLouvainPass);
  FRIEND_TEST(ALouvainAlgorithm, AssingsMappingToNextLevelFinerGraph);
  FRIEND_TEST(ALouvainKarateClub, DoesLouvainAlgorithm);

  EdgeWeight run(Graph& graph) {
    bool improvement = false;
    size_t iteration = 0;
    EdgeWeight old_quality = -1.0L;
    EdgeWeight cur_quality = -1.0L;

    const size_t max_iterations = std::numeric_limits<size_t>::max();


    std::vector<std::vector<NodeID> > mapping_stack;
    std::vector<Graph> graph_stack;

    graph_stack.emplace_back(std::move(graph));
    int cur_idx = 0;

    do {
      LOG("Graph Number Nodes: " << graph_stack[cur_idx].numNodes());
      LOG("Graph Number Edges: " << graph_stack[cur_idx].numEdges());
      QualityMeasure quality(graph_stack[cur_idx], _config);
      if (iteration == 0) {
        cur_quality = quality.quality();
      }

      ++iteration;
      LOG("######## Starting Louvain-Pass #" << iteration << " ########");

      //Checks if quality of the coarse graph is equal with the quality of next level finer graph
      ASSERT([&]() {
          if (cur_idx == 0) return true;
          if (std::abs(cur_quality - quality.quality()) > EPS) {
            LOGVAR(cur_quality);
            LOGVAR(quality.quality());
            return false;
          }
          return true;
        } (), "Quality of the contracted graph is not equal with quality of its corresponding uncontracted graph!");

      old_quality = cur_quality;
      HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
      cur_quality = louvain_pass(graph_stack[cur_idx], quality);
      HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed_seconds = end - start;
      LOG("Louvain-Pass #" << iteration << " Time: " << elapsed_seconds.count() << "s");
      improvement = cur_quality - old_quality > _config.preprocessing.louvain_community_detection.min_eps_improvement || max_iterations == 2;

      LOG("Louvain-Pass #" << iteration << " improve quality from " << old_quality << " to " << cur_quality);

      if (improvement) {
        cur_quality = quality.quality();
        LOG("Starting Contraction of communities...");
        start = std::chrono::high_resolution_clock::now();
        auto contraction = graph_stack[cur_idx++].contractCluster();
        end = std::chrono::high_resolution_clock::now();
        elapsed_seconds = end - start;
        LOG("Contraction Time: " << elapsed_seconds.count() << "s");
        graph_stack.push_back(std::move(contraction.first));
        mapping_stack.push_back(std::move(contraction.second));
        LOG("Current number of communities: " << graph_stack[cur_idx].numNodes());
      }

      LOG("");
    } while (improvement && iteration < max_iterations);

    ASSERT((mapping_stack.size() + 1) == graph_stack.size(), "Unequality between graph and mapping stack!");
    while (!mapping_stack.empty()) {
      assignClusterToNextLevelFinerGraph(graph_stack[cur_idx - 1], graph_stack[cur_idx], mapping_stack[cur_idx - 1]);
      graph_stack.pop_back();
      mapping_stack.pop_back();
      cur_idx--;
    }

    graph = std::move(graph_stack[0]);

    return cur_quality;
  }

  void assignClusterToNextLevelFinerGraph(Graph& fineGraph, Graph& coarseGraph, std::vector<NodeID>& mapping) {
    for (NodeID node : fineGraph.nodes()) {
      fineGraph.setClusterID(node, coarseGraph.clusterID(mapping[node]));
    }

    //Check if cluster ids from coarse graph are succesfully mapped to the nodes of
    //next level finer graph
    ASSERT([&]() {
        for (NodeID node : fineGraph.nodes()) {
          if (fineGraph.clusterID(node) != coarseGraph.clusterID(mapping[node])) {
            LOGVAR(fineGraph.clusterID(node));
            LOGVAR(coarseGraph.clusterID(mapping[node]));
            return false;
          }
        }
        return true;
      } (), "Mapping from coarse to fine graph failed!");
  }

  EdgeWeight louvain_pass(Graph& g, QualityMeasure& quality) {
    size_t node_moves = 0;
    int iterations = 0;

    //TODO(heuer): Think about shuffling nodes before louvain pass

    g.shuffleNodes();

    do {
      LOG("######## Starting Louvain-Pass-Iteration #" << ++iterations << " ########");
      node_moves = 0;
      for (NodeID node : g.nodes()) {
        const ClusterID cur_cid = g.clusterID(node);
        EdgeWeight cur_incident_cluster_weight = 0.0L;
        ClusterID best_cid = cur_cid;
        EdgeWeight best_incident_cluster_weight = 0.0L;
        EdgeWeight best_gain = 0.0L;

        for (Edge e : g.incidentEdges(node)) {
          if (g.clusterID(e.targetNode) == cur_cid && e.targetNode != node) {
            cur_incident_cluster_weight += e.weight;
          }
        }
        best_incident_cluster_weight = cur_incident_cluster_weight;

        quality.remove(node, cur_incident_cluster_weight);

        for (auto cluster : g.incidentClusterWeightOfNode(node)) {
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
          /*ASSERT([&]() {
              quality.remove(node,best_incident_cluster_weight); //Remove node from best cluster...
              quality.insert(node,cur_cid,cur_incident_cluster_weight); // ... and insert in his old cluster.
              EdgeWeight quality_before = quality.quality();
              quality.remove(node,cur_incident_cluster_weight); //Remove node again from his old cluster ...
              quality.insert(node,best_cid,best_incident_cluster_weight); //... and insert it in cluster with best gain.
              EdgeWeight quality_after = quality.quality();
              if(quality_after - quality_before < -EPS) {
                  LOGVAR(quality_before);
                  LOGVAR(quality_after);
                  return false;
              }
              return true;
          }(),"Move did not increase the quality!");*/

          ++node_moves;
        }
      }

      LOG("Iteration #" << iterations << ": Moving " << node_moves << " nodes to new communities.");
    } while (node_moves > 0 && iterations < _config.preprocessing.louvain_community_detection.max_pass_iterations);


    return quality.quality();
  }

  Graph _graph;
  const Configuration& _config;
};
}  // namespace kahypar
