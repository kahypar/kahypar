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
#include "partition/initial_partitioning/IInitialPartitioner.h";
#include "partition/initial_partitioning/InitialPartitionerBase.h";
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



		void kwayPartitionImpl() final {

		}

		void bisectionPartitionImpl() final {
			std::queue<HypernodeID> bfs;
			HypernodeID startNode = Randomize::getRandomInt(1,_hg.numNodes());
			bfs.push(startNode);
			while(true) {
				HypernodeID hn = bfs.front(); bfs.pop();
				if(_hg.partID(hn) != -1)
					continue;
				if(!assignHypernodeToPartition(hn,0))
					break;
				for(HyperedgeID he : _hg.incidentEdges(hn)) {
					for(HypernodeID hnodes : _hg.pins(he))
						bfs.push(hnodes);
				}
				if(bfs.empty()) {
					HypernodeID newStartNode = Randomize::getRandomInt(1,_hg.numNodes());
					while(_hg.partID(newStartNode) != -1)
						newStartNode = Randomize::getRandomInt(0,_hg.numNodes());
					bfs.push(startNode);
				}
			}
			for(HypernodeID hn : _hg.nodes()) {
				if(_hg.partID(hn) != 0)
					assignHypernodeToPartition(hn,1);
			}
		}

		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_BFSINITIALPARTITIONER_H_ */
