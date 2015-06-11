/*
 * nLevelInitialPartitioner.h
 *
 *  Created on: 03.06.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_NLEVELINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_NLEVELINITIALPARTITIONER_H_

#include <vector>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "tools/RandomFunctions.h"
#include "partition/Factories.h"
#include "partition/Partitioner.h"

using defs::HypernodeWeight;

namespace partition {

class nLevelInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	nLevelInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~nLevelInitialPartitioner() {
	}

private:

	void prepareConfiguration() {
		_config.partition.initial_partitioner_path =
				"/home/theuer/Dokumente/hypergraph/release/src/partition/initial_partitioning/application/InitialPartitioningKaHyPar";
		_config.partition.graph_filename =
				_config.initial_partitioning.coarse_graph_filename;
		_config.partition.coarse_graph_filename = std::string("/tmp/PID_")
				+ std::to_string(getpid()) + std::string("_coarse_")
				+ _config.partition.graph_filename.substr(
						_config.partition.graph_filename.find_last_of("/") + 1);
		_config.partition.coarse_graph_partition_filename =
				_config.partition.coarse_graph_filename + ".part."
						+ std::to_string(_config.initial_partitioning.k);
		_config.partition.total_graph_weight = _hg.totalWeight();

		_config.coarsening.max_allowed_weight_multiplier = 3.5;
		_config.coarsening.contraction_limit =
				_config.coarsening.contraction_limit_multiplier
						* _config.initial_partitioning.k;
		_config.coarsening.hypernode_weight_fraction =
				_config.coarsening.max_allowed_weight_multiplier
						/ _config.coarsening.contraction_limit;

		_config.partition.max_part_weight =
				(1 + _config.initial_partitioning.epsilon)
						* ceil(
								_config.partition.total_graph_weight
										/ static_cast<double>(_config.initial_partitioning.k));
		_config.coarsening.max_allowed_node_weight =
				_config.coarsening.hypernode_weight_fraction
						* _config.partition.total_graph_weight;
		_config.fm_local_search.beta = log(_hg.numNodes());
		_config.fm_local_search.stopping_rule = RefinementStoppingRule::simple;
		_config.fm_local_search.num_repetitions = -1;
		_config.fm_local_search.max_number_of_fruitless_moves = 150;
		_config.fm_local_search.alpha = 8;
		_config.her_fm.stopping_rule = RefinementStoppingRule::simple;
		_config.her_fm.num_repetitions = 1;
		_config.her_fm.max_number_of_fruitless_moves = 10;
		_config.lp_refiner.max_number_iterations = 3;

		_config.partition.initial_partitioning_attempts = 1;
		_config.partition.global_search_iterations = 10;
		_config.partition.hyperedge_size_threshold = -1;

		_config.partition.mode = Mode::direct_kway;
	}

	void kwayPartitionImpl() final {
		InitialPartitionerBase::resetPartitioning(-1);
		_config.partition.initial_partitioner = InitialPartitioner::KaHyPar;

		if (_config.initial_partitioning.k > 2) {
			_config.coarsening.contraction_limit_multiplier = _hg.initialNumNodes()
					/ (static_cast<double>(_config.initial_partitioning.k) * _config.initial_partitioning.direct_nlevel_contraction_divider);
		}

		prepareConfiguration();
		Partitioner partitioner;
		partitioner.partition(_hg, _config);

		std::remove(_config.partition.coarse_graph_filename.c_str());
		std::remove(_config.partition.coarse_graph_partition_filename.c_str());
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

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_NLEVELINITIALPARTITIONER_H_ */
