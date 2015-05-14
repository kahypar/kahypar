/*
 * HMetisInitialPartitioner.h
 *
 *  Created on: May 11, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_HMETISINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_HMETISINITIALPARTITIONER_H_

#define EMPTY_LABEL -1

#include <vector>
#include <algorithm>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"

using defs::Hypergraph;
using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;

namespace partition {

class HMetisInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	HMetisInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~HMetisInitialPartitioner() {
	}

	void kwayPartitionImpl() final {

		double total_graph_weight = 0.0;
		for (HypernodeID hn : _hg.nodes()) {
			total_graph_weight += _hg.nodeWeight(hn);
		}

		_config.partition.initial_partitioner_path =
				"/software/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1";

		double exp = 1.0 / log2(_config.initial_partitioning.k);
		_config.partition.hmetis_ub_factor =
				std::max(50.0
						* (2
								* pow(
										(1
												+ _config.initial_partitioning.epsilon),
										exp)
								* pow(
										ceil(
												static_cast<double>(total_graph_weight)
														/ _config.initial_partitioning.k)
												/ total_graph_weight, exp) - 1), 0.1);

		std::string partition_output =
				_config.initial_partitioning.coarse_graph_filename + ".part."
				+ std::to_string(_config.initial_partitioning.k);

		std::string initial_partitioner_call =
				_config.partition.initial_partitioner_path + " "
						+ _config.initial_partitioning.coarse_graph_filename
						+ " " + std::to_string(_config.initial_partitioning.k)
						+ " -seed="
						+ std::to_string(_config.initial_partitioning.seed)
						+ " -ufactor="
						+ std::to_string(_config.partition.hmetis_ub_factor)
						+ partition_output;

		std::cout << initial_partitioner_call << std::endl;
	    std::system(initial_partitioner_call.c_str());

	    std::vector<PartitionID> partitioning;
	    io::readPartitionFile(partition_output, partitioning);
	    for(HypernodeID hn : _hg.nodes()) {
	    	_hg.setNodePart(hn,partitioning[hn]);
	    }

	}

	void bisectionPartitionImpl() final {
		kwayPartitionImpl();
	}

protected:
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_HMETISINITIALPARTITIONER_H_ */
