/*
 * BFSInitialPartitioning.h
 *
 *  Created on: Apr 12, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_

#include <queue>
#include <unordered_map>
#include <algorithm>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;
using partition::StartNodeSelectionPolicy;

namespace partition {

template<class StartNodeSelection = StartNodeSelectionPolicy>
class BFSInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	BFSInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config), q() {
	}

	~BFSInitialPartitioner() {
	}

private:
	FRIEND_TEST(ABFSBisectionInitialPartionerTest, HasCorrectInQueueMapValuesAfterPushingIncidentHypernodesNodesIntoQueue);FRIEND_TEST(ABFSBisectionInitialPartionerTest, HasCorrectHypernodesIntoQueueAfterPushingIncidentHypernodesIntoQueue);

	void pushIncidentHypernodesIntoQueue(std::queue<HypernodeID>& q,
			HypernodeID hn, std::vector<bool>& in_queue,
			std::vector<bool>& hyperedge_in_queue) {
		for (const HyperedgeID he : _hg.incidentEdges(hn)) {
			if (!hyperedge_in_queue[he]) {
				for (const HypernodeID hnodes : _hg.pins(he)) {
					if (_hg.partID(hnodes) == _config.initial_partitioning.unassigned_part
							&& !in_queue[hnodes]) {
						q.push(hnodes);
						in_queue[hnodes] = true;
					}
				}
				hyperedge_in_queue[he] = true;
			}
		}
	}

	// TODO(heuer): If I'm right, the k-way impl could be implemented in the same way as the different
	// greedy variants. Did you try these as well?
	void initialPartition() final {
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		InitialPartitionerBase::resetPartitioning();

		//Initialize a vector of queues for each partition
		q.clear();
		q.assign(_config.initial_partitioning.k, std::queue<HypernodeID>());

		//Initialize a vector for each partition, which indicate if a partition is ready to receive further hypernodes.
		std::vector<bool> partEnable(_config.initial_partitioning.k, true);
		if (unassigned_part != -1) {
			partEnable[unassigned_part] = false;
		}

		HypernodeWeight assigned_nodes_weight = 0;
		if (unassigned_part != -1) {
			//TODO(heuer): Warum ist hier -epsilon? 
			assigned_nodes_weight =
					_config.initial_partitioning.perfect_balance_partition_weight[unassigned_part]
							* (1.0 - _config.initial_partitioning.epsilon);
		}

		//Initialize a vector for each partition, which indicates the hypernodes which are and were already in the queue.
		//TODO(heuer): This can be done more efficiently. Why not use vector<bool> instead of unordered maps?
		//TODO(heuer): Is it possible to get this into one vector<bool> instead of vectors of vectors?
		std::vector<std::vector<bool>> in_queue(_hg.k(),
				std::vector<bool>(_hg.numNodes(), false));
		std::vector<std::vector<bool>> hyperedge_in_queue(_hg.k(),
				std::vector<bool>(_hg.numEdges(), false));

		//Calculate Startnodes and push them into the queues.
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				_config.initial_partitioning.k);
		// TODO(heuer): Also, why build the start node vector only to then insert the nodes into
		// the queue. Why not directly insert them into the queue?
		for (PartitionID k = 0; k < startNodes.size(); k++) {
			q[k].push(startNodes[k]);
			in_queue[k][startNodes[k]] = true;
		}

		while (assigned_nodes_weight < _config.partition.total_graph_weight) {
			bool every_part_is_disable = true;
			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				every_part_is_disable = every_part_is_disable && !partEnable[i];
				if (partEnable[i]) {
					HypernodeID hn = invalid_hypernode;

					//Searching for an unassigned hypernode in queue for Partition i
					if (!q[i].empty()) {
						hn = q[i].front();
						q[i].pop();
						while (_hg.partID(hn) != unassigned_part
								&& !q[i].empty()) {
							hn = q[i].front();
							q[i].pop();
						}
					}

					//If no unassigned hypernode was found we have to select a new startnode.
					if (hn == invalid_hypernode
							|| _hg.partID(hn) != unassigned_part) {
						hn = InitialPartitionerBase::getUnassignedNode();
					}

					if (hn != invalid_hypernode) {
						in_queue[i][hn] = true;
						ASSERT(_hg.partID(hn) == unassigned_part,
								"Hypernode " << hn << " isn't a node from an unassigned part.");

						if (assignHypernodeToPartition(hn, i)) {
							assigned_nodes_weight += _hg.nodeWeight(hn);

							pushIncidentHypernodesIntoQueue(q[i], hn,
									in_queue[i], hyperedge_in_queue[i]);

							ASSERT(
									[&]() { for (HyperedgeID he : _hg.incidentEdges(hn)) { for (HypernodeID hnodes : _hg.pins(he)) { if (_hg.partID(hnodes) == unassigned_part && !in_queue[i][hnodes]) { return false; } } } return true; }(),
									"Some hypernodes are missing into the queue!");
						} else {
							if (q[i].empty()) {
								partEnable[i] = false;
							}
						}
					} else {
						partEnable[i] = false;
					}
				}
			}

			if (every_part_is_disable) {
				break;
			}
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
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	const HypernodeID invalid_hypernode =
			std::numeric_limits<HypernodeID>::max();
	std::vector<std::queue<HypernodeID>> q;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_ */
