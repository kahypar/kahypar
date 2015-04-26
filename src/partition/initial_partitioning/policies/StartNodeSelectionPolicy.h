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
	static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes,
			const Hypergraph& hg, const PartitionID k) noexcept {
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
		for (unsigned int i = 0; i < startNodes.size(); i++)
			bfs.push(startNodes[i]);
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

struct RandomStartNodeSelectionPolicy: public StartNodeSelectionPolicy {
	static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes,
			const Hypergraph& hg, const PartitionID k) noexcept {
		if (k == 2) {
			startNodes.push_back(Randomize::getRandomInt(0, hg.numNodes() - 1));
			return;
		}

		for (PartitionID i = 0; i < k; i++) {
			HypernodeID hn;
			while (true) {
				hn = Randomize::getRandomInt(0, hg.numNodes() - 1);
				auto node = std::find(startNodes.begin(),startNodes.end(),hn);
				if(node == startNodes.end()) {
					startNodes.push_back(hn);
					break;
				}
			}
		}
	}


};



} // namespace partition

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_STARTNODESELECTIONPOLICY_H_ */
