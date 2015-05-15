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
			InitialPartitionerBase(hypergraph, config) {
	}

	~BFSInitialPartitioner() {
	}

private:
	FRIEND_TEST(ABFSInitialPartionerTest, BFSExpectedHypernodesInQueueAfterPushingIncidentHypernodesInQueue);
	FRIEND_TEST(ABFSInitialPartionerTest, BFSInQueueMapUpdateAfterPushingIncidentHypernodesInQueue);

	void pushIncidentHyperedgesIntoQueue(std::queue<HypernodeID>& q,
			HypernodeID hn, std::unordered_map<HypernodeID, bool>& in_queue,
			PartitionID& unassigned_part) {
		for (HyperedgeID he : _hg.incidentEdges(hn)) {
			for (HypernodeID hnodes : _hg.pins(he)) {
				if (_hg.partID(hnodes) == unassigned_part
						&& !in_queue[hnodes]) {
					q.push(hnodes);
					in_queue[hnodes] = true;
				}
			}
		}
	}

	void kwayPartitionImpl() final {
		PartitionID unassigned_part = -1;
		InitialPartitionerBase::resetPartitioning(unassigned_part);

		//Initialize a vector of queues for each partition
		std::vector<std::queue<HypernodeID>> q(_config.initial_partitioning.k,
				std::queue<HypernodeID>());

		//Initialize a vector for each partition, which indicate if a partition is ready to receive further hypernodes.
		std::vector<bool> partEnable(_config.initial_partitioning.k, true);

		//Initialize a vector for each partition, which indicates the hypernodes which are and were already in the queue.
		std::vector<std::unordered_map<HypernodeID,bool>> in_queue(_hg.k());

		//Calculate Startnodes and push them into the queues.
		std::vector<HypernodeID> startNodes;
		StartNodeSelection::calculateStartNodes(startNodes, _hg,
				_config.initial_partitioning.k);
		for (unsigned int i = 0; i < startNodes.size(); i++) {
			q[i].push(startNodes[i]);
			in_queue[i][startNodes[i]] = true;
		}

		unsigned int assignedNodes = 0;
		while (assignedNodes != _hg.numNodes()) {
			for (PartitionID i = 0; i < _config.initial_partitioning.k; i++) {
				if (partEnable[i]) {
					HypernodeID hn = 0;

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
					if (_hg.partID(hn) != unassigned_part && q[i].empty()) {
						hn = InitialPartitionerBase::getUnassignedNode(
								unassigned_part);
						in_queue[i][hn] = true;
					}

					ASSERT(_hg.partID(hn) == unassigned_part, "Hypernode " << hn << " isn't a node from an unassigned part.");

					pushIncidentHyperedgesIntoQueue(q[i], hn, in_queue[i],
							unassigned_part);

					ASSERT([&]() {
						for (HyperedgeID he : _hg.incidentEdges(hn)) {
							for (HypernodeID hnodes : _hg.pins(he)) {
								if (_hg.partID(hnodes) == unassigned_part && !in_queue[i][hnodes]) {
									return false;
								}
							}
						}
						return true;
					}(),
							"Some hypernodes are missing into the queue!");

					if (assignHypernodeToPartition(hn, i)) {
						assignedNodes++;
					} else {
						if(q[i].empty()) {
							partEnable[i] = false;
						}
					}

					if(assignedNodes == _hg.numNodes()) {
						break;
					}
				}
			}
		}
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		PartitionID unassigned_part = 1;
		std::queue<HypernodeID> bfs;
		std::unordered_map<HypernodeID,bool> in_queue;


		//Calculate Startnode and push them into the queues.
		std::vector<HypernodeID> startNode;
		StartNodeSelection::calculateStartNodes(startNode, _hg,
				static_cast<PartitionID>(2));
		bfs.push(startNode[0]);
		InitialPartitionerBase::resetPartitioning(unassigned_part);
		in_queue[startNode[0]] = true;

		HypernodeID hn;
		do {

			//Searching for an unassigned hypernode in the queue.
			if (!bfs.empty()) {
				hn = bfs.front(); bfs.pop();
			}
			//If no unassigned hypernode was found we have to select a new startnode.
			else {
				hn = InitialPartitionerBase::getUnassignedNode(unassigned_part);
				in_queue[hn] = true;
			}

			ASSERT(_hg.partID(hn) == unassigned_part, "Hypernode " << hn << " isn't a node from an unassigned part.");

			pushIncidentHyperedgesIntoQueue(bfs, hn, in_queue, unassigned_part);
		} while (assignHypernodeToPartition(hn, 0));

		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();
	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_ */
