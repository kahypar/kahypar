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
	RandomInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~RandomInitialPartitioner() {
	}

private:

	void kwayPartitionImpl() final {
		InitialPartitionerBase::resetPartitioning(
				_config.initial_partitioning.unassigned_part);

		for (const HypernodeID hn : _hg.nodes()) {
			PartitionID p = -1;
			do {
				p = Randomize::getRandomInt(0,
						_config.initial_partitioning.k - 1);
			} while (_hg.partWeight(p) + _hg.nodeWeight(hn)
					> _config.initial_partitioning.upper_allowed_partition_weight[p]);

			if(p == _config.initial_partitioning.unassigned_part) {
				break;
			}
			assignHypernodeToPartition(hn, p);
		}
		InitialPartitionerBase::rollbackToBestCut();
		InitialPartitionerBase::performFMRefinement();
	}

	void bisectionPartitionImpl() final {
		PartitionID k = _config.initial_partitioning.k;
		_config.initial_partitioning.k = 2;
		kwayPartitionImpl();
		_config.initial_partitioning.k = k;
	}

	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

}
;

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_RANDOMINITIALPARTITIONER_H_ */
