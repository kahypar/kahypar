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
#include "partition/initial_partitioning/GreedyHypergraphGrowingBaseFunctions.h"
#include "tools/RandomFunctions.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"
using datastructure::PriorityQueue;
using datastructure::NoDataBinaryMaxHeap;

using defs::HypernodeWeight;

using Gain = HyperedgeWeight;

using TwoWayFMHeap = NoDataBinaryMaxHeap<HypernodeID, Gain,
std::numeric_limits<HyperedgeWeight> >;
using PrioQueue = PriorityQueue<TwoWayFMHeap>;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy,
		class GainComputation = GainComputationPolicy>
class GreedyHypergraphGrowingRoundRobinInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	GreedyHypergraphGrowingRoundRobinInitialPartitioner(Hypergraph& hypergraph,
			Configuration& config) :
			InitialPartitionerBase(hypergraph, config), greedy_base(hypergraph,
					config) {

	}

	~GreedyHypergraphGrowingRoundRobinInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		PartitionID unassigned_part = -1;
		InitialPartitionerBase::resetPartitioning(-1);

		Gain init_pq = _hg.numNodes();
		std::vector<PrioQueue*> bq(_config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
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
			greedy_base.processNodeForBucketPQ(*bq[i], startNodes[i], i);
		}

		HypernodeID assigned_nodes_weight = 0;
		if (unassigned_part != -1) {
			assigned_nodes_weight =
					_config.initial_partitioning.perfect_balance_partition_weight[unassigned_part]
							* (1.0 - _config.initial_partitioning.epsilon);
		}

		std::vector<std::vector<bool>> hyperedge_already_process(_hg.k(),
				std::vector<bool>(_hg.numEdges()));

		while (assigned_nodes_weight < _config.partition.total_graph_weight) {

			bool every_part_disable = true;

			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				if (partEnable[i]) {
					every_part_disable = false;
					HypernodeID hn;
					if (bq[i]->empty()) {
						HypernodeID newStartNode =
								InitialPartitionerBase::getUnassignedNode(
										unassigned_part);
						if (newStartNode == invalid_node) {
							continue;
						}
						greedy_base.processNodeForBucketPQ(*bq[i], newStartNode,
								i);
					}
					hn = bq[i]->max();

					ASSERT(_hg.partID(hn) == unassigned_part,
							"Hypernode " << hn << "should be unassigned!");

					if (!assignHypernodeToPartition(hn, i)) {
						if (!bq[i]->empty()) {
							bq[i]->deleteMax();
						}
						if (bq[i]->empty()) {
							partEnable[i] = false;
						}
					} else {

						ASSERT(!bq[i]->empty(),
								"Bucket queue of partition " << i << " shouldn't be empty!");

						ASSERT(
								[&]() { if(_config.initial_partitioning.unassigned_part != -1) { Gain gain = bq[i]->maxKey(); _hg.changeNodePart(hn,i,unassigned_part); HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg); _hg.changeNodePart(hn,unassigned_part,i); return metrics::hyperedgeCut(_hg) == (cut_before-gain); } return true; }(),
								"Gain calculation failed!");

						bq[i]->deleteMax();
						greedy_base.deleteNodeInAllBucketQueues(bq, hn);

						assigned_nodes_weight += _hg.nodeWeight(hn);

						// TODO(heuer): You can easily be a factor of 2 faster, if you combine
						// delta-gain updates and new insertions in ONE loop over hes and pins
						// instead of 2 separate loops. This also holds true for GHG-Global
						// and might even be true for BFS.
						GainComputation::deltaGainUpdate(_hg, bq, hn,
								unassigned_part, i);
						for (HyperedgeID he : _hg.incidentEdges(hn)) {
							if (!hyperedge_already_process[i][he]) {
								for (HypernodeID hnode : _hg.pins(he)) {
									if (_hg.partID(hnode) == unassigned_part) {
										greedy_base.processNodeForBucketPQ(
												*bq[i], hnode, i);
									}
								}
								hyperedge_already_process[i][he] = true;
							}
						}
					}

				}
				if (assigned_nodes_weight
						== _config.partition.total_graph_weight) {
					break;
				}
			}
			if (every_part_disable) {
				break;
			}
		}

		for (PartitionID k = 0; k < _config.initial_partitioning.k; k++) {
			delete bq[k];
		}

		for (HypernodeID hn : _hg.nodes()) {
			if (_hg.partID(hn) == -1) {
				Gain gain0 = GainComputation::calculateGain(_hg, hn, 0);
				Gain gain1 = GainComputation::calculateGain(_hg, hn, 1);
				if (gain0 > gain1) {
					_hg.setNodePart(hn, 0);
				} else {
					_hg.setNodePart(hn, 1);
				}
			}
		}

		if(unassigned_part == -1) {
			_hg.initializeNumCutHyperedges();
		}

		InitialPartitionerBase::recalculateBalanceConstraints(
				_config.initial_partitioning.epsilon);
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

	static const Gain initial_gain = std::numeric_limits<Gain>::min();

	static const HypernodeID invalid_node =
			std::numeric_limits<HypernodeID>::max();

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDYHYPERGRAPHGROWINGROUNDROBININITIALPARTITIONER_H_ */
