/*
 * GreedyHypergraphGrowingRoundRobinInitialPartitioner.h
 *
 *  Created on: 30.04.2015
 *      Author: heuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGROUNDROBININITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGROUNDROBININITIALPARTITIONER_H_


#include <queue>
#include <limits>
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
class GreedyHypergraphGrowingRoundRobinInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingRoundRobinInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {

		/*max_net_size = 0.0;
		 for(HyperedgeID he : hypergraph.edges()) {
		 if(hypergraph.edgeSize(he) > max_net_size)
		 max_net_size = static_cast<double>(hypergraph.edgeSize(he));
		 }*/
	}

	~GreedyHypergraphGrowingRoundRobinInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		PartitionID unassigned_part = 0;
		for (HypernodeID hn : _hg.nodes()) {
			_hg.setNodePart(hn, unassigned_part);
		}

		std::vector<BucketQueue<HypernodeID, Gain>> bq(
				_config.initial_partitioning.k,
				BucketQueue<HypernodeID, Gain>(2 * _hg.numNodes()));

		//Enable parts are allowed to receive further hypernodes.
		std::vector<bool> partEnable(_config.initial_partitioning.k, true);
		partEnable[unassigned_part] = false;

		//Calculate Startnodes and push them into the queues.
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				_config.initial_partitioning.k);
		for (PartitionID i = 0; i < startNodes.size(); i++) {
			processNodeForBucketPQ(bq[i], startNodes[i], i);
		}

		int count = 0;
		while (_hg.partWeight(unassigned_part)
				> _config.initial_partitioning.lower_allowed_partition_weight[unassigned_part]) {

			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				if (partEnable[i]) {
					HypernodeID hn;
					if (bq[i].empty()) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode(
										unassigned_part);
						processNodeForBucketPQ(bq[i], newStartNode, i);
					}
					hn = bq[i].getMax();

					//If the current maximum gain hypernode isn't an unassigned hypernode, we search until we found one.
					while (_hg.partID(hn) != unassigned_part && !bq[i].empty()) {
						count++;
						bq[i].deleteMax();
						if (bq[i].empty()) {
							HypernodeID newStartNode =
									InitialPartitionerBase::getUnassignedNode(
											unassigned_part);
							processNodeForBucketPQ(bq[i], newStartNode, i);
						}
						hn = bq[i].getMax();
					}
					ASSERT(_hg.partID(hn) == unassigned_part,
							"Hypernode " << hn << "should be unassigned!");

					if(!assignHypernodeToPartition(hn,i)) {
						partEnable[i] = false;
					} else {

						ASSERT(!bq[i].empty(),
								"Bucket queue of partition " << i << " shouldn't be empty!");

						ASSERT(
								[&]() {
									Gain gain = bq[i].getMaxKey();
									_hg.changeNodePart(hn,i,unassigned_part);
									HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
									_hg.changeNodePart(hn,unassigned_part,i);
									return metrics::hyperedgeCut(_hg) == (cut_before-gain);
								}(), "Gain calculation failed!");

						for (HyperedgeID he : _hg.incidentEdges(hn)) {
							for (HypernodeID hnode : _hg.pins(he)) {
								if (_hg.partID(hnode) == unassigned_part) {
									processNodeForBucketPQ(bq, hnode, i);
								}
							}
						}
						bq[i].deleteMax();

					}

				}
			}
		}
		std::cout << count << std::endl;
		InitialPartitionerBase::recalculateBalanceConstraints();
		InitialPartitionerBase::performFMRefinement();
	}


	void bisectionPartitionImpl() final {
		BucketQueue<HypernodeID, Gain> bq(2 * _hg.numNodes());
		std::vector<HypernodeID> startNode;
		StartNodeSelection::calculateStartNodes(startNode, _hg,
				static_cast<PartitionID>(2));
		for (HypernodeID hn : _hg.nodes()) {
			_hg.setNodePart(hn, 1);
		}
		processNodeForBucketPQ(bq, startNode[0], 0);
		HypernodeID hn;
		do {
			if (!bq.empty()) {
				hn = bq.getMax();
				bq.deleteMax();
				while (_hg.partID(hn) != 1 && !bq.empty()) {
					hn = bq.getMax();
					bq.deleteMax();
				}
			}

			if (bq.empty() && _hg.partID(hn) != 1) {
				hn = InitialPartitionerBase::getUnassignedNode(1);
			}

			for (HyperedgeID he : _hg.incidentEdges(hn)) {
				for (HypernodeID hnode : _hg.pins(he)) {
					processNodeForBucketPQ(bq, hnode, 0);
				}
			}

		} while (assignHypernodeToPartition(hn, 0, 1, true));

		InitialPartitionerBase::rollbackToBestBisectionCut();
		InitialPartitionerBase::performFMRefinement();

	}

	void processNodeForBucketPQ(BucketQueue<HypernodeID, Gain>& bq,
			const HypernodeID hn, const PartitionID target_part) {
		if (_hg.partID(hn) != target_part) {
			Gain gain = GainComputation::calculateGain(_hg, hn, target_part);
			if (bq.contains(hn)) {
				bq.updateKey(hn, gain);
			} else {
				bq.push(hn, gain);
			}
		}
	}

	void processNodeForBucketPQ(std::vector<BucketQueue<HypernodeID, Gain>>& bq,
			const HypernodeID hn, const PartitionID target_part) {
		for (int i = 0; i < bq.size(); i++) {
			if (_hg.partID(hn) != target_part) {
				if (bq[i].contains(hn)) {
					Gain gain = GainComputation::calculateGain(_hg, hn, i);
					bq[i].updateKey(hn, gain);
				} else if (i == target_part) {
					Gain gain = GainComputation::calculateGain(_hg, hn,
							target_part);
					bq[target_part].push(hn, gain);
				}
			}
		}
	}

	//double max_net_size;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	static const Gain initial_gain = std::numeric_limits<Gain>::min();

};

}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGROUNDROBININITIALPARTITIONER_H_ */
