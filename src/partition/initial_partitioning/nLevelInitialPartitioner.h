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
#include "partition/initial_partitioning/ConfigurationManager.h"

using defs::HypernodeWeight;
using partition::ConfigurationManager;

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

	void kwayPartitionImpl() final {
		InitialPartitionerBase::resetPartitioning(-1);
		_config.partition.initial_partitioner = InitialPartitioner::KaHyPar;

		Configuration nlevel_config =
				ConfigurationManager::copyConfigAndSetValues(_config,
						[&](Configuration& config) {
							if (_config.initial_partitioning.k > 2) {
								config.coarsening.contraction_limit_multiplier = _hg.initialNumNodes()
								/ (static_cast<double>(_config.initial_partitioning.k) * _config.initial_partitioning.direct_nlevel_contraction_divider);
							}
							config.fm_local_search.max_number_of_fruitless_moves = 20;
							ConfigurationManager::setHypergraphDependingParameters(config,_hg);
						});

		Partitioner partitioner;
		partitioner.partition(_hg, nlevel_config);

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
