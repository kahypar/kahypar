/*
 * GreedyHypergraphGrowingGlobalInitialPartitioner.h
 *
 *  Created on: 30.04.2015
 *      Author: heuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGGLOBALINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGGLOBALINITIALPARTITIONER_H_

#include <queue>
#include <limits>
#include <algorithm>

#include "lib/definitions.h"
#include "lib/datastructure/BucketQueue.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingBaseFunctions.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "tools/RandomFunctions.h"
#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"

using defs::HypernodeWeight;
using partition::StartNodeSelectionPolicy;
using partition::GainComputationPolicy;
using datastructure::BucketQueue;
using datastructure::PriorityQueue;
using external::NoDataBinaryMaxHeap;

using Gain = HyperedgeWeight;

using TwoWayFMHeap = NoDataBinaryMaxHeap<HypernodeID, Gain,
std::numeric_limits<HyperedgeWeight> >;
using PrioQueue = PriorityQueue<TwoWayFMHeap>;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingGlobalInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingGlobalInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config), greedy_base(hypergraph,
					config) {

	}

	~GreedyHypergraphGrowingGlobalInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		InitialPartitionerBase::resetPartitioning(unassigned_part);
		Gain init_pq = _hg.numNodes();
		std::vector<PrioQueue*> bq(_config.initial_partitioning.k);
		for (PartitionID k = 0; k < _config.initial_partitioning.k; k++) {
			bq[k] = new PrioQueue(init_pq);
		}

		//Enable parts are allowed to receive further hypernodes.
		std::vector<bool> partEnable(_config.initial_partitioning.k, true);
		if (unassigned_part != -1) {
			partEnable[unassigned_part] = false;
		}

		//Calculate Startnodes and push them into the queues.
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				_config.initial_partitioning.k);
		for (PartitionID i = 0; i < startNodes.size(); ++i) {
			greedy_base.processNodeForBucketPQ(*bq[i], startNodes[i], i);
		}

		HypernodeID assigned_nodes_weight = 0;
		if (unassigned_part != -1) {
			assigned_nodes_weight =
					_config.initial_partitioning.perfect_balance_partition_weight[unassigned_part]
							* (1.0 - _config.initial_partitioning.epsilon);
		}

		//Define a weight bound, which every partition have to reach, to avoid very small partitions.
		InitialPartitionerBase::recalculateBalanceConstraints(
				-_config.initial_partitioning.epsilon);
		bool is_upper_bound_released = false;

		std::vector<PartitionID> part_shuffle(_config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; ++i) {
			part_shuffle[i] = i;
		}


		// TODO(heuer): Why do you use assigned_nodes_weight instead of counting
		// the number of assigned hypernodes?
		while (assigned_nodes_weight < _config.partition.total_graph_weight) {
			std::random_shuffle(part_shuffle.begin(), part_shuffle.end());
			//Searching for the highest gain value
			Gain best_gain = kInitialGain;
			PartitionID best_part = kInvalidPartition;
			HypernodeID best_node = kInvalidNode;
			bool is_every_part_disable = true;
			for (PartitionID i = 0; i < _config.initial_partitioning.k; ++i) {
				const PartitionID current_part = part_shuffle[i];
				is_every_part_disable = is_every_part_disable
						&& !partEnable[current_part];
				if (partEnable[current_part]) {
					HypernodeID hn;
					if (bq[current_part]->empty()) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode(
										unassigned_part);
						if(newStartNode == kInvalidNode) {
							continue;
						}
						greedy_base.processNodeForBucketPQ(*bq[current_part],
								newStartNode, current_part);
					}
					hn = bq[current_part]->max();

					ASSERT(_hg.partID(hn) == unassigned_part,
							"Hypernode " << hn << "should be unassigned!");
					ASSERT(!bq[part_shuffle[i]]->empty(),
							"Bucket Queue of partition " << i << " should not be empty!");
					if (best_gain < bq[part_shuffle[i]]->maxKey()) {
						best_gain = bq[part_shuffle[i]]->maxKey();
						best_node = hn;
						best_part = part_shuffle[i];
					}
				}
			}
			//Release upper partition weight bound
			if (is_every_part_disable && !is_upper_bound_released) {
				InitialPartitionerBase::recalculateBalanceConstraints(
						_config.initial_partitioning.epsilon);
				is_upper_bound_released = true;
				for (PartitionID i = 0; i < _config.initial_partitioning.k;
						i++) {
					if (i != unassigned_part) {
						partEnable[i] = true;
					}
				}
				is_every_part_disable = false;
			} else if (best_part != kInvalidPartition) {
				ASSERT(best_gain != kInitialGain,
						"No hypernode found to assign!");

				if (!assignHypernodeToPartition(best_node, best_part)) {
					partEnable[best_part] = false;
				} else {
					ASSERT(!bq[best_part]->empty(),
							"Bucket queue of partition " << best_part << " shouldn't be empty!");

					ASSERT([&]() {
						Gain gain = bq[best_part]->maxKey();
						_hg.changeNodePart(best_node,best_part,unassigned_part);
						HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
						_hg.changeNodePart(best_node,unassigned_part,best_part);
						return metrics::hyperedgeCut(_hg) == (cut_before-gain);
					}(), "Gain calculation failed!");

					greedy_base.deleteNodeInAllBucketQueues(bq, best_node);

					GainComputation::deltaGainUpdate(_hg, bq, best_node,
							unassigned_part, best_part);
					//Pushing incident hypernode into bucket queue or update gain value
					for (HyperedgeID he : _hg.incidentEdges(best_node)) {
						for (HypernodeID hnode : _hg.pins(he)) {
							if (_hg.partID(hnode) == unassigned_part) {
								greedy_base.processNodeForBucketPQ(
										*bq[best_part], hnode, best_part);
							}
						}
					}

					// TODO(heuer): Either a big assertion or a corresponding test case is missing
					// that verifies for all neighbors of best_node that they are contained in the
					// pqs with the correct gain values.

					assigned_nodes_weight += _hg.nodeWeight(best_node);
				}
			}

			// TODO(heuer): Is this break really necessary? Loop should exit as soon as all
			// nodes are assigned?
			if (is_every_part_disable && is_upper_bound_released) {
				break;
			}
		}


		for (PartitionID k = 0; k < _config.initial_partitioning.k; k++) {
			delete bq[k];
		}

		InitialPartitionerBase::recalculateBalanceConstraints(_config.initial_partitioning.epsilon);
		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::eraseConnectedComponents();
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		PartitionID k = _config.initial_partitioning.k;
		_config.initial_partitioning.k = 2;
		kwayPartitionImpl();
		_config.initial_partitioning.k = k;
	}

	//double max_net_size;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	GreedyHypergraphGrowingBaseFunctions<GainComputation> greedy_base;

	static const Gain kInitialGain = std::numeric_limits<Gain>::min();
	static const PartitionID kInvalidPartition = std::numeric_limits<PartitionID>::max();
	static const HypernodeID kInvalidNode =
			std::numeric_limits<HypernodeID>::max();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGGLOBALINITIALPARTITIONER_H_ */
