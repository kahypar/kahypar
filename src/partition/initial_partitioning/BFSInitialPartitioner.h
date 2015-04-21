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



		void pushIncidentHyperedgesIntoQueue(std::queue<HypernodeID>& q, HypernodeID hn, int max_incident_edge_count) {
			std::vector<std::pair<int,HyperedgeID>> hyperedge_size;
			for(HyperedgeID he : _hg.incidentEdges(hn)) {
				hyperedge_size.push_back(std::make_pair(_hg.edgeSize(he),he));
			}
			std::sort(hyperedge_size.begin(), hyperedge_size.end());
			for(int i = 0; i < hyperedge_size.size(); i++) {
				for(HypernodeID hnodes : _hg.pins(hyperedge_size[i].second)) {
					int count = 0;
					for(HyperedgeID he : _hg.incidentEdges(hnodes))
						count++;
					if(_hg.partID(hnodes) == -1 && count < max_incident_edge_count)
						q.push(hnodes);
				}
			}
		}

		void kwayPartitionImpl() final {
			std::vector<std::queue<HypernodeID>> bfs(_config.initial_partitioning.k, std::queue<HypernodeID>());
			std::vector<bool> partEnable(_config.initial_partitioning.k, true);
			std::vector<HypernodeID> startNodes;
			StartNodeSelection::calculateStartNodes(startNodes,_hg,_config.initial_partitioning.k);
			for(unsigned int i = 0; i < startNodes.size(); i++) {
				bfs[i].push(startNodes[i]);
			}
			unsigned int assignedNodes = 0;
			int max_incident_edge_count = 10;
			while(true) {
				for(unsigned int i = 0; i < _config.initial_partitioning.k; i++) {
					if(partEnable[i] && !bfs[i].empty()) {
						HypernodeID hn = bfs[i].front(); bfs[i].pop();
						if(_hg.partID(hn) != -1)
							continue;
						if(!assignHypernodeToPartition(hn,i))
							partEnable[i] = false;
						else
							assignedNodes++;
						pushIncidentHyperedgesIntoQueue(bfs[i],hn,max_incident_edge_count);
					}
					if(partEnable[i] && bfs[i].empty() && assignedNodes != _hg.numNodes()) {
						HypernodeID newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
						while(_hg.partID(newStartNode) != -1) {
							newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
						}
						bfs[i].push(newStartNode);
						max_incident_edge_count += 2;
					}
				}
				if(assignedNodes == _hg.numNodes())
					break;
				max_incident_edge_count++;
			}
			InitialPartitionerBase::performFMRefinement();
		}

		void bisectionPartitionImpl() final {
			std::stack<std::pair<int,HypernodeID>> cutStack;
			std::queue<HypernodeID> bfs;
			std::vector<HypernodeID> startNode;
			PartitionID k = 2;
			StartNodeSelection::calculateStartNodes(startNode,_hg,k);
			bfs.push(startNode[0]);
			int max_incident_edge_count = 2;
			while(true) {
				if(!bfs.empty()) {
					HypernodeID hn = bfs.front(); bfs.pop();
					if(_hg.partID(hn) != -1)
						continue;
					if(!assignHypernodeToPartition(hn,0,true))
						break;
					pushIncidentHyperedgesIntoQueue(bfs,hn,max_incident_edge_count);
				}
				else {
					HypernodeID newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
					while(_hg.partID(newStartNode) != -1)
						newStartNode = Randomize::getRandomInt(0,_hg.numNodes()-1);
					bfs.push(newStartNode);
					max_incident_edge_count += 10;
				}
					max_incident_edge_count++;
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
