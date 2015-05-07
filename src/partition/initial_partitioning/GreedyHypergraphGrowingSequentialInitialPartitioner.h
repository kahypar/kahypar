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
#include "lib/datastructure/BucketQueue.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/HypergraphPartitionBalancer.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "tools/RandomFunctions.h"
#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"

using external::NoDataBinaryMaxHeap;
using defs::HypernodeWeight;
using partition::HypergraphPartitionBalancer;
using partition::StartNodeSelectionPolicy;
using partition::GainComputationPolicy;
using datastructure::BucketQueue;
using datastructure::PriorityQueue;

using Gain = HyperedgeWeight;

using TwoWayFMHeap = NoDataBinaryMaxHeap<HypernodeID, Gain,
std::numeric_limits<HyperedgeWeight> >;
using PrioQueue = PriorityQueue<TwoWayFMHeap>;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingSequentialInitialPartitioner: public IInitialPartitioner,
		public InitialPartitionerBase {

public:
	GreedyHypergraphGrowingSequentialInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config), _balancer(_hg, config) {

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

	void kwayPartitionImpl() final {
		PartitionID unassigned_part = 0;
		if (unassigned_part != -1) {
			for (HypernodeID hn : _hg.nodes()) {
				_hg.setNodePart(hn, unassigned_part);
			}
		}

		Gain init_pq = 2 * _hg.numNodes();
		std::vector<PrioQueue*> bq(_config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			bq[i] = new PrioQueue(init_pq);
		}

		//Calculate Startnodes and push them into the queues.
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				_config.initial_partitioning.k);

		for (PartitionID i = 0; i < startNodes.size(); i++) {
			processNodeForBucketPQ(*bq[i], startNodes[i], i);
		}

		std::sort(startNodes.begin(), startNodes.end());

		//Define a weight bound, which every partition have to reach, to avoid very small partitions.
		double epsilon = _config.partition.epsilon;
		_config.partition.epsilon = 0.0;
		InitialPartitionerBase::recalculateBalanceConstraints();


		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if (i != unassigned_part) {

				processNodeForBucketPQ(*bq[i], bq[i]->max(), i);
				HypernodeID hn = bq[i]->max();
				while (InitialPartitionerBase::assignHypernodeToPartition(hn, i)) {



					ASSERT([&]() {
						Gain gain = bq[i]->maxKey();
						_hg.changeNodePart(hn,i,unassigned_part);
						HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
						_hg.changeNodePart(hn,unassigned_part,i);
						return metrics::hyperedgeCut(_hg) == (cut_before-gain);
					}(), "Gain calculation of hypernode " << hn << " failed!");

					bq[i]->deleteMax();

					deltaGainUpdate(bq, hn, unassigned_part, i);
					//Pushing incident hypernode into bucket queue or update gain value
					for (HyperedgeID he : _hg.incidentEdges(hn)) {
						for (HypernodeID hnode : _hg.pins(he)) {
							if (_hg.partID(hnode) == unassigned_part) {
								auto startNode = std::find(startNodes.begin(),
										startNodes.end(), hnode);
								if (startNode == startNodes.end()) {
									processNodeForBucketPQ(*bq[i], hnode, i);
								}
							}
						}
					}

					if (bq[i]->empty()) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode(
										unassigned_part);
						processNodeForBucketPQ(*bq[i], newStartNode, i);
					}

					ASSERT(!bq[i]->empty(),
							"Bucket queue of partition " << i << " shouldn't be empty!");
					hn = bq[i]->max();


				}
			}
		}


		_config.partition.epsilon = epsilon;
		InitialPartitionerBase::recalculateBalanceConstraints();

		_balancer.balancePartitions();

		InitialPartitionerBase::rollbackToBestBisectionCut();
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

		InitialPartitionerBase::rollbackToBestBisectionCut();
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


	void deleteAssignedNodeInBucketPQ(std::vector<PrioQueue*>& bq,
			HypernodeID hn) {
		for (int i = 0; i < bq.size(); i++) {
			if (bq[i]->contains(hn)) {
				bq[i]->remove(hn);
			}
		}
	}

	//double max_net_size;
protected:
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;
	static const HypernodeID invalid_node =
			std::numeric_limits<HypernodeID>::max();

	HypergraphPartitionBalancer _balancer;

};

}



#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGSEQUENTIALINITIALPARTITIONER_H_ */
