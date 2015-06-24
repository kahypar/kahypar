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
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		InitialPartitionerBase::resetPartitioning(-1);

		for (const HypernodeID hn : _hg.nodes()) {
			PartitionID p = -1;
			std::vector<bool> has_try_partition(_config.initial_partitioning.k,
					false);
			int partition_sum = 0;
			do {
				if (p != -1 && !has_try_partition[p]) {
					partition_sum += (p + 1);
					has_try_partition[p] = true;
					if (partition_sum
							== (_config.initial_partitioning.k
									* (_config.initial_partitioning.k + 1))
									/ 2) {
						_hg.setNodePart(hn, p);
						_config.initial_partitioning.rollback = false;
						break;
					}
				}
				p = Randomize::getRandomInt(0,
						_config.initial_partitioning.k - 1);
			} while (!assignHypernodeToPartition(hn, p));
		}
		_hg.initializeNumCutHyperedges();
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
