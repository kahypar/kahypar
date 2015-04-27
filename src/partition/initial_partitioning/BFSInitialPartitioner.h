/*
 * BFSInitialPartitioning.h
 *
 *  Created on: Apr 12, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_

#include <queue>
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
		BFSInitialPartitioner(Hypergraph& hypergraph,
				Configuration& config) :
				InitialPartitionerBase(hypergraph, config) {
		}

		~BFSInitialPartitioner() {
		}

	private:



		void pushIncidentHyperedgesIntoQueue(std::queue<HypernodeID>& q, HypernodeID hn, std::vector<bool>& in_queue, PartitionID& unassigned_part) {
			std::vector<std::pair<int,HyperedgeID>> hyperedge_size;
			for(HyperedgeID he : _hg.incidentEdges(hn)) {
				hyperedge_size.push_back(std::make_pair(_hg.edgeSize(he),he));
			}
			std::sort(hyperedge_size.begin(), hyperedge_size.end());
			for(int i = 0; i < hyperedge_size.size(); i++) {
				for(HypernodeID hnodes : _hg.pins(hyperedge_size[i].second)) {
					if(_hg.partID(hnodes) == unassigned_part && !in_queue[hnodes]) {
						q.push(hnodes);
						in_queue[hnodes] = true;
					}
				}
			}
		}

		void kwayPartitionImpl() final {
			PartitionID unassigned_part = -1;
			std::vector<std::queue<HypernodeID>> bfs(_config.initial_partitioning.k, std::queue<HypernodeID>());
			std::vector<bool> partEnable(_config.initial_partitioning.k, true);
			std::vector<HypernodeID> startNodes;
			StartNodeSelection::calculateStartNodes(startNodes,_hg,_config.initial_partitioning.k);
			for(unsigned int i = 0; i < startNodes.size(); i++) {
				bfs[i].push(startNodes[i]);
			}
			unsigned int assignedNodes = 0;
			std::vector<bool> in_queue(_hg.numNodes(),false);
			while(true) {
				for(unsigned int i = 0; i < _config.initial_partitioning.k; i++) {
					if(partEnable[i] && !bfs[i].empty()) {
						HypernodeID hn = bfs[i].front(); bfs[i].pop();
						if(_hg.partID(hn) != unassigned_part) {
							continue;
						}
						if(!assignHypernodeToPartition(hn,i)) {
							partEnable[i] = false;
						}
						else {
							assignedNodes++;
						}
						pushIncidentHyperedgesIntoQueue(bfs[i],hn,in_queue,unassigned_part);
					}
					if(partEnable[i] && bfs[i].empty() && assignedNodes != _hg.numNodes()) {
						HypernodeID newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
						while(_hg.partID(newStartNode) != -1) {
							newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
						}
						bfs[i].push(newStartNode);
					}
				}
				if(assignedNodes == _hg.numNodes()) {
					break;
				}
			}
			InitialPartitionerBase::performFMRefinement();
		}

		void bisectionPartitionImpl() final {
			PartitionID unassigned_part = 1;
			std::stack<std::pair<int,HypernodeID>> cutStack;
			std::queue<HypernodeID> bfs;
			std::vector<HypernodeID> startNode;
			PartitionID k = 2;
			StartNodeSelection::calculateStartNodes(startNode,_hg,k);
			bfs.push(startNode[0]);
			for (HypernodeID hn : _hg.nodes()) {
				_hg.setNodePart(hn, 1);
			}
			std::vector<bool> in_queue(_hg.numNodes(),false);
			while(true) {
				if(!bfs.empty()) {
					HypernodeID hn = bfs.front(); bfs.pop();
					if(_hg.partID(hn) != unassigned_part) {
						continue;
					}
					if(!assignHypernodeToPartition(hn,0,1,true)) {
						break;
					}
					pushIncidentHyperedgesIntoQueue(bfs,hn,in_queue,unassigned_part);
				}
				else {
					HypernodeID newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
					while(_hg.partID(newStartNode) != 1) {
						newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
					}
					bfs.push(newStartNode);
				}
			}

			InitialPartitionerBase::rollbackToBestBisectionCut();
			InitialPartitionerBase::performFMRefinement();
		}


		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_ */
