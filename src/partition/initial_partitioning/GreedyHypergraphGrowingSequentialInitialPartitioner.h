/*
 * GreedyHypergraphGrowingSequentialInitialPartitioner.h
 *
 *  Created on: May 6, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGSEQUENTIALINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGSEQUENTIALINITIALPARTITIONER_H_

#include <queue>
#include <limits>
#include <algorithm>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingBaseFunctions.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "lib/datastructure/FastResetBitVector.h"
#include "tools/RandomFunctions.h"

using datastructure::NoDataBinaryMaxHeap;
using defs::HypernodeWeight;
using partition::StartNodeSelectionPolicy;
using partition::GainComputationPolicy;

using Gain = HyperedgeWeight;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingSequentialInitialPartitioner: public IInitialPartitioner,
		public InitialPartitionerBase {

public:
	GreedyHypergraphGrowingSequentialInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config), greedy_base(hypergraph,
					config) {

		/*max_net_size = 0.0;
		 for(HyperedgeID he : hypergraph.edges()) {
		 if(hypergraph.edgeSize(he) > max_net_size)
		 max_net_size = static_cast<double>(hypergraph.edgeSize(he));
		 }*/
	}

	~GreedyHypergraphGrowingSequentialInitialPartitioner() {
	}

private:
	FRIEND_TEST(AGreedyInitialPartionerTest, ExpectedMaxGainValueAndHypernodeAfterPushingSomeHypernodesIntoBucketQueue);FRIEND_TEST(AGreedyInitialPartionerTest, ExpectedMaxGainValueAfterUpdateAHypernodeIntoBucketQueue);

	void initialPartition() final {

		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		InitialPartitionerBase::resetPartitioning();
		greedy_base.reset();

		//Calculate Startnodes and push them into the queues.
		greedy_base.calculateStartNodes();

		// TODO(heuer): This does not seem to be fair. In other variants you
		// (1) have a less tight bound, (2) you unlock the upper bound later on...
		// Why don't you also perform an unlock here?
		//Define a weight bound, which every partition have to reach, to avoid very small partitions.
		InitialPartitionerBase::recalculateBalanceConstraints(0.0);

		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if (i != unassigned_part) {

				HypernodeID hn = kInvalidNode;

				do {
					ASSERT([&]() {
						if(hn != kInvalidNode) {
							if(_hg.partID(hn) != i) {
								return false;
							}
						}
						return true;
					}(),
							"Assignment of hypernode " << hn << " to partition " << i << " failed!");
					ASSERT(
							[&]() {
								if(_config.initial_partitioning.unassigned_part != -1 && hn != kInvalidNode && GainComputation::getType() == GainType::fm_gain) {
									Gain gain = greedy_base.maxKeyFromPartition(i);
									_hg.changeNodePart(hn,i,unassigned_part);
									HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
									_hg.changeNodePart(hn,unassigned_part,i);
									return metrics::hyperedgeCut(_hg) == (cut_before-gain);
								}
								else {
									return true;
								}}(),
							"Gain calculation of hypernode " << hn << " failed!");

					if (hn != kInvalidNode) {
						greedy_base.insertAndUpdateNodesAfterMove(hn, i);
					}

					if (greedy_base.empty(i)) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode();
						if (newStartNode == invalid_node) {
							break;
						}
						greedy_base.insertNodeIntoPQ(newStartNode, i);
					}

					ASSERT(!greedy_base.empty(i),
							"Bucket queue of partition " << i << " shouldn't be empty!");
					hn = greedy_base.maxFromPartition(i);
				} while (InitialPartitionerBase::assignHypernodeToPartition(hn,
						i));
			}
		}

		InitialPartitionerBase::recalculateBalanceConstraints(
				_config.initial_partitioning.epsilon);
		assignAllUnassignedHypernodesAccordingToTheirGain();

		if (unassigned_part == -1) {
			_hg.initializeNumCutHyperedges();
		}

		ASSERT([&]() {
			for(HypernodeID hn : _hg.nodes()) {
				if(_hg.partID(hn) == -1) {
					return false;
				}
			}
			return true;
		}(), "There are unassigned hypernodes!");

		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();
	}


	void assignAllUnassignedHypernodesAccordingToTheirGain() {
		//Assign first all possible unassigned nodes, which are still left in the priority queue.
		while (true) {
			HypernodeID best_node = kInvalidNode;
			PartitionID best_part = kInvalidPartition;
			Gain best_gain = kInitialGain;
			greedy_base.getMaxGainHypernode(best_node, best_part, best_gain);
			if (best_part != kInvalidPartition) {
				if (InitialPartitionerBase::assignHypernodeToPartition(
						best_node, best_part)) {
					greedy_base.insertAndUpdateNodesAfterMove(best_node,
							best_part, false);
					if (_config.initial_partitioning.unassigned_part != -1) {
						if (_hg.partWeight(
								_config.initial_partitioning.unassigned_part)
								< _config.initial_partitioning.upper_allowed_partition_weight[_config.initial_partitioning.unassigned_part]) {
							break;
						}
					}
				}
			} else {
				break;
			}
		}

		greedy_base.clearAllPQs();

		//Visit all unassigned nodes in a random order and assign them to best partition according to their gain
		std::vector<HypernodeID> unassigned_nodes;
		HypernodeWeight part_weight = 0;
		HypernodeWeight unassigned_part_weight_bound =
				(_config.initial_partitioning.unassigned_part == -1) ?
						0 :
						_config.initial_partitioning.upper_allowed_partition_weight[_config.initial_partitioning.unassigned_part];
		for (HypernodeID hn : _hg.nodes()) {
			if (_hg.partID(hn)
					== _config.initial_partitioning.unassigned_part) {
				unassigned_nodes.push_back(hn);
				part_weight += _hg.nodeWeight(hn);
			}
		}
		Randomize::shuffleVector(unassigned_nodes, unassigned_nodes.size());
		while (part_weight > unassigned_part_weight_bound
				&& !unassigned_nodes.empty()) {
			HypernodeID hn = unassigned_nodes.back();
			unassigned_nodes.pop_back();
			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				if (i != _config.initial_partitioning.unassigned_part) {
					greedy_base.insertNodeIntoPQ(hn, i);
				}
			}
			HypernodeID best_node = kInvalidNode;
			PartitionID best_part = kInvalidPartition;
			Gain best_gain = kInitialGain;
			greedy_base.getMaxGainHypernode(best_node, best_part, best_gain);
			if (best_part != kInvalidPartition) {
				if (assignHypernodeToPartition(best_node, best_part)) {
					part_weight -= _hg.nodeWeight(best_node);
				} else {
					if (_config.initial_partitioning.unassigned_part == -1) {
						_hg.setNodePart(best_node,
								Randomize::getRandomInt(0,
										_config.initial_partitioning.k - 1));
						part_weight -= _hg.nodeWeight(best_node);
					}
				}
			}
			greedy_base.deleteNodeInAllBucketQueues(best_node);
		}
	}

	//double max_net_size;
protected:
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	GreedyHypergraphGrowingBaseFunctions<StartNodeSelection, GainComputation> greedy_base;

	static const HypernodeID invalid_node =
			std::numeric_limits<HypernodeID>::max();
	static const Gain kInitialGain = std::numeric_limits<Gain>::min();
	static const PartitionID kInvalidPartition = -1;
	static const HypernodeID kInvalidNode =
			std::numeric_limits<HypernodeID>::max();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGSEQUENTIALINITIALPARTITIONER_H_ */
