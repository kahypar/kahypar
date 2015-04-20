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
					if (!visited[hnodes])
						bfs.push(hnodes);
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
				bool foundNode = false;
				for (int i = 0; i < startNodes.size(); i++) {
					if (startNodes[i] == hn) {
						foundNode = true;
						break;
					}
				}
				if (!foundNode) {
					startNodes.push_back(hn);
					break;
				}
			}
		}

	}
};

struct BFSSparseRegionStartNodeSelectionPolicy: public StartNodeSelectionPolicy {

	static const int neighboorhood_search_space = 4;

	static const int last_node_queue_size = 20;

	static inline void calculateStartNodes(std::vector<HypernodeID>& startNodes,
			const Hypergraph& hg, const PartitionID k) noexcept {
		if (k == 2) {
			std::vector<HypernodeID> nodes;
			for (HypernodeID hn : hg.nodes())
				nodes.push_back(hn);
			startNodes.push_back(getHypernodeIDWithSparsestRegion(hg, nodes));
			return;
		} else if (startNodes.size() == k) {
			for(int i = 0; i < k; i++)
				std::cout << startNodes[i] << " ";
			std::cout << std::endl;
			return;
		} else if (startNodes.size() == 0) {
			std::vector<HypernodeID> nodes;
			for (HypernodeID hn : hg.nodes())
				nodes.push_back(hn);
			startNodes.push_back(getHypernodeIDWithSparsestRegion(hg, nodes));
			calculateStartNodes(startNodes, hg, k);
			return;
		}

		std::vector<bool> visited(hg.numNodes(), false);
		std::queue<HypernodeID> bfs;
		std::queue<HypernodeID> next_q;
		std::queue<HypernodeID> last_node_queue;
		for (unsigned int i = 0; i < startNodes.size(); i++)
			bfs.push(startNodes[i]);
		while (true) {
			while (!bfs.empty()) {
				HypernodeID hn = bfs.front();
				bfs.pop();
				if (visited[hn])
					continue;
				visited[hn] = true;
				if(last_node_queue.size() >= last_node_queue_size)
					last_node_queue.pop();
				last_node_queue.push(hn);
				for (HyperedgeID he : hg.incidentEdges(hn)) {
					for (HypernodeID hnodes : hg.pins(he)) {
						if (!visited[hnodes])
							next_q.push(hnodes);
					}
				}
			}
			if(!next_q.empty()) {
				std::swap(bfs,next_q);
			}
			else
				break;
		}
		std::vector<HypernodeID> last_nodes;
		while(!last_node_queue.empty()) {
			last_nodes.push_back(last_node_queue.front());
			last_node_queue.pop();
		}
		HypernodeID sparsestHypernode = getHypernodeIDWithSparsestRegion(hg,
				last_nodes);
		startNodes.push_back(sparsestHypernode);
		calculateStartNodes(startNodes, hg, k);

	}

	static inline HypernodeID getHypernodeIDWithSparsestRegion(
			const Hypergraph& hg, std::vector<HypernodeID>& nodes) {
		std::priority_queue<std::pair<int, HypernodeID>,
				std::vector<std::pair<int, HypernodeID>>,
				std::greater<std::pair<int, HypernodeID>>> pq;
		for (HypernodeID hn : nodes) {
			pq.push(std::make_pair(getNeighbourhoodSize(hg, hn), hn));
		}
		return pq.top().second;
	}

	static inline int getNeighbourhoodSize(const Hypergraph& hg,
			const HypernodeID& hn) {
		std::queue<HypernodeID> q;
		std::queue<HypernodeID> next_q;
		std::vector<bool> visited(hg.numNodes(), false);
		q.push(hn);
		int size = 0;
		for (int i = 0; i < neighboorhood_search_space; i++) {
			while (!q.empty()) {
				HypernodeID hnode = q.front();
				q.pop();
				if (visited[hnode])
					continue;
				visited[hnode] = true;
				size++;
				for (HyperedgeID he : hg.incidentEdges(hnode)) {
					for (HypernodeID node : hg.pins(he)) {
						if (!visited[node])
							next_q.push(node);
					}
				}
			}
			if (next_q.empty())
				break;
			std::swap(q, next_q);
		}
		std::cout << size << std::endl;
		return size;
	}

};

} // namespace partition

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POLICIES_STARTNODESELECTIONPOLICY_H_ */
