/*
 * PaToHInitialPartitioner.h
 *
 *  Created on: 23.05.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_PATOHINITIALPARTITIONER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_PATOHINITIALPARTITIONER_H_

#include <vector>
#include <map>
#include <algorithm>

#include "lib/definitions.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"

using defs::Hypergraph;
using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;

namespace partition {

class PaToHInitialPartitioner: public IInitialPartitioner,
		private InitialPartitionerBase {

public:
	PaToHInitialPartitioner(Hypergraph& hypergraph, Configuration& config) :
			InitialPartitionerBase(hypergraph, config) {
	}

	~PaToHInitialPartitioner() {
	}

	void kwayPartitionImpl() final {

		double total_graph_weight = 0.0;
		for (HypernodeID hn : _hg.nodes()) {
			total_graph_weight += _hg.nodeWeight(hn);
		}


		std::string patoh_coarse_graph = "patoh_hypergraph.hgr";
		std::string partition_output =
				patoh_coarse_graph + ".part."
						+ std::to_string(_config.initial_partitioning.k);

		std::unordered_map<HypernodeID, HypernodeID> patoh_to_hg;
		std::unordered_map<HypernodeID, HypernodeID> hg_to_patoh;

		int i = 0;
		for (const HypernodeID hn : _hg.nodes()) {
			hg_to_patoh[hn] = i;
			patoh_to_hg[i] = hn;
			++i;
		}

		io::writeHypergraphForPaToHPartitioning(_hg, patoh_coarse_graph,
				hg_to_patoh);

		std::string initial_partitioner_call =
				"/software/patoh-Linux-x86_64/Linux-x86_64/patoh "
						+ patoh_coarse_graph + " "
						+ std::to_string(_config.partition.k) + " SD="
						+ std::to_string(_config.initial_partitioning.seed)
						+ " FI=" + std::to_string(_config.partition.epsilon)
						+ " PQ=Q"  // quality preset
						+ " UM=U"  // net-cut metric
						+ " WI=1"  // write partition info
						+ " BO=C"  // balance on cell weights
						+ " OD=3";

		std::cout << initial_partitioner_call << std::endl;
		std::system(initial_partitioner_call.c_str());

		std::vector<PartitionID> partitioning;
		io::readPartitionFile(partition_output, partitioning);
		for (HypernodeID hn : _hg.nodes()) {
			_hg.setNodePart(patoh_to_hg[hn], partitioning[hn]);
		}

		std::remove(patoh_coarse_graph.c_str());
		std::remove(partition_output.c_str());

	}

	void bisectionPartitionImpl() final {
		kwayPartitionImpl();
	}

protected:
	using InitialPartitionerBase::_hg;
	using InitialPartitionerBase::_config;

};

}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_PATOHINITIALPARTITIONER_H_ */
