/*

 * GreedyHypergraphGrowingInitialPartitioner.h
 *
 *  Created on: Apr 18, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_

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
using datastructure::BucketQueue;

using Gain = HyperedgeWeight;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config), _balancer(hypergraph,
					config) {

		/*max_net_size = 0.0;
		 for(HyperedgeID he : hypergraph.edges()) {
		 if(hypergraph.edgeSize(he) > max_net_size)
		 max_net_size = static_cast<double>(hypergraph.edgeSize(he));
		 }*/
	}

	~GreedyHypergraphGrowingInitialPartitioner() {
	}

private:
	FRIEND_TEST(AGreedyInitialPartionerTest, APushingNodesIntoBucketQueueInitialPartionerTest);

	void kwayPartitionImpl() final {
		PartitionID unassigned_part = 0;
		if (unassigned_part != -1) {
			for (HypernodeID hn : _hg.nodes()) {
				_hg.setNodePart(hn, unassigned_part);
			}
		}

		std::vector<BucketQueue<HypernodeID, Gain>> bq(
				_config.initial_partitioning.k,
				BucketQueue<HypernodeID, Gain>(2 * _hg.numNodes()));

		//Calculate Startnodes and push them into the queues.
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				_config.initial_partitioning.k);

		for (PartitionID i = 0; i < startNodes.size(); i++) {
			processNodeForBucketPQ(bq[i], startNodes[i], i);
		}

		//Define a weight bound, which every partition have to reach, to avoid very small partitions.
		double epsilon = _config.partition.epsilon;
		_config.partition.epsilon = 0.0;
		InitialPartitionerBase::recalculateBalanceConstraints();

		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if (i != unassigned_part) {

				processNodeForBucketPQ(bq[i], bq[i].getMax(), i);
				HypernodeID hn = bq[i].getMax();
				while (assignHypernodeToPartition(hn, i)) {

					ASSERT([&]() {
						Gain gain = bq[i].getMaxKey();
						_hg.changeNodePart(hn,i,unassigned_part);
						HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
						_hg.changeNodePart(hn,unassigned_part,i);
						return metrics::hyperedgeCut(_hg) == (cut_before-gain);
					}(), "Gain calculation of hypernode " << hn << " failed!");

					bq[i].deleteMax();

					//Pushing incident hypernode into bucket queue or update gain value
					for (HyperedgeID he : _hg.incidentEdges(hn)) {
						for (HypernodeID hnode : _hg.pins(he)) {
							if (_hg.partID(hnode) == unassigned_part) {
								auto startNode = std::find(startNodes.begin(),
										startNodes.end(), hnode);
								if (startNode == startNodes.end()) {
									processNodeForBucketPQ(bq[i], hnode, i);
								}
							}
						}
					}

					if (bq[i].empty()) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode(
										unassigned_part);
						processNodeForBucketPQ(bq[i], newStartNode, i);
					}

					ASSERT(!bq[i].empty(),
							"Bucket queue of partition " << i << " shouldn't be empty!");
					hn = bq[i].getMax();
				}
			}
		}

		_config.partition.epsilon = epsilon;
		InitialPartitionerBase::recalculateBalanceConstraints();

		_balancer.balancePartitions();

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
		HypernodeID hn = invalid_node;
		do {

			if (hn != invalid_node) {

				ASSERT(
						[&]() {
							Gain gain = bq.getMaxKey();
							_hg.changeNodePart(hn,0,1);
							HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
							_hg.changeNodePart(hn,1,0);
							return metrics::hyperedgeCut(_hg) == (cut_before-gain);
						}(), "Gain calculation failed!");


				bq.deleteMax();

				for (HyperedgeID he : _hg.incidentEdges(hn)) {
					for (HypernodeID hnode : _hg.pins(he)) {
						if (_hg.partID(hnode) == 1 && hnode != hn) {
							processNodeForBucketPQ(bq, hnode, 0);
						}
					}
				}
			}

			if (!bq.empty()) {
				hn = bq.getMax();
				while (_hg.partID(hn) != 1 && !bq.empty()) {
					hn = bq.getMax();
					bq.deleteMax();
				}
			}

			if (bq.empty() && _hg.partID(hn) != 1) {
				hn = InitialPartitionerBase::getUnassignedNode(1);
				processNodeForBucketPQ(bq, hn, 0);
			}

			ASSERT(_hg.partID(hn) == 1, "Hypernode " << hn << " should be from part 1!");

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

	void deleteAssignedNodeInBucketPQ(std::vector<BucketQueue<HypernodeID, Gain>>& bq,
			HypernodeID hn) {
		for (int i = 0; i < bq.size(); i++) {
			if(bq[i].contains(hn)) {
				bq[i].deleteNode(hn);
			}
		}
	}

	void updateAssignedNodeInBucketPQ(
			std::vector<BucketQueue<HypernodeID, Gain>>& bq,
			const HypernodeID hn) {
		for (int i = 0; i < bq.size(); i++) {
			if (bq[i].contains(hn)) {
				Gain gain = GainComputation::calculateGain(_hg, hn, i);
				bq[i].updateKey(hn, gain);
			}
		}
	}

	//double max_net_size;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;
	HypergraphPartitionBalancer _balancer;

	static const HypernodeID invalid_node =
			std::numeric_limits<HypernodeID>::max();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_ */
