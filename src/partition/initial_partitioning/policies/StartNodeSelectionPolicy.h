/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_STARTNODESELECTIONPOLICY_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_STARTNODESELECTIONPOLICY_H_

#include <algorithm>
#include <queue>
#include <vector>

#include "lib/datastructure/FastResetBitVector.h"
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::Hypergraph;

namespace partition {
struct StartNodeSelectionPolicy {
  virtual ~StartNodeSelectionPolicy() { }
};

struct BFSStartNodeSelectionPolicy : public StartNodeSelectionPolicy {
  static inline void calculateStartNodes(std::vector<HypernodeID>& start_nodes,
                                         const Hypergraph& hg, const PartitionID k) noexcept {
    start_nodes.push_back(Randomize::getRandomInt(0, hg.numNodes() - 1));
    FastResetBitVector<> in_queue(hg.numNodes(), false);
    FastResetBitVector<> hyperedge_in_queue(hg.numEdges(), false);

    while (start_nodes.size() != static_cast<size_t>(k)) {
      std::queue<HypernodeID> bfs;
      HypernodeID lastHypernode = -1;
      size_t visited_nodes = 0;
      for (const HypernodeID start_node : start_nodes) {
        bfs.push(start_node);
        in_queue.setBit(start_node, true);
      }


      while (!bfs.empty()) {
        const HypernodeID hn = bfs.front();
        bfs.pop();
        lastHypernode = hn;
        visited_nodes++;
        for (const HyperedgeID he : hg.incidentEdges(lastHypernode)) {
          if (!hyperedge_in_queue[he]) {
            for (const HypernodeID pin : hg.pins(he)) {
              if (!in_queue[pin]) {
                bfs.push(pin);
                in_queue.setBit(pin, true);
              }
            }
            hyperedge_in_queue.setBit(he, true);
          }
        }
        if (bfs.empty() && visited_nodes != hg.numNodes()) {
          for (const HypernodeID hn : hg.nodes()) {
            if (!in_queue[hn]) {
              bfs.push(hn);
              in_queue.setBit(hn, true);
            }
          }
        }
      }
      start_nodes.push_back(lastHypernode);
      in_queue.resetAllBitsToFalse();
      hyperedge_in_queue.resetAllBitsToFalse();
    }
  }
};

struct RandomStartNodeSelectionPolicy : public StartNodeSelectionPolicy {
  static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes,
                                         const Hypergraph& hg, const PartitionID k) noexcept {
    if (k == 2) {
      startNodes.push_back(Randomize::getRandomInt(0, hg.numNodes() - 1));
      return;
    }

    for (PartitionID i = 0; i < k; ++i) {
      while (true) {
        HypernodeID hn = Randomize::getRandomInt(0, hg.numNodes() - 1);
        // TODO(heuer): in this case you check for duplicate start nodes...
        // This check is missing in the BFS-Policy.
        auto node = std::find(startNodes.begin(), startNodes.end(), hn);
        if (node == startNodes.end()) {
          startNodes.push_back(hn);
          break;
        }
      }
    }
  }
};

// TODO(heuer): Unneccesary code-duplication. It would be better to further parametrize
// the first Policy such that it uses Random in one case, and in the test-case only returns
// 0 as start node.
// Additionally, this method has the same bug as described above.
struct TestStartNodeSelectionPolicy : public StartNodeSelectionPolicy {
  static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes,
                                         const Hypergraph& hg, const PartitionID k) noexcept {
    if (static_cast<int>(startNodes.size()) == k) {
      return;
    } else if (startNodes.size() == 0) {
      startNodes.push_back(0);
      calculateStartNodes(startNodes, hg, k);
      return;
    }

    std::vector<bool> visited(hg.numNodes(), false);
    std::vector<bool> in_queue(hg.numNodes(), false);
    std::queue<HypernodeID> bfs;
    HypernodeID lastHypernode = -1;
    for (unsigned int i = 0; i < startNodes.size(); i++) {
      bfs.push(startNodes[i]);
    }
    while (!bfs.empty()) {
      HypernodeID hn = bfs.front();
      bfs.pop();
      if (visited[hn])
        continue;
      lastHypernode = hn;
      visited[hn] = true;
      for (HyperedgeID he : hg.incidentEdges(lastHypernode)) {
        for (HypernodeID hnodes : hg.pins(he)) {
          if (!visited[hnodes] && !in_queue[hnodes]) {
            bfs.push(hnodes);
            in_queue[hnodes] = true;
          }
        }
      }
    }
    startNodes.push_back(lastHypernode);
    calculateStartNodes(startNodes, hg, k);
  }
};
}  // namespace partition

#endif  // SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_STARTNODESELECTIONPOLICY_H_
