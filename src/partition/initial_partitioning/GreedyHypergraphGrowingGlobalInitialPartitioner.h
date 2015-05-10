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
			InitialPartitionerBase(hypergraph, config) {

		/*max_net_size = 0.0;
		 for(HyperedgeID he : hypergraph.edges()) {
		 if(hypergraph.edgeSize(he) > max_net_size)
		 max_net_size = static_cast<double>(hypergraph.edgeSize(he));
		 }*/
	}

	~GreedyHypergraphGrowingGlobalInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		PartitionID unassigned_part = 0;
		if (unassigned_part != -1) {
			for (HypernodeID hn : _hg.nodes()) {
				_hg.setNodePart(hn, unassigned_part);
			}
		}

		Gain init_pq = 2 * _hg.numNodes();
		std::vector<PrioQueue*> bq(_config.initial_partitioning.k);
		for(PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			bq[i] = new PrioQueue(init_pq);
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
		for (PartitionID i = 0; i < startNodes.size(); i++) {
			processNodeForBucketPQ(*bq[i], startNodes[i], i);
		}

		HypernodeID assigned_nodes_weight = 0;
		if (unassigned_part != -1) {
			assigned_nodes_weight =
					_config.initial_partitioning.lower_allowed_partition_weight[unassigned_part];
		}

		//Define a weight bound, which every partition have to reach, to avoid very small partitions.
		double epsilon = _config.partition.epsilon;
		_config.partition.epsilon = -_config.partition.epsilon;
		InitialPartitionerBase::recalculateBalanceConstraints();
		bool release_upper_partition_bound = false;

		std::vector<PartitionID> part_shuffle(_config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			part_shuffle[i] = i;
		}


		while (assigned_nodes_weight < _config.partition.total_graph_weight) {
			std::random_shuffle(part_shuffle.begin(), part_shuffle.end());
			//Searching for the highest gain value
			Gain best_gain = initial_gain;
			PartitionID best_part = unassigned_part;
			HypernodeID best_node = 0;
			bool every_part_is_disable = true;
			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				every_part_is_disable = every_part_is_disable
						&& !partEnable[part_shuffle[i]];
				if (partEnable[part_shuffle[i]]) {
					HypernodeID hn;
					if (bq[part_shuffle[i]]->empty()) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode(
										unassigned_part);
						processNodeForBucketPQ(*bq[part_shuffle[i]],
								newStartNode, part_shuffle[i]);
					}
					hn = bq[part_shuffle[i]]->max();


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
				every_part_is_disable = false;
			} else {
				ASSERT(best_gain != initial_gain,
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

					bq[best_part]->deleteMax();
					deleteAssignedNodeInBucketPQ(bq,best_node);

					deltaGainUpdate(bq,best_node,unassigned_part,best_part);
					//Pushing incident hypernode into bucket queue or update gain value
					for (HyperedgeID he : _hg.incidentEdges(best_node)) {
						for (HypernodeID hnode : _hg.pins(he)) {
							if (_hg.partID(hnode) == unassigned_part) {
								processNodeForBucketPQ(*bq[best_part], hnode, best_part);
							}
						}
					}

					assigned_nodes_weight += _hg.nodeWeight(best_node);

				}
			}

			if(every_part_is_disable  && release_upper_partition_bound) {
				break;
			}
		}
		InitialPartitionerBase::recalculateBalanceConstraints();
		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::eraseConnectedComponents();
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		HyperedgeWeight init_pq = 2 * _hg.numNodes();
		PrioQueue* bq = new PrioQueue(init_pq);
		std::vector<HypernodeID> startNode;
		StartNodeSelection::calculateStartNodes(startNode, _hg,
				static_cast<PartitionID>(2));
		for (HypernodeID hn : _hg.nodes()) {
			_hg.setNodePart(hn, 1);
		}
		processNodeForBucketPQ(*bq, startNode[0], 0);
		HypernodeID hn = invalid_node;
		do {

			if (hn != invalid_node) {

				ASSERT([&]() {
					Gain gain = bq->maxKey();
					_hg.changeNodePart(hn,0,1);
					HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
					_hg.changeNodePart(hn,1,0);
					return metrics::hyperedgeCut(_hg) == (cut_before-gain);
				}(), "Gain calculation failed!");

				bq->deleteMax();

				for (HyperedgeID he : _hg.incidentEdges(hn)) {
					for (HypernodeID hnode : _hg.pins(he)) {
						if (_hg.partID(hnode) == 1 && hnode != hn) {
							processNodeForBucketPQ(*bq, hnode, 0, true);
						}
					}
				}
			}

			if (!bq->empty()) {
				hn = bq->max();
				while (_hg.partID(hn) != 1 && !bq->empty()) {
					hn = bq->max();
					bq->deleteMax();
				}
			}

			if (bq->empty() && _hg.partID(hn) != 1) {
				hn = InitialPartitionerBase::getUnassignedNode(1);
				processNodeForBucketPQ(*bq, hn, 0);
			}

			ASSERT(_hg.partID(hn) == 1,
					"Hypernode " << hn << " should be from part 1!");

		} while (assignHypernodeToPartition(hn, 0));

		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();

	}

	void deltaGainUpdate(std::vector<PrioQueue*>& bq, HypernodeID hn,
			PartitionID from, PartitionID to) {
		for (HyperedgeID he : _hg.incidentEdges(hn)) {

			HypernodeID pin_count_in_source_part_before = _hg.pinCountInPart(he,
					from) + 1;
			HypernodeID pin_count_in_target_part_after = _hg.pinCountInPart(he,
					to);
			PartitionID connectivity = _hg.connectivity(he);

			for (HypernodeID node : _hg.pins(he)) {
				if (node != hn) {

					if (connectivity == 2 && pin_count_in_target_part_after == 1
							&& pin_count_in_source_part_before > 1) {
						for (PartitionID i = 0;
								i < _config.initial_partitioning.k; i++) {
							if (i != from) {
								deltaNodeUpdate(*bq[i], node,
										_hg.edgeWeight(he));
							}
						}
					}

					if (connectivity == 1
							&& pin_count_in_source_part_before == 1) {
						for (PartitionID i = 0;
								i < _config.initial_partitioning.k; i++) {
							if (i != to) {
								deltaNodeUpdate(*bq[i], node,
										-_hg.edgeWeight(he));
							}
						}
					}

					if (pin_count_in_target_part_after
							== _hg.edgeSize(he) - 1) {
						if (_hg.partID(node) != to) {
							deltaNodeUpdate(*bq[to], node, _hg.edgeWeight(he));
						}
					}

					if (pin_count_in_source_part_before
							== _hg.edgeSize(he) - 1) {
						if (_hg.partID(node) != from) {
							deltaNodeUpdate(*bq[from], node,
									-_hg.edgeWeight(he));
						}
					}

				}
			}
		}
	}

	void deltaNodeUpdate(PrioQueue& bq, HypernodeID hn, Gain delta_gain) {
		if (bq.contains(hn)) {
			Gain old_gain = bq.key(hn);
			bq.updateKey(hn, old_gain + delta_gain);
		}
	}

	void processNodeForBucketPQ(PrioQueue& bq, const HypernodeID hn,
			const PartitionID target_part, bool updateGain = false) {
		if (_hg.partID(hn) != target_part) {
			if (!bq.contains(hn)) {
				Gain gain = GainComputation::calculateGain(_hg, hn,
						target_part);
				bq.insert(hn, gain);
			}
			if(updateGain) {
				if (bq.contains(hn)) {
					Gain gain = GainComputation::calculateGain(_hg, hn,
							target_part);
					bq.updateKey(hn, gain);
				}
			}
		}
	}



	void deleteAssignedNodeInBucketPQ(
			std::vector<PrioQueue*>& bq, HypernodeID hn) {
		for (int i = 0; i < bq.size(); i++) {
			if (bq[i]->contains(hn)) {
				bq[i]->remove(hn);
			}
		}
	}



	//double max_net_size;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	static const Gain initial_gain = std::numeric_limits<Gain>::min();
	static const HypernodeID invalid_node =
			std::numeric_limits<HypernodeID>::max();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGGLOBALINITIALPARTITIONER_H_ */
