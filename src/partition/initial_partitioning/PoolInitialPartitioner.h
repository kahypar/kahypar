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
		//mix3 == 011110110111 == 1975
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::greedy_global);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::greedy_round);
		_partitioner_pool.push_back(
				InitialPartitionerAlgorithm::greedy_sequential);
		_partitioner_pool.push_back(
				InitialPartitionerAlgorithm::greedy_global_maxpin);
		_partitioner_pool.push_back(
				InitialPartitionerAlgorithm::greedy_round_maxpin);
		_partitioner_pool.push_back(
				InitialPartitionerAlgorithm::greedy_sequential_maxpin);
		_partitioner_pool.push_back(
				InitialPartitionerAlgorithm::greedy_global_maxnet);
		_partitioner_pool.push_back(
				InitialPartitionerAlgorithm::greedy_round_maxnet);
		_partitioner_pool.push_back(
				InitialPartitionerAlgorithm::greedy_sequential_maxnet);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::lp);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::bfs);
		_partitioner_pool.push_back(InitialPartitionerAlgorithm::random);
	}

	~PoolInitialPartitioner() {
	}

private:

	void initialPartition() final {

		HyperedgeWeight best_cut = max_cut;
		double best_imbalance = _config.initial_partitioning.epsilon;
		std::vector<PartitionID> best_partition(_hg.numNodes());
		std::string best_algorithm = "";
		unsigned int n = _partitioner_pool.size() - 1;
		for (unsigned int i = 0; i <= n; i++) {
			if (!((_config.initial_partitioning.pool_type >> (n - i)) & 1)) {
				continue;
			}
			InitialPartitionerAlgorithm algo = _partitioner_pool[i];
			LOG("Calling initial partitioning algorithm: "
					<< partition::toString(algo));
			std::unique_ptr<IInitialPartitioner> partitioner(
					InitialPartitioningFactory::getInstance().createObject(algo,
							_hg, _config));
			(*partitioner).partition(_hg, _config);
			HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
			double current_imbalance = metrics::imbalance(_hg, _config);
			LOG("[Cut: " << current_cut << " - Imbalance: "
					<< current_imbalance << "]");
			if (current_cut <= best_cut) {
				bool apply_best_partition = true;
				if (best_cut != max_cut) {
					if (current_imbalance
							> _config.initial_partitioning.epsilon) {
						if (current_imbalance > best_imbalance) {
							apply_best_partition = false;
						}
					}
				}
				if (apply_best_partition) {
					for (HypernodeID hn : _hg.nodes()) {
						best_partition[hn] = _hg.partID(hn);
					}
					best_cut = current_cut;
					best_imbalance = current_imbalance;
					best_algorithm = partition::toString(algo);
				}
			}
			std::cout << "-----------------------------------------"
					<< std::endl;
		}

		LOG("Pool partitioner results: [min: " << best_cut
				<< ",  algo: " << best_algorithm << "]");
		PartitionID unassigned_part =
				_config.initial_partitioning.unassigned_part;
		_config.initial_partitioning.unassigned_part = -1;
		InitialPartitionerBase::resetPartitioning();
		_config.initial_partitioning.unassigned_part = unassigned_part;
		for (HypernodeID hn : _hg.nodes()) {
			_hg.setNodePart(hn, best_partition[hn]);
		}

		_hg.initializeNumCutHyperedges();

		ASSERT([&]() {
			for(HypernodeID hn : _hg.nodes()) {
				if(_hg.partID(hn) == -1) {
					return false;
				}
			}
			return true;
		}(), "There are unassigned hypernodes!");

		_config.initial_partitioning.nruns = 1;

	}

	std::vector<InitialPartitionerAlgorithm> _partitioner_pool;
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

	const HyperedgeWeight max_cut = std::numeric_limits<HyperedgeWeight>::max();

}
;

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POOLINITIALPARTITIONER_H_ */
