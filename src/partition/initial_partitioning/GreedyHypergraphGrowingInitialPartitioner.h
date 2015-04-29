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
class GreedyHypergraphGrowingInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {

		/*max_net_size = 0.0;
		 for(HyperedgeID he : hypergraph.edges()) {
		 if(hypergraph.edgeSize(he) > max_net_size)
		 max_net_size = static_cast<double>(hypergraph.edgeSize(he));
		 }*/
	}

	~GreedyHypergraphGrowingInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		kWayPartitionImpl1();
	}

	void kWayPartitionImpl1() {
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

		//Define a weight bound, which every partition have to reach, to avoid very small partitions.
		double epsilon = _config.partition.epsilon;
		_config.partition.epsilon = -_config.partition.epsilon;
		InitialPartitionerBase::recalculateBalanceConstraints();
		bool release_upper_partition_bound = false;

		while (_hg.partWeight(unassigned_part)
				> _config.initial_partitioning.lower_allowed_partition_weight[unassigned_part]) {

			//Searching for the highest gain value
			Gain best_gain = initial_gain;
			PartitionID best_part = 0;
			HypernodeID best_node = 0;
			bool every_part_is_disable = true;
			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				every_part_is_disable = every_part_is_disable && !partEnable[i];
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
					ASSERT(!bq[i].empty(),
							"Bucket Queue of partition " << i << " should not be empty!");
					if (best_gain < bq[i].getMaxKey()) {
						best_gain = bq[i].getMaxKey();
						best_node = hn;
						best_part = i;
					}
				}
			}

			//Release upper partition weight bound
			if (every_part_is_disable && !release_upper_partition_bound) {
				_config.partition.epsilon = epsilon;
				InitialPartitionerBase::recalculateBalanceConstraints();
				release_upper_partition_bound = true;
				for (PartitionID i = 0; i < _config.initial_partitioning.k;
						i++) {
					if (i != unassigned_part) {
						partEnable[i] = true;
					}
				}
			} else {
				ASSERT(best_gain != initial_gain,
						"No hypernode found to assign!");
				if (!assignHypernodeToPartition(best_node, best_part)) {
					partEnable[best_part] = false;
				} else {
					ASSERT(!bq[best_part].empty(),
							"Bucket queue of partition " << best_part << " shouldn't be empty!");

					ASSERT(
							[&]() {
								_hg.changeNodePart(best_node,best_part,unassigned_part);
								HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
								_hg.changeNodePart(best_node,unassigned_part,best_part);
								return metrics::hyperedgeCut(_hg) == (cut_before-best_gain);
							}(), "Gain calculation failed!");

					bq[best_part].deleteMax();
				}
				//Pushing incident hypernode into bucket queue or update gain value
				for (HyperedgeID he : _hg.incidentEdges(best_node)) {
					for (HypernodeID hnode : _hg.pins(he)) {
						if (_hg.partID(hnode) == unassigned_part) {
							processNodeForBucketPQ(bq, hnode, best_part);
						}
					}
				}
			}

		}
		InitialPartitionerBase::recalculateBalanceConstraints();
		InitialPartitionerBase::performFMRefinement();
	}

	void kWayPartitionImpl2() {
		PartitionID unassigned_part = 0;
		for (HypernodeID hn : _hg.nodes()) {
			_hg.setNodePart(hn, unassigned_part);
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
		double delta_epsilon = epsilon/10.0;
		_config.partition.epsilon = 0.0;
		InitialPartitionerBase::recalculateBalanceConstraints();

		while (_hg.partWeight(unassigned_part)
				> _config.initial_partitioning.upper_allowed_partition_weight[unassigned_part]) {

			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				if (i != unassigned_part) {

					HypernodeID hn = bq[i].getMax();
					while (_hg.partID(hn) != unassigned_part && !bq[i].empty()) {
						bq[i].deleteMax();
						if (bq[i].empty()) {
							HypernodeID newStartNode =
									InitialPartitionerBase::getUnassignedNode(
											unassigned_part);
							processNodeForBucketPQ(bq[i], newStartNode, i);
						}
						hn = bq[i].getMax();
					}

					while (assignHypernodeToPartition(hn, i)) {
						bq[i].deleteMax();

						//Pushing incident hypernode into bucket queue or update gain value
						for (HyperedgeID he : _hg.incidentEdges(hn)) {
							for (HypernodeID hnode : _hg.pins(he)) {
								if (_hg.partID(hnode) == unassigned_part
										&& std::find(startNodes.begin(),
												startNodes.end(), hnode)
												== startNodes.end()) {
									processNodeForBucketPQ(bq[i], hnode, i);
								}
							}
						}

						if (bq[i].empty()) {
							HypernodeID newStartNode =
									InitialPartitionerBase::getUnassignedNode(
											unassigned_part);
							processNodeForBucketPQ(bq[i], newStartNode, i);
						}
						hn = bq[i].getMax();
					}
				}
			}

			//Increment upper partition weight bound
			_config.partition.epsilon += delta_epsilon;
			InitialPartitionerBase::recalculateBalanceConstraints();
		}

		_config.partition.epsilon = epsilon;
		InitialPartitionerBase::recalculateBalanceConstraints();

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

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGINITIALPARTITIONER_H_ */
