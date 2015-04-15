/*
 * BFSInitialPartitioning.h
 *
 *  Created on: Apr 12, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_

#include <queue>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;

namespace partition {

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


		void findStartNodes(std::vector<HypernodeID>& currentStartNodes, PartitionID& k) {
			if(k == 2 ) {
				currentStartNodes.push_back(Randomize::getRandomInt(0,_hg.numNodes()-1));
				return;
			} else if(currentStartNodes.size() == k) {
				return;
			} else if(currentStartNodes.size() == 0) {
				currentStartNodes.push_back(Randomize::getRandomInt(0,_hg.numNodes()-1));
				findStartNodes(currentStartNodes,k);
				return;
			}

			std::vector<bool> visited(_hg.numNodes(),false);
			std::queue<HypernodeID> bfs;
			HypernodeID lastHypernode = -1;
			for(unsigned int i = 0; i < currentStartNodes.size(); i++)
				bfs.push(currentStartNodes[i]);
			while(!bfs.empty()) {
				HypernodeID hn = bfs.front(); bfs.pop();
				if(visited[hn])
					continue;
				lastHypernode = hn; visited[hn] = true;
				for(HyperedgeID he : _hg.incidentEdges(lastHypernode)) {
					for(HypernodeID hnodes : _hg.pins(he)) {
						if(!visited[hnodes])
							bfs.push(hnodes);
					}
				}
			}
			currentStartNodes.push_back(lastHypernode);
			findStartNodes(currentStartNodes,k);
		}

		void kwayPartitionImpl() final {
			std::vector<std::queue<HypernodeID>> bfs(_config.initial_partitioning.k, std::queue<HypernodeID>());
			std::vector<bool> partEnable(_config.initial_partitioning.k, true);
			std::vector<HypernodeID> startNodes;
			findStartNodes(startNodes,_config.initial_partitioning.k);
			for(unsigned int i = 0; i < startNodes.size(); i++) {
				bfs[i].push(startNodes[i]);
			}
			unsigned int assignedNodes = 0;
			while(true) {
				for(unsigned int i = 0; i < startNodes.size(); i++) {
					if(partEnable[i] && !bfs[i].empty()) {
						HypernodeID hn = bfs[i].front(); bfs[i].pop();
						if(_hg.partID(hn) != -1)
							continue;
						if(!assignHypernodeToPartition(hn,i))
							partEnable[i] = false;
						else
							assignedNodes++;
						for(HyperedgeID he : _hg.incidentEdges(hn)) {
							for(HypernodeID hnodes : _hg.pins(he))
								if(_hg.partID(hnodes) == -1)
									bfs[i].push(hnodes);
						}
					}
					if(partEnable[i] && bfs[i].empty() && assignedNodes != _hg.numNodes()) {
						HypernodeID newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
						while(_hg.partID(newStartNode) != -1) {
							newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
						}
						bfs[i].push(newStartNode);
					}
				}
				if(assignedNodes == _hg.numNodes())
					break;
			}
			InitialPartitionerBase::performFMRefinement();
		}

		void bisectionPartitionImpl() final {
			std::stack<std::pair<int,HypernodeID>> cutStack;
			std::queue<HypernodeID> bfs;
			std::vector<HypernodeID> startNode;
			findStartNodes(startNode,_config.initial_partitioning.k);
			bfs.push(startNode[0]);
			while(true) {
				if(!bfs.empty()) {
					HypernodeID hn = bfs.front(); bfs.pop();
					if(_hg.partID(hn) != -1)
						continue;
					if(!assignHypernodeToPartition(hn,0,true))
						break;
					for(HyperedgeID he : _hg.incidentEdges(hn)) {
						for(HypernodeID hnodes : _hg.pins(he)) {
							if(_hg.partID(hnodes) == -1) {
								bfs.push(hnodes);
							}
						}
					}
				}
				else {
					HypernodeID newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
					while(_hg.partID(newStartNode) != -1)
						newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
					bfs.push(newStartNode);
				}

			}
			for(HypernodeID hn : _hg.nodes()) {
				if(_hg.partID(hn) != 0)
					assignHypernodeToPartition(hn,1);
			}
			InitialPartitionerBase::rollbackToBestBisectionCut();
			InitialPartitionerBase::performFMRefinement();
		}


		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_ */
