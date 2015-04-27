/*
 * RandomInitialPartitioner.h
 *
 *  Created on: Apr 3, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_RANDOMINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_RANDOMINITIALPARTITIONER_H_

#include <vector>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;

namespace partition {

class RandomInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

	public:
		RandomInitialPartitioner(Hypergraph& hypergraph,
				Configuration& config) :
				InitialPartitionerBase(hypergraph, config) {
		}

		~RandomInitialPartitioner() {
		}

	private:



		void kwayPartitionImpl() final {
			for (const HypernodeID hn : _hg.nodes()) {
				PartitionID p = Randomize::getRandomInt(0,
						_config.initial_partitioning.k - 1);
				while (!assignHypernodeToPartition(hn, p))
					p = (p + 1) % _config.initial_partitioning.k;
			}
			InitialPartitionerBase::performFMRefinement();
		}

		void bisectionPartitionImpl() final {
			for (HypernodeID hn : _hg.nodes())
				_hg.setNodePart(hn, 1);
			HypernodeID hn = Randomize::getRandomInt(0,
					_hg.numNodes()-1);
			while(assignHypernodeToPartition(hn,0,1,true)) {
				hn = Randomize::getRandomInt(0,
									_hg.numNodes()-1);
				while(_hg.partID(hn) != 1) {
					hn = Randomize::getRandomInt(0,
										_hg.numNodes()-1);
				}
			}
			InitialPartitionerBase::rollbackToBestBisectionCut();
			InitialPartitionerBase::performFMRefinement();
		}

		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_RANDOMINITIALPARTITIONER_H_ */
