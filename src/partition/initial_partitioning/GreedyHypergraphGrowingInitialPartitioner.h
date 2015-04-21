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
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;
using partition::StartNodeSelectionPolicy;
using partition::GainComputationPolicy;

using Gain = HyperedgeWeight;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
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

	void kwayPartitionImpl() final {

	}

	void bisectionPartitionImpl() final {
		BucketQueue<HypernodeID, Gain> bq(2 * _hg.numNodes());
		std::vector<HypernodeID> startNode;
		PartitionID k = 2;
		StartNodeSelection::calculateStartNodes(startNode, _hg, k);
		for (HypernodeID hn : _hg.nodes())
			_hg.setNodePart(hn, 1);
		processNodeForBucketPQ(bq, startNode[0], 0);
		while (true) {
			if (!bq.empty()) {
				HypernodeID hn = bq.getMax();
				bq.deleteMax();
				if (!assignHypernodeToPartition(hn, 0, 1, true))
					break;
				for (HyperedgeID he : _hg.incidentEdges(hn)) {
					for (HypernodeID hnode : _hg.pins(he))
						processNodeForBucketPQ(bq, hnode, 0);
				}
			} else {
				HypernodeID newStartNode = Randomize::getRandomInt(0,
						_hg.numNodes() - 1);
				while (_hg.partID(newStartNode) != 1)
					newStartNode = Randomize::getRandomInt(0,
							_hg.numNodes() - 1);
				processNodeForBucketPQ(bq, newStartNode, 0);
			}
		}
		InitialPartitionerBase::rollbackToBestBisectionCut();
		InitialPartitionerBase::performFMRefinement();

	}

	void processNodeForBucketPQ(BucketQueue<HypernodeID, Gain>& bq,
			const HypernodeID hn, const PartitionID target_part) {
		if (_hg.partID(hn) != target_part) {
			if (bq.contains(hn)) {
				Gain gain = GainComputation::calculateGain(_hg, hn,
						target_part);
				bq.updateKey(hn, gain);
			} else {
				Gain gain = GainComputation::calculateGain(_hg, hn,
						target_part);
				bq.push(hn, gain);
			}
		}
	}


	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_ */
