/*
 * TestInitialPartitioner.h
 *
 *  Created on: Apr 24, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_TESTINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_TESTINITIALPARTITIONER_H_

#include <vector>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;

namespace partition {

class TestInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

	public:
		TestInitialPartitioner(Hypergraph& hypergraph,
				Configuration& config) :
				InitialPartitionerBase(hypergraph, config) {
		}

		~TestInitialPartitioner() {
		}

	private:



		void kwayPartitionImpl() final {

		}

		void bisectionPartitionImpl() final {

			PartitionID unassigned_part = 1;

			for (const HypernodeID hn : _hg.nodes()) {
					_hg.setNodePart(hn,1);
			}

			std::vector<std::vector<int>> hyperedge_count(_hg.numNodes(),std::vector<int>(_hg.numNodes(),0));
			HypernodeID bi = 0; HypernodeID bj = 0;

			for(HypernodeID hn : _hg.nodes()) {
				for(HyperedgeID he : _hg.incidentEdges(hn) ) {
					for(HypernodeID hnode : _hg.pins(he)) {
						if(hn < hnode) {
							hyperedge_count[hn][hnode]++;
							hyperedge_count[hnode][hn] = hyperedge_count[hn][hnode];
							if(hyperedge_count[bi][bj] < hyperedge_count[hn][hnode]) {
								bi = hn;
								bj = hnode;
							}
						}
					}
				}
			}

			assignHypernodeToPartition(bi,0,unassigned_part,true);
			HypernodeID current_node = bj;
			while(assignHypernodeToPartition(current_node,0,-1,true)) {
				HypernodeID best_node = -1;
				int best_count = -1;
				for(HypernodeID hn : _hg.nodes()) {
					if(_hg.partID(hn) == unassigned_part && best_count < hyperedge_count[current_node][hn]) {
						best_count = hyperedge_count[current_node][hn];
						best_node = hn;
					}
				}
				current_node = best_node;
			}




			InitialPartitionerBase::rollbackToBestBisectionCut();
			InitialPartitionerBase::performFMRefinement();
		}

		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}



#endif /* SRC_PARTITION_INITIAL_PARTITIONING_TESTINITIALPARTITIONER_H_ */
