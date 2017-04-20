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

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/graph.h"
#include "kahypar/definitions.h"
#include "kahypar/macros.h"

namespace kahypar {
const bool dbg_modularity_function = false;

class Modularity {
 private:
  using Edge = ds::Edge;
  using Graph = ds::Graph;

 public:
  explicit Modularity(Graph& graph) :
    _graph(graph),
    _internal_weight(graph.numNodes(), 0),
    _total_weight(graph.numNodes(), 0),
    _vis(graph.numNodes()) {
    for (const NodeID& node : _graph.nodes()) {
      ASSERT(static_cast<NodeID>(_graph.clusterID(node)) == node);
      _internal_weight[node] = _graph.selfloopWeight(node);
      _total_weight[node] = _graph.weightedDegree(node);
    }
  }

  void remove(const NodeID node, const EdgeWeight incident_community_weight) {
    ASSERT(node < _graph.numNodes(), "NodeID" << node << "doesn't exist!");
    const ClusterID cid = _graph.clusterID(node);

    _internal_weight[cid] -= 2.0L * incident_community_weight + _graph.selfloopWeight(node);
    _total_weight[cid] -= _graph.weightedDegree(node);

    _graph.setClusterID(node, -1);
  }

  void insert(const NodeID node, const ClusterID new_cid,
              const EdgeWeight incident_community_weight) {
    ASSERT(node < _graph.numNodes(), "NodeID" << node << "doesn't exist!");
    ASSERT(_graph.clusterID(node) == -1, "Node" << node << "isn't a isolated node!");

    _internal_weight[new_cid] += 2.0L * incident_community_weight + _graph.selfloopWeight(node);
    _total_weight[new_cid] += _graph.weightedDegree(node);

    _graph.setClusterID(node, new_cid);

    ASSERT([&]() {
        if (!dbg_modularity_function) return true;
        const EdgeWeight q = quality();
        return q < std::numeric_limits<EdgeWeight>::max();
      } (), "");
  }

  EdgeWeight gain(const NodeID node, const ClusterID cid,
                  const EdgeWeight incident_community_weight) {
    ASSERT(node < _graph.numNodes(), "NodeID" << node << "doesn't exist!");
    ASSERT(_graph.clusterID(node) == -1, "Node" << node << "isn't a isolated node!");

    const EdgeWeight totc = _total_weight[cid];
    const EdgeWeight m2 = _graph.totalWeight();
    const EdgeWeight w_degree = _graph.weightedDegree(node);

    const EdgeWeight gain = incident_community_weight - totc * w_degree / m2;

    ASSERT([&]() {
        if (!dbg_modularity_function) return true;
        const EdgeWeight modularity_before = modularity();
        insert(node, cid, incident_community_weight);
        const EdgeWeight modularity_after = modularity();
        remove(node, incident_community_weight);
        const EdgeWeight real_gain = modularity_after - modularity_before;
        if (std::abs(2.0L * gain / m2 - real_gain) > Graph::kEpsilon) {
          LOG << V(real_gain);
          LOG << V(2.0L * gain / m2);
          return false;
        }
        return true;
      } (), "Gain calculation failed!");

    return gain;
  }


  EdgeWeight quality() {
    EdgeWeight q = 0.0L;
    const EdgeWeight m2 = _graph.totalWeight();
    for (const NodeID& node : _graph.nodes()) {
      if (_total_weight[node] > Graph::kEpsilon) {
        q += _internal_weight[node] - (_total_weight[node] * _total_weight[node]) / m2;
      }
    }

    q /= m2;

    ASSERT(!dbg_modularity_function || std::abs(q - modularity()) < Graph::kEpsilon,
           "Calculated modularity (q=" << q << ") is not equal with the real modularity "
                                       << "(modularity=" << modularity() << ")!");

    return q;
  }

 private:
  FRIEND_TEST(AModularityMeasure, IsCorrectInitialized);
  FRIEND_TEST(AModularityMeasure, RemoveNodeFromCommunity);
  FRIEND_TEST(AModularityMeasure, InsertNodeInCommunity);
  FRIEND_TEST(AModularityMeasure, RemoveNodeFromCommunityWithMoreThanOneNode);

  EdgeWeight modularity() {
    EdgeWeight q = 0.0L;
    const EdgeWeight m2 = _graph.totalWeight();

    for (const NodeID& u : _graph.nodes()) {
      for (const Edge& edge : _graph.incidentEdges(u)) {
        const NodeID v = edge.target_node;
        _vis.set(v, true);
        if (_graph.clusterID(u) == _graph.clusterID(v)) {
          q += edge.weight - (_graph.weightedDegree(u) * _graph.weightedDegree(v)) / m2;
        }
      }
      for (const NodeID& v : _graph.nodes()) {
        if (_graph.clusterID(u) == _graph.clusterID(v) && !_vis[v]) {
          q -= (_graph.weightedDegree(u) * _graph.weightedDegree(v)) / m2;
        }
      }
      _vis.reset();
    }

    q /= m2;
    return q;
  }

  Graph& _graph;
  std::vector<EdgeWeight> _internal_weight;
  std::vector<EdgeWeight> _total_weight;
  ds::FastResetFlagArray<> _vis;
};
}  // namespace kahypar
