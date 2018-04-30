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
  static inline void calculateStartNodes(std::vector<std::vector<HypernodeID> >& start_nodes,
                                         const Context& context, const Hypergraph& hg,
                                         const PartitionID k) {
    ds::FastResetFlagArray<> in_queue(hg.initialNumNodes());
    ds::FastResetFlagArray<> hyperedge_in_queue(hg.initialNumEdges());

    bool start_nodes_empty = true;
    for (PartitionID i = 0; i < k; ++i) {
      start_nodes_empty = start_nodes_empty && start_nodes[i].size() == 0;
    }

    if (start_nodes_empty) {
      if (UseRandomStartHypernode) {
        const HypernodeID start_hn = Randomize::instance().getRandomInt(0, hg.initialNumNodes() - 1);
        start_nodes[0].push_back(start_hn);
      } else {
        start_nodes[0].push_back(0);
      }
    }

    PartitionID cur_part = nextPartID(start_nodes, k);
    while (cur_part != k) {
      std::queue<HypernodeID> bfs;
      HypernodeID lastHypernode = -1;
      size_t visited_nodes = 0;
      initializeQueue(start_nodes, bfs, in_queue, k);

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
      start_nodes[cur_part].push_back(lastHypernode);
      cur_part = nextPartID(start_nodes, k);
      in_queue.reset();
      hyperedge_in_queue.reset();
    }
  }

  static inline void initializeQueue(std::vector<std::vector<HypernodeID> >& start_nodes,
                                     std::queue<HypernodeID>& q,
                                     ds::FastResetFlagArray<>& in_queue,
                                     const PartitionID k) {
    ASSERT(static_cast<PartitionID>(start_nodes.size()) == k, "Size of start nodes are not equal to" << k);

    for (PartitionID i = 0; i < k; ++i) {
      for (const HypernodeID& hn : start_nodes[i]) {
        q.push(hn);
        in_queue.set(hn, true);
      }
    }
  }

  static inline PartitionID nextPartID(std::vector<std::vector<HypernodeID> >& start_nodes,
                                       const PartitionID k) {
    ASSERT(static_cast<PartitionID>(start_nodes.size()) == k, "Size of start nodes are not equal to" << k);
    for (PartitionID i = 0; i < k; ++i) {
      if (start_nodes[i].size() == 0) {
        return i;
      }
    }
    return k;
  }
};
}  // namespace kahypar
