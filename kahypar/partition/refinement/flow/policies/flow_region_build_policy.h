/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
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
#include "kahypar/datastructure/flow_network.h"
#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/typelist.h"
#include "kahypar/partition/context.h"

namespace kahypar {
class FlowRegionBuildPolicy : public meta::PolicyBase {
 public:
  template <class Network = Mandatory>
  static inline void bfs(const Hypergraph& hg,
                         Network& flow_network,
                         std::vector<HypernodeID>& start_nodes,
                         const PartitionID part,
                         const HypernodeWeight max_part_weight,
                         FastResetFlagArray<>& visited) {
    visited.reset();
    std::random_shuffle(start_nodes.begin(), start_nodes.end());
    std::queue<HypernodeID> Q;
    HypernodeWeight queue_weight = 0;
    for (const HypernodeID& hn : start_nodes) {
      if (queue_weight + hg.nodeWeight(hn) <= max_part_weight) {
        Q.push(hn);
        queue_weight += hg.nodeWeight(hn);
        visited.set(hn, true);
      }
    }

    const size_t num_hypernodes = hg.initialNumNodes();
    while (!Q.empty()) {
      const HypernodeID hn = Q.front();
      Q.pop();

      flow_network.addHypernode(hn);

      for (const HyperedgeID& he : hg.incidentEdges(hn)) {
        if (!visited[num_hypernodes + he]) {
          for (const HypernodeID& pin : hg.pins(he)) {
            if (!visited[pin] && hg.partID(pin) == part &&
                queue_weight + hg.nodeWeight(pin) <= max_part_weight) {
              Q.push(pin);
              queue_weight += hg.nodeWeight(pin);
              visited.set(pin, true);
            }
          }
          visited.set(num_hypernodes + he, true);
        }
      }
    }
  }
};


class CutBuildPolicy : public FlowRegionBuildPolicy {
 public:
  template <class Network = Mandatory>
  inline static void buildFlowNetwork(const Hypergraph& hg,
                                      const Context& context,
                                      Network& flow_network,
                                      std::vector<HyperedgeID>& cut_hes,
                                      const double alpha,
                                      const PartitionID block_0,
                                      const PartitionID block_1,
                                      FastResetFlagArray<>& visited) {
    visited.reset();
    std::vector<HypernodeID> start_nodes_block_0;
    std::vector<HypernodeID> start_nodes_block_1;
    for (const HyperedgeID he : cut_hes) {
      ASSERT(hg.connectivity(he) > 1, "Hyperedge is not a cut hyperedge!");
      for (const HypernodeID& pin : hg.pins(he)) {
        if (!visited[pin]) {
          if (hg.partID(pin) == block_0) {
            start_nodes_block_0.push_back(pin);
          } else if (hg.partID(pin) == block_1) {
            start_nodes_block_1.push_back(pin);
          }
          visited.set(pin, true);
        }
      }
    }
    visited.reset();

    const HypernodeWeight max_part_weight_0 =
      std::max(((1.0 + std::min(alpha * context.partition.epsilon, 0.5))
                * context.partition.perfect_balance_part_weights[1]
                - hg.partWeight(block_1)), 0.0);
    const HypernodeWeight max_part_weight_1 =
      std::max(((1.0 + std::min(alpha * context.partition.epsilon, 0.5))
                * context.partition.perfect_balance_part_weights[0]
                - hg.partWeight(block_0)), 0.0);

    FlowRegionBuildPolicy::bfs<Network>(hg, flow_network, start_nodes_block_0,
                                        block_0, max_part_weight_0, visited);
    FlowRegionBuildPolicy::bfs<Network>(hg, flow_network, start_nodes_block_1,
                                        block_1, max_part_weight_1, visited);

    ASSERT([&]() {
        HypernodeWeight weight_block_0 = 0;
        HypernodeWeight weight_block_1 = 0;
        for (const HypernodeID& hn : flow_network.hypernodes()) {
          if (hg.partID(hn) == block_0) {
            weight_block_0 += hg.nodeWeight(hn);
          } else if (hg.partID(hn) == block_1) {
            weight_block_1 += hg.nodeWeight(hn);
          }
        }
        if (weight_block_0 > max_part_weight_0) {
          return false;
        }
        if (weight_block_1 > max_part_weight_1) {
          return false;
        }
        return true;
      } (), "Block size of one part is greater than the maximum allowed block size!");
  }
};
}  // namespace kahypar
