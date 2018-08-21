/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <utility>
#include <vector>

#include "kahypar/datastructure/graph.h"
#include "kahypar/definitions.h"

namespace kahypar {
using ds::Graph;
using ds::Edge;

class StronglyConnectedComponents {
 public:
  explicit StronglyConnectedComponents(size_t max_num_nodes) :
    _dfs_num(max_num_nodes, -1),
    _unfinished(),
    _roots(),
    _call_stack() {
    _unfinished.reserve(max_num_nodes);
    _roots.reserve(max_num_nodes);
  }

  void compute(Graph& graph) {
    ASSERT(_roots.empty());
    ASSERT(_unfinished.empty());
    ASSERT(_call_stack.empty());
    _call_stack.reserve(graph.numEdges());

    std::fill(_dfs_num.begin(), _dfs_num.end(), -1);
    for (const NodeID& u : graph.nodes()) {
      graph.setClusterID(u, -1);
    }

    NodeID dfs_num = 0;
    ClusterID cid = 0;
    for (const NodeID& u : graph.nodes()) {
      if (_dfs_num[u] == -1) {
        scc(u, graph, dfs_num, cid);
      }
    }
  }

 private:
  // Cheriyan/Mehlhorn 96, Gabow 2000
  void scc(const NodeID node, Graph& g, NodeID& dfs_num, ClusterID& cid) {
    using EdgeIterator = Graph::EdgeIterator;

    ASSERT(_call_stack.empty());
    _call_stack.push_back(std::make_pair(node, g.firstEdge(node)));
    _dfs_num[node] = dfs_num++;
    _unfinished.push_back(node);
    _roots.push_back(node);

    while (!_call_stack.empty()) {
      const NodeID current_node = _call_stack.back().first;
      const EdgeIterator current_edge = _call_stack.back().second;
      _call_stack.pop_back();

      const EdgeIterator first_invalid_edge = g.firstInvalidEdge(current_node);
      for (EdgeIterator e = current_edge; e != first_invalid_edge; ++e) {
        const NodeID target = e->target_node;

        if (_dfs_num[target] == -1) {
          _call_stack.push_back(std::make_pair(current_node, e));
          _call_stack.push_back(std::make_pair(target, g.firstEdge(target)));

          _dfs_num[target] = dfs_num++;
          _unfinished.push_back(target);
          _roots.push_back(target);
          break;
        } else if (g.clusterID(target) == -1) {
          while (_dfs_num[_roots.back()] > _dfs_num[target]) {
            _roots.pop_back();
          }
        }
      }

      if (current_node == _roots.back()) {
        NodeID w = Graph::kInvalidNode;
        do {
          w = _unfinished.back();
          _unfinished.pop_back();
          g.setClusterID(w, cid);
        } while (w != current_node);
        ++cid;
        _roots.pop_back();
      }
    }
  }


  std::vector<int> _dfs_num;
  std::vector<NodeID> _unfinished;
  std::vector<NodeID> _roots;
  std::vector<std::pair<NodeID, Graph::EdgeIterator> > _call_stack;
};
}  // namespace kahypar
