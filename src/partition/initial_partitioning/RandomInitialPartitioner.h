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
#include "partition/initial_partitioning/IInitialPartitioner.h";
#include "partition/initial_partitioning/InitialPartitionerBase.h";
#include "tools/RandomFunctions.h"

using defs::HypernodeWeight;

namespace partition {

class RandomInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

	public:
		RandomInitialPartitioner(Hypergraph& hypergraph,
				const Configuration& config) :
				InitialPartitionerBase(hypergraph, config) {
		}

		~RandomInitialPartitioner() {
		}

	private:

		bool assignHypernodeToPartition(HypernodeID hn, PartitionID p) {
			HypernodeWeight assign_partition_weight = _hg.partWeight(p)
					+ _hg.nodeWeight(hn);
			if (assign_partition_weight
					< _config.initial_partitioning.upper_allowed_partition_weight) {
				_hg.setNodePart(hn, p);
				return true;
			} else {
				return false;
			}
		}

		void balancePartition(PartitionID k) {
			for (const HypernodeID hn : _hg.nodes()) {
				PartitionID nodePart = _hg.partID(hn);
				if(nodePart != k && _hg.partWeight(nodePart) - _hg.nodeWeight(hn) >= _config.initial_partitioning.lower_allowed_partition_weight)
					_hg.changeNodePart(hn,nodePart,k);
				if(_hg.partWeight(k) >= _config.initial_partitioning.lower_allowed_partition_weight)
					break;
			}
		}

		void balancePartitions() {
			for (PartitionID k = 0; k < _config.initial_partitioning.k; k++) {
				if (_hg.partWeight(k)
						< _config.initial_partitioning.lower_allowed_partition_weight) {
					balancePartition(k);
				}
			}
		}

		void partitionImpl() final {
			for (const HypernodeID hn : _hg.nodes()) {
				PartitionID p = Randomize::getRandomInt(0,
						_config.initial_partitioning.k - 1);
				while (!assignHypernodeToPartition(hn, p))
					p = (p + 1) % _config.initial_partitioning.k;
			}
			balancePartitions();
		}

		using InitialPartitionerBase::_hg;
		using InitialPartitionerBase::_config;

	};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_RANDOMINITIALPARTITIONER_H_ */
