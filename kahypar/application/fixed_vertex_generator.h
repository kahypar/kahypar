/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@gmx.net>
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
#include <queue>
#include <vector>
#include <utility>

#include "kahypar/definitions.h"
#include "kahypar/utils/randomize.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/partition/initial_partitioning/policies/ip_start_node_selection_policy.h"
#include "kahypar/kahypar.h"

namespace kahypar {

static inline void randomFixedVertexGenerator(Hypergraph& hypergraph, const Context& context) {
  for (const HypernodeID& hn : hypergraph.nodes()) {
    int r = Randomize::instance().getRandomInt(1, 100000);
    if (r <= context.partition.fixed_vertex_fraction * 100000.0) {
      kahypar::PartitionID fixedPart = hypergraph.partID(hn);
      if (fixedPart == -1) {
        fixedPart =  Randomize::instance().getRandomInt(0, context.partition.k - 1);
      }
      hypergraph.setFixedVertex(hn, fixedPart);
    }
  }
}

static inline void bubbleFixedVertexGenerator(Hypergraph& hypergraph, const Context& context) {
  using QueueElement = std::pair<HypernodeID, PartitionID>;
  PartitionID k = context.partition.k;
  std::vector<std::vector<HypernodeID>> start_nodes(k, std::vector<HypernodeID>());
  BFSStartNodeSelectionPolicy<>::calculateStartNodes(start_nodes, context, hypergraph, k);

  std::queue<QueueElement> q;
  HypernodeID num_nodes = hypergraph.initialNumNodes();
  HyperedgeID num_edges = hypergraph.initialNumEdges();
  std::vector<bool> hypernode_in_queue(k * num_nodes, false);
  std::vector<bool> hyperedge_in_queue(k * num_edges, false);
  for (PartitionID i = 0; i < k; ++i) {
    q.push(std::make_pair(start_nodes[i][0], i));
    hypernode_in_queue[i * num_nodes + start_nodes[i][0]] = true;
  }

  std::vector<HypernodeID> unassigned_hypernodes(num_nodes, 0);
  size_t unassigned_index = num_nodes;
  std::iota(unassigned_hypernodes.begin(), unassigned_hypernodes.end(), 0);

  auto push_incident_nodes_into_pq = [&](const HypernodeID hn, const PartitionID part) {
    for (const HyperedgeID& he : hypergraph.incidentEdges(hn)) {
      if (!hyperedge_in_queue[part * num_edges + he]) {
        for (const HypernodeID& pin : hypergraph.pins(he)) {
          if (!hypernode_in_queue[part * num_nodes + pin]) {
            q.push(std::make_pair(pin, part));
            hypernode_in_queue[part * num_nodes + pin] = true;
          }
        }
        hyperedge_in_queue[part * num_edges + he] = true;
      }
    }
  };

  auto get_unassigned_hypernode = [&]() {
    HypernodeID hn = unassigned_hypernodes[0];
    while (unassigned_index > 0 && hypergraph.partID(hn) != -1) {
      std::swap(unassigned_hypernodes[0], unassigned_hypernodes[--unassigned_index]);
      hn = unassigned_hypernodes[0];
    }
    if (unassigned_index == 0) {
      hn = num_nodes;
    }
    return hn;
  };

  HypernodeWeight max_part_weight = context.partition.fixed_vertex_fraction *
                                    (hypergraph.totalWeight() / context.partition.k);
  while (!q.empty()) {
    HypernodeID hn = q.front().first;
    PartitionID part = q.front().second;
    q.pop();

    if (hypergraph.partID(hn) == -1) {
      if (hypergraph.partWeight(part) + hypergraph.nodeWeight(hn) <= max_part_weight) {
        hypergraph.setNodePart(hn, part);
        push_incident_nodes_into_pq(hn, part);
      }
    }

    if (q.empty()) {
      for (PartitionID i = 0; i < k; ++i) {
        if (hypergraph.partWeight(i) < max_part_weight) {
          HypernodeID hn = get_unassigned_hypernode();
          if (hn != num_nodes) {
            q.push(std::make_pair(hn, i));
          }
        }
      }
    }
  }

  for (const HypernodeID& hn : hypergraph.nodes()) {
    if (hypergraph.partID(hn) != -1) {
      hypergraph.setFixedVertex(hn, hypergraph.partID(hn));
    }
  }
  hypergraph.resetPartitioning();
}

static inline void repartitioningFixedVertexGenerator(Hypergraph& hypergraph, const Context& context) {
  Partitioner partitioner;
  Context current_context(context);
  current_context.partition.quiet_mode = true;
  current_context.partition.verbose_output = false;
  current_context.partition.sp_process_output = false;
  current_context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;
  current_context.initial_partitioning.mode = Mode::recursive_bisection;
  current_context.initial_partitioning.technique = InitialPartitioningTechnique::multilevel;
  current_context.initial_partitioning.local_search.algorithm = RefinementAlgorithm::twoway_fm;

  // Compute k-way partition for fixed vertex generation
  partitioner.partition(hypergraph, current_context);

  // Select random fixed vertices with part ids from
  // previously computed partition
  randomFixedVertexGenerator(hypergraph, context);
  hypergraph.resetPartitioning();
}

}  // namespace kahypar
