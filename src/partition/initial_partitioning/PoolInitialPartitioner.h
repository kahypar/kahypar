/*
 * PoolInitialPartitioner.h
 *
 *  Created on: 03.06.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_POOLINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_POOLINITIALPARTITIONER_H_



#include <vector>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"
#include "partition/Factories.h"
#include "partition/Partitioner.h"

using defs::HypernodeWeight;
using partition::Configuration;

namespace partition {

class PoolInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	PoolInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config), _partitioner_pool() {
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy_global);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy_global_maxpin);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy_global_maxnet);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_lp);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_bfs);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_random);
	}

	~PoolInitialPartitioner() {
	}

private:


	void kwayPartitionImpl() final {

		HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
		std::vector<PartitionID> best_partition(_hg.numNodes());

		for(InitialPartitionerAlgorithm algo : _partitioner_pool) {
			std::cout << "Starting initial partitioner algorithm: " << partition::toString(algo) << std::endl;
			std::unique_ptr<IInitialPartitioner> partitioner(
					InitialPartitioningFactory::getInstance().createObject(
							algo, _hg, _config));
			(*partitioner).partition(_config.initial_partitioning.k);
			HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
			std::cout << "HyperedgeCut: " << current_cut << std::endl;
			if(current_cut < best_cut) {
				for(HypernodeID hn : _hg.nodes()) {
					best_partition[hn] = _hg.partID(hn);
				}
				best_cut = current_cut;
			}
		}

		InitialPartitionerBase::resetPartitioning(-1);
		for(HypernodeID i = 0; i < best_partition.size(); ++i) {
			_hg.setNodePart(i,best_partition[i]);
		}

	}

	void bisectionPartitionImpl() final {
		PartitionID k = _config.initial_partitioning.k;
		_config.initial_partitioning.k = 2;
		kwayPartitionImpl();
		_config.initial_partitioning.k = k;
	}

	std::vector<InitialPartitionerAlgorithm> _partitioner_pool;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

}
;

}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POOLINITIALPARTITIONER_H_ */
