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
		configurePoolPartitioner();
	}

	~PoolInitialPartitioner() {
	}

private:

	void configurePoolPartitioner() {

		if (_config.initial_partitioning.pool_type.compare("full") == 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxnet);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxnet);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxnet);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_lp);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_bfs);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_random);
		} else if (_config.initial_partitioning.pool_type.compare("adaptive") == 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxnet);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxnet);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_lp);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_bfs);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_random);
		} else if (_config.initial_partitioning.pool_type.compare("greedy_full")
				== 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxnet);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxnet);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxnet);
		} else if (_config.initial_partitioning.pool_type.compare("greedy")
				== 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy);
		} else if (_config.initial_partitioning.pool_type.compare("greedy_maxpin")
				== 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxpin);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy_maxpin);
		} else if (_config.initial_partitioning.pool_type.compare("greedy_maxnet")
				== 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxnet);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxnet);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy_maxnet);
		} else if (_config.initial_partitioning.pool_type.compare("no_greedy")
				== 0) {
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_lp);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_bfs);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_random);
		} else if (_config.initial_partitioning.pool_type.compare("mix1") == 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxnet);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxnet);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_lp);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_bfs);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_random);
		} else if (_config.initial_partitioning.pool_type.compare("mix2") == 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_maxnet);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_lp);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_bfs);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_random);
		}  else if (_config.initial_partitioning.pool_type.compare("mix3") == 0) {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxnet);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_round_maxnet);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_lp);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_bfs);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_random);
		} else {
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_greedy);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxpin);
			_partitioner_pool.push_back(
					InitialPartitionerAlgorithm::rb_greedy_global_maxnet);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_lp);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_bfs);
			_partitioner_pool.push_back(InitialPartitionerAlgorithm::rb_random);
		}

	}

	void kwayPartitionImpl() final {

		HyperedgeWeight best_cut = max_cut;
		double best_imbalance = _config.initial_partitioning.epsilon;
		std::vector<PartitionID> best_partition(_hg.numNodes());
		std::string best_algorithm = "";
		int n = _partitioner_pool.size();
		if(_config.initial_partitioning.pool_type.compare("adaptive") == 0) {
			Randomize::shuffleVector(_partitioner_pool,n);
			n = 6;
		}

		for (int i = 0; i < n; i++) {
			InitialPartitionerAlgorithm algo = _partitioner_pool[i];
			std::cout << "Starting initial partitioner algorithm: "
					<< partition::toString(algo) << std::endl;
			std::unique_ptr<IInitialPartitioner> partitioner(
					InitialPartitioningFactory::getInstance().createObject(algo,
							_hg, _config));
			(*partitioner).partition(_config.initial_partitioning.k);
			HyperedgeWeight current_cut = metrics::hyperedgeCut(_hg);
			double current_imbalance = metrics::imbalance(_hg,
					_config.initial_partitioning.k);
			std::cout << "[Cut: " << current_cut << " - Imbalance: "
					<< current_imbalance << "]" << std::endl;
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

		std::cout << "Pool partitioner results: [min: " << best_cut
				<< ",  algo: " << best_algorithm << "]" << std::endl;
		InitialPartitionerBase::resetPartitioning(-1);
		for (HypernodeID hn : _hg.nodes()) {
			_hg.setNodePart(hn, best_partition[hn]);
		}

		_hg.initializeNumCutHyperedges();

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

	const HyperedgeWeight max_cut = std::numeric_limits<HyperedgeWeight>::max();

}
;

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_POOLINITIALPARTITIONER_H_ */
