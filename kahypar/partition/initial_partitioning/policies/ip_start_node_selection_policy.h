/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
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
#include <queue>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/definitions.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
template <bool UseRandomStartHypernode = true>
class BFSStartNodeSelectionPolicy {
 public:
  static inline void calculateStartNodes(std::vector<HypernodeID>& start_nodes, const Context& context,
                                         const Hypergraph& hg, const PartitionID k) {
    HypernodeID start_hn = 0;
    if (UseRandomStartHypernode) {
      start_hn = Randomize::instance().getRandomInt(0, hg.initialNumNodes() - 1);
    }
    start_nodes.push_back(start_hn);
    ds::FastResetFlagArray<> in_queue(hg.initialNumNodes());
    ds::FastResetFlagArray<> hyperedge_in_queue(hg.initialNumEdges());

    while (start_nodes.size() != static_cast<size_t>(k)) {
      std::queue<HypernodeID> bfs;
      HypernodeID lastHypernode = -1;
      size_t visited_nodes = 0;
      for (const HypernodeID& start_node : start_nodes) {
        bfs.push(start_node);
        in_queue.set(start_node, true);
      }


      while (!bfs.empty()) {
        lastHypernode = bfs.front();
        bfs.pop();
        visited_nodes++;
        for (const HyperedgeID& he : hg.incidentEdges(lastHypernode)) {
          if (!hyperedge_in_queue[he]) {
            if (hg.edgeSize(he) <= context.partition.hyperedge_size_threshold) {
              for (const HypernodeID& pin : hg.pins(he)) {
                if (!in_queue[pin]) {
                  bfs.push(pin);
                  in_queue.set(pin, true);
                }
              }
            }
            hyperedge_in_queue.set(he, true);
          }
        }
        if (bfs.empty() && visited_nodes != hg.initialNumNodes()) {
          for (const HypernodeID& hn : hg.nodes()) {
            if (!in_queue[hn]) {
              bfs.push(hn);
              in_queue.set(hn, true);
            }
          }
        }
      }
      start_nodes.push_back(lastHypernode);
      in_queue.reset();
      hyperedge_in_queue.reset();
    }
  }
};

class RandomStartNodeSelectionPolicy {
 public:
  static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes, const Context&,
                                         const Hypergraph& hg, const PartitionID k) {
    if (k == 2) {
      startNodes.push_back(Randomize::instance().getRandomInt(0, hg.initialNumNodes() - 1));
      return;
    }

    for (PartitionID i = 0; i < k; ++i) {
      while (true) {
        HypernodeID hn = Randomize::instance().getRandomInt(0, hg.initialNumNodes() - 1);
        auto node = std::find(startNodes.begin(), startNodes.end(), hn);
        if (node == startNodes.end()) {
          startNodes.push_back(hn);
          break;
        }
      }
    }
  }
};
}  // namespace kahypar
