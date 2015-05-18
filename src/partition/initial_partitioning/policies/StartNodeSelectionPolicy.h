/*
 * StartNodeSelectionPolicy.h
 *
 *  Created on: Apr 18, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_STARTNODESELECTIONPOLICY_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_STARTNODESELECTIONPOLICY_H_

#include <vector>
#include <queue>
#include "lib/definitions.h"
#include "tools/RandomFunctions.h"
#include <algorithm>

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::Hypergraph;

namespace partition {
struct StartNodeSelectionPolicy {
	virtual ~StartNodeSelectionPolicy() {
	}

};

struct BFSStartNodeSelectionPolicy: public StartNodeSelectionPolicy {

  // TODO(heuer): this way, you might end up with the same start node multiple times
  // Wouldn't it be better to ensure that each node is contained at most once in startNodes?
  // Btw: naming convention: start_nodes ... not java-style startNodes...
  static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes,
                        const Hypergraph& hg, const PartitionID k) noexcept {
    // TODO(heuer): BUG: all start nodes are not marked as in_queue!
		if (k == 2) {
			startNodes.push_back(Randomize::getRandomInt(0, hg.numNodes() - 1));
			return;
		} else if (startNodes.size() == k) {
			return;
		} else if (startNodes.size() == 0) {
			startNodes.push_back(Randomize::getRandomInt(0, hg.numNodes() - 1));
			calculateStartNodes(startNodes, hg, k);
			return;
		}

		std::vector<bool> visited(hg.numNodes(), false);
		std::vector<bool> in_queue(hg.numNodes(), false);
		std::queue<HypernodeID> bfs;
                HypernodeID lastHypernode = -1;
                // TODO(heuer): code-style: code would look more pretty with range-based for
                // loops: for (const HypernodeID start_node : start_nodes) { ...}
		for (unsigned int i = 0; i < startNodes.size(); i++) {
			bfs.push(startNodes[i]);
                }

		while (!bfs.empty()) {
			HypernodeID hn = bfs.front();
			bfs.pop();
                        // TODO(heuer): codestyle!
			if (visited[hn])
				continue;
			lastHypernode = hn;
                        visited[hn] = true;
                        // TODO(heuer): make he and hnodes const if you don't need to modify it.
                        for (HyperedgeID he : hg.incidentEdges(lastHypernode)) {
                          //TODO(heuer): a more natural name would be pin instead of hnodes.
                                for (HypernodeID hnodes : hg.pins(he)) {
                                  // TODO(heuer): You miss all start nodes with the second check,
                                  // because they are never marked as in_queue!
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

struct RandomStartNodeSelectionPolicy: public StartNodeSelectionPolicy {
	static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes,
			const Hypergraph& hg, const PartitionID k) noexcept {
		if (k == 2) {
			startNodes.push_back(Randomize::getRandomInt(0, hg.numNodes() - 1));
			return;
		}

                for (PartitionID i = 0; i < k; i++) {
                  // TODO(heuer): uninitialized variable. Hn is also not needed outside of the while
                  // loop. Thus there is no need to declare it here...,
			HypernodeID hn;
			while (true) {
                                hn = Randomize::getRandomInt(0, hg.numNodes() - 1);
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
struct TestStartNodeSelectionPolicy: public StartNodeSelectionPolicy {
	static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes,
			const Hypergraph& hg, const PartitionID k) noexcept {
		if (k == 2) {
			startNodes.push_back(0);
			return;
		} else if (startNodes.size() == k) {
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

} // namespace partition

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_STARTNODESELECTIONPOLICY_H_ */
