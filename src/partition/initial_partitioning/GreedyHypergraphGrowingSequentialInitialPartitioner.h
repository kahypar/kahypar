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
#include "partition/initial_partitioning/GreedyHypergraphGrowingBaseFunctions.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "tools/RandomFunctions.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"

using datastructure::NoDataBinaryMaxHeap;
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

	void kwayPartitionImpl() final {
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		InitialPartitionerBase::resetPartitioning(unassigned_part);

		Gain init_pq = _hg.numNodes();
		std::vector<PrioQueue*> bq(_config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			bq[i] = new PrioQueue(init_pq);
		}

		//Calculate Startnodes and push them into the queues.
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				_config.initial_partitioning.k);
		for (PartitionID i = 0; i < startNodes.size(); i++) {
			greedy_base.processNodeForBucketPQ(*bq[i], startNodes[i], i);
		}

		std::vector<std::vector<bool>> hyperedge_already_process(_hg.k(),
				std::vector<bool>(_hg.numEdges()));

		// TODO(heuer): This does not seem to be fair. In other variants you
		// (1) have a less tight bound, (2) you unlock the upper bound later on...
		// Why don't you also perform an unlock here?
		//Define a weight bound, which every partition have to reach, to avoid very small partitions.
		InitialPartitionerBase::recalculateBalanceConstraints(0.0);

		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			if (i != unassigned_part) {

				HypernodeID hn = bq[i]->max();

				// TODO(heuer): Is there a reason why it is a while loop here and a do-while in
				// the other GHG variants?
				while (InitialPartitionerBase::assignHypernodeToPartition(hn, i)) {
					ASSERT(
							[&]() { if(unassigned_part != -1) { Gain gain = bq[i]->maxKey(); _hg.changeNodePart(hn,i,unassigned_part); HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg); _hg.changeNodePart(hn,unassigned_part,i); return metrics::hyperedgeCut(_hg) == (cut_before-gain); } else { return true; } }(),
							"Gain calculation of hypernode " << hn << " failed!");

					bq[i]->deleteMax();

					// TODO(heuer): Performance improvement if you only loop once
					GainComputation::deltaGainUpdate(_hg, bq, hn,
							unassigned_part, i);
					//Pushing incident hypernode into bucket queue or update gain value
					for (HyperedgeID he : _hg.incidentEdges(hn)) {
						if (!hyperedge_already_process[i][he]) {
							for (HypernodeID hnode : _hg.pins(he)) {
								if (_hg.partID(hnode) == unassigned_part) {
									// TODO(heuer): why? if you find this node via bfs, it might as well
									// be a candidate for this part. It it becomes assigned to part i,
									// you just have to find a new start node for the other part.
									auto startNode = std::find(
											startNodes.begin(),
											startNodes.end(), hnode);
									if (startNode == startNodes.end()) {
										greedy_base.processNodeForBucketPQ(
												*bq[i], hnode, i);
									}
								}
							}
							hyperedge_already_process[i][he] = true;
						}
					}

					if (bq[i]->empty()) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode(
										unassigned_part);
						if (hn == invalid_node) {
							break;
						}
						greedy_base.processNodeForBucketPQ(*bq[i], newStartNode,
								i);
					}

					ASSERT(!bq[i]->empty(),
							"Bucket queue of partition " << i << " shouldn't be empty!");
					hn = bq[i]->max();
				}
			}
		}
		InitialPartitionerBase::recalculateBalanceConstraints(
				_config.initial_partitioning.epsilon);
		assignAllUnassignedHypernodesAccordingToTheirGain(bq);

		for (PartitionID k = 0; k < _config.initial_partitioning.k; k++) {
			delete bq[k];
		}

		if(unassigned_part == -1) {
			_hg.initializeNumCutHyperedges();
		}

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

	void assignAllUnassignedHypernodesAccordingToTheirGain(
			std::vector<PrioQueue*>& bq) {
		//Assign first all possible unassigned nodes, which are still left in the priority queue.
		while (true) {
			std::pair<HypernodeID, PartitionID> best_move =
					greedy_base.getMaxGainHypernode(bq);
			HypernodeID best_node = best_move.first;
			PartitionID best_part = best_move.second;
			if (best_part != -1) {
				if (assignHypernodeToPartition(best_node, best_part)) {
					greedy_base.deleteNodeInAllBucketQueues(bq, best_node);
					GainComputation::deltaGainUpdate(_hg, bq, best_node,
							_config.initial_partitioning.unassigned_part,
							best_part);
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
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			bq[i]->clear();
		}

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
					greedy_base.processNodeForBucketPQ(*bq[i], hn, i);
				}
			}
			std::pair<HypernodeID, PartitionID> best_move =
					greedy_base.getMaxGainHypernode(bq);
			HypernodeID best_node = best_move.first;
			PartitionID best_part = best_move.second;
			if (best_part != -1) {
				if (assignHypernodeToPartition(best_node, best_part)) {
					part_weight -= _hg.nodeWeight(best_node);
				}
			} else {
				if (_config.initial_partitioning.unassigned_part == -1) {
					_hg.setNodePart(best_node,
							Randomize::getRandomInt(0,
									_config.initial_partitioning.k - 1));
				}
			}
			greedy_base.deleteNodeInAllBucketQueues(bq, best_node);
		}
	}

	//double max_net_size;
protected:
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	GreedyHypergraphGrowingBaseFunctions<GainComputation> greedy_base;

	static const HypernodeID invalid_node =
			std::numeric_limits<HypernodeID>::max();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGSEQUENTIALINITIALPARTITIONER_H_ */
