/*
 * GreedyHypergraphGrowingInitialPartitioner.h
 *
 *  Created on: Apr 18, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_

/*
 * BFSInitialPartitioning.h
 *
 *  Created on: Apr 12, 2015
 *      Author: theuer
 */

#include <queue>
#include <algorithm>

#include "lib/definitions.h"
#include "lib/datastructure/BucketQueue.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;

using Gain = HyperedgeWeight;

namespace partition {

class GreedyHypergraphGrowingInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~GreedyHypergraphGrowingInitialPartitioner() {
	}

private:

	void findStartNodes(std::vector<HypernodeID>& currentStartNodes,
			PartitionID& k) {
		if (k == 2) {
			currentStartNodes.push_back(
					Randomize::getRandomInt(0, _hg.numNodes() - 1));
			return;
		} else if (currentStartNodes.size() == k) {
			return;
		} else if (currentStartNodes.size() == 0) {
			currentStartNodes.push_back(
					Randomize::getRandomInt(0, _hg.numNodes() - 1));
			findStartNodes(currentStartNodes, k);
			return;
		}

		std::vector<bool> visited(_hg.numNodes(), false);
		std::queue<HypernodeID> bfs;
		HypernodeID lastHypernode = -1;
		for (unsigned int i = 0; i < currentStartNodes.size(); i++)
			bfs.push(currentStartNodes[i]);
		while (!bfs.empty()) {
			HypernodeID hn = bfs.front();
			bfs.pop();
			if (visited[hn])
				continue;
			lastHypernode = hn;
			visited[hn] = true;
			for (HyperedgeID he : _hg.incidentEdges(lastHypernode)) {
				for (HypernodeID hnodes : _hg.pins(he)) {
					if (!visited[hnodes])
						bfs.push(hnodes);
				}
			}
		}
		currentStartNodes.push_back(lastHypernode);
		findStartNodes(currentStartNodes, k);
	}

	void kwayPartitionImpl() final {

	}

	void bisectionPartitionImpl() final {
		BucketQueue<HypernodeID, Gain> bq(_hg.numNodes());

		std::vector<HypernodeID> startNode;
		PartitionID k = 2;
		findStartNodes(startNode, k);
		for (HypernodeID hn : _hg.nodes())
			_hg.setNodePart(hn, 1);
		processNodeForBucketPQ(bq, startNode[0], 0);
		while (true) {
			if (!bq.empty()) {
				HypernodeID hn = bq.getMax();
				Gain gain = bq.getMaxKey();
				bq.deleteMax();
				if (!assignHypernodeToPartition(hn, 0, 1, true))
					break;
				for (HyperedgeID he : _hg.incidentEdges(hn)) {
					for (HypernodeID hnode : _hg.pins(he))
						processNodeForBucketPQ(bq, hnode, 0);
				}
			}
			else {
				HypernodeID newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
				while(_hg.partID(newStartNode) != 1)
					newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
				processNodeForBucketPQ(bq,newStartNode,0);
			}
		}
		InitialPartitionerBase::rollbackToBestBisectionCut();
		InitialPartitionerBase::performFMRefinement();

	}

	Gain gainInducedByHypergraph(const HypernodeID hn,
			const PartitionID target_part) const noexcept {
		const PartitionID source_part = _hg.partID(hn);
		Gain gain = 0;
		for (const HyperedgeID he : _hg.incidentEdges(hn)) {
			ASSERT(_hg.edgeSize(he) > 1, "Computing gain for Single-Node HE");
			if (_hg.connectivity(he) == 1) {
				gain -= _hg.edgeWeight(he);
			} else {
				const HypernodeID pins_in_source_part = _hg.pinCountInPart(he,
						source_part);
				const HypernodeID pins_in_target_part = _hg.pinCountInPart(he,
						target_part);
				if (pins_in_source_part == 1
						&& pins_in_target_part == _hg.edgeSize(he) - 1) {
					gain += _hg.edgeWeight(he);
				}
			}
		}
		return gain;
	}

	void processNodeForBucketPQ(BucketQueue<HypernodeID, Gain>& bq,
			const HypernodeID hn, const PartitionID target_part) {
		if (bq.contains(hn)) {
			if (_hg.partID(hn) != target_part) {
				Gain gain = gainInducedByHypergraph(hn, target_part);
				bq.updateKey(hn, gain);
			}
		} else {
			if (_hg.partID(hn) != target_part) {
				Gain gain = gainInducedByHypergraph(hn, target_part);
				bq.push(hn, gain);
			}
		}
	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_ */
