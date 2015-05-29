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
#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"
using datastructure::PriorityQueue;
using external::NoDataBinaryMaxHeap;

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
			InitialPartitionerBase(hypergraph, config),
			greedy_base(hypergraph,config) {

		_config.initial_partitioning.unassigned_part = -1;
	}

	~GreedyHypergraphGrowingRoundRobinInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		PartitionID unassigned_part = _config.initial_partitioning.unassigned_part;
		InitialPartitionerBase::resetPartitioning(unassigned_part);

		Gain init_pq = _hg.numNodes();
		std::vector<PrioQueue*> bq(_config.initial_partitioning.k);
		for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
			bq[i] = new PrioQueue(init_pq);
		}

		//Enable parts are allowed to receive further hypernodes.
		// TODO(heuer): Since the code is pretty similar to GHG-Global, I
		// don't repeat the obvious comments here.
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
						greedy_base.processNodeForBucketPQ(*bq[i], newStartNode, i);
						// TODO(heuer): Here, you also use the start node directly.
					}
					hn = bq[i]->max();

					ASSERT(_hg.partID(hn) == unassigned_part,
							"Hypernode " << hn << "should be unassigned!");

					if (!assignHypernodeToPartition(hn, i)) {
						// TODO(heuer): Is this the right thing to do?
						// If the assignment was not feasible... maybe hn was just wayyyy to heavy
						// to be assigned to that part, but another hypernode with a slightly smaller
						// gain (or in the worst case with the same gain but considerable less weight)
						// would now be on top of the pq. Wouldn't it therefore be better to just
						// remove hn from pq i only and try to find other feasible moves?
						partEnable[i] = false;

					} else {

						ASSERT(!bq[i]->empty(),
								"Bucket queue of partition " << i << " shouldn't be empty!");

						ASSERT(
								[&]() {
									Gain gain = bq[i]->maxKey();
									_hg.changeNodePart(hn,i,unassigned_part);
									HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
									_hg.changeNodePart(hn,unassigned_part,i);
									return metrics::hyperedgeCut(_hg) == (cut_before-gain);
								}(), "Gain calculation failed!");

						bq[i]->deleteMax();
						// TODO(heuer): Method name is misleading. You delete the node in all pqs...
						greedy_base.deleteAssignedNodeInBucketPQ(bq, hn);

						assigned_nodes_weight += _hg.nodeWeight(hn);

						// TODO(heuer): You can easily be a factor of 2 faster, if you combine
						// delta-gain updates and new insertions in ONE loop over hes and pins
						// instead of 2 separate loops. This also holds true for GHG-Global
						// and might even be true for BFS.
						GainComputation::deltaGainUpdate(_hg,bq, hn, unassigned_part, i);
						for (HyperedgeID he : _hg.incidentEdges(hn)) {
							for (HypernodeID hnode : _hg.pins(he)) {
								if (_hg.partID(hnode) == unassigned_part) {
									greedy_base.processNodeForBucketPQ(*bq[i], hnode, i);
								}
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

	// TODO(heuer): If I'm correct, this is exactly the same as GHG-Global?
	// What does this have to do with round-robin?
	// A Round-Robin implementation however would make sense i think.
	// Also the implementation for k=2 should be the same as for k !=2
	// to see consisten behavior. It doesn't make sense to compare GHG-Global
	// and GHG-RoundRobin since now, for k=2 both are exactly the same.
	// This should not be intended!
	// Since the implementation is the same as GHG-Global, it has the same BUGs
	// in it. :(
	/*void bisectionPartitionImpl() final {
		PartitionID unassigned_part = 1;
		PartitionID assigned_part = 1 - unassigned_part;
		InitialPartitionerBase::resetPartitioning(unassigned_part);

		HyperedgeWeight init_pq = 2 * _hg.numNodes();
		std::vector<PrioQueue*> bq(2);
		for (PartitionID i = 0; i < 2; i++) {
			bq[i] = new PrioQueue(init_pq);
		}
		std::vector<HypernodeID> startNode;
		StartNodeSelection::calculateStartNodes(startNode, _hg,
				static_cast<PartitionID>(2));

		greedy_base.processNodeForBucketPQ(*bq[assigned_part], startNode[assigned_part],
				assigned_part);
		HypernodeID hn = invalid_node;
		do {

			if (hn != invalid_node) {

				ASSERT([&]() {
					Gain gain = bq[assigned_part]->maxKey();
					_hg.changeNodePart(hn,assigned_part,unassigned_part);
					HyperedgeWeight cut_before = metrics::hyperedgeCut(_hg);
					_hg.changeNodePart(hn,unassigned_part,assigned_part);
					return metrics::hyperedgeCut(_hg) == (cut_before-gain);
				}(), "Gain calculation failed!");

				bq[0]->deleteMax();

				GainComputation::deltaGainUpdate(_hg,bq, hn, unassigned_part, assigned_part);
				for (HyperedgeID he : _hg.incidentEdges(hn)) {
					for (HypernodeID hnode : _hg.pins(he)) {
						if (_hg.partID(hnode) == unassigned_part
								&& hnode != hn) {
							greedy_base.processNodeForBucketPQ(*bq[assigned_part], hnode,
									assigned_part);
						}
					}
				}
			}

			if (!bq[assigned_part]->empty()) {
				hn = bq[assigned_part]->max();
			}

			if (bq[assigned_part]->empty()
					&& _hg.partID(hn) != unassigned_part) {
				hn = InitialPartitionerBase::getUnassignedNode(unassigned_part);
				greedy_base.processNodeForBucketPQ(*bq[assigned_part], hn, assigned_part);
			}

			ASSERT(_hg.partID(hn) == unassigned_part,
					"Hypernode " << hn << " should be from part 1!");

		} while (assignHypernodeToPartition(hn, assigned_part));

		if (unassigned_part == -1) {
			for (HypernodeID hn : _hg.nodes()) {
				if (_hg.partID(hn) == -1) {
					_hg.setNodePart(hn, 1);
				}
			}
		}

		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();

	}*/

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
