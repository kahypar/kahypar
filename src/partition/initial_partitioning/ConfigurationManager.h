/*
 * ConfigurationManager.h
 *
 *  Created on: 11.06.2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_CONFIGURATIONMANAGER_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_CONFIGURATIONMANAGER_H_


#include "lib/definitions.h"
#include "partition/Configuration.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;

namespace partition {

class ConfigurationManager  {

public:
	ConfigurationManager() {
	}

	~ConfigurationManager() {
	}

	static inline void setDefaults(Configuration& config) {
		//InitialPartitioningParameters
		config.initial_partitioning.k = 2;
		config.initial_partitioning.epsilon = 0.03;
		config.initial_partitioning.algorithm = "random";
		config.initial_partitioning.pool_type = "basic";
		config.initial_partitioning.algo = InitialPartitionerAlgorithm::random;
		config.initial_partitioning.mode = "direct";
		config.initial_partitioning.seed = -1;
		config.initial_partitioning.min_ils_iterations = 400;
		config.initial_partitioning.max_stable_net_removals = 100;
		config.initial_partitioning.pool_type = "basic";
		config.initial_partitioning.nruns = 1;
		config.initial_partitioning.direct_nlevel_contraction_divider = 2;
		config.initial_partitioning.unassigned_part = 1;
		config.initial_partitioning.alpha = 1.0;
		config.initial_partitioning.beta = 1.0;
		config.initial_partitioning.rollback = true;
		config.initial_partitioning.refinement = true;
		config.initial_partitioning.erase_components = false;
		config.initial_partitioning.balance = false;
		config.initial_partitioning.stats = true;
		config.initial_partitioning.styles = true;

		//Partitioner-Parameters
		config.partition.k = 2;
		config.partition.epsilon = 0.03;
		config.partition.initial_partitioner_path =
				"/home/theuer/Dokumente/hypergraph/release/src/partition/initial_partitioning/application/InitialPartitioningKaHyPar";
		config.partition.initial_partitioner = InitialPartitioner::KaHyPar;
		config.partition.initial_partitioning_attempts = 1;
		config.partition.global_search_iterations = 10;
		config.partition.hyperedge_size_threshold = -1;
		config.partition.mode = Mode::direct_kway;


		//Coarsening-Parameters
		config.coarsening.contraction_limit_multiplier = 150;
		config.partition.coarsening_algorithm = CoarseningAlgorithm::heavy_partial;
		config.coarsening.max_allowed_weight_multiplier = 2.5;

		//Refinement-Parameters
		config.partition.global_search_iterations = 0;
		config.partition.refinement_algorithm =
				RefinementAlgorithm::label_propagation;
		config.fm_local_search.num_repetitions = -1;
		config.fm_local_search.max_number_of_fruitless_moves = 20;
		config.fm_local_search.stopping_rule = RefinementStoppingRule::simple;
		config.fm_local_search.alpha = 8;
		config.her_fm.num_repetitions = -1;
		config.her_fm.max_number_of_fruitless_moves = 10;
		config.lp_refiner.max_number_iterations = 3;
	}

	static inline void setHypergraphDependingParameters(Configuration& config, const Hypergraph& hg) {
		//Partitioner-Parameters
		config.partition.total_graph_weight = hg.totalWeight();
		config.partition.graph_filename =
				config.initial_partitioning.coarse_graph_filename;
		config.partition.coarse_graph_filename = std::string("/tmp/PID_")
				+ std::to_string(getpid()) + std::string("_coarse_")
				+ config.partition.graph_filename.substr(
						config.partition.graph_filename.find_last_of("/") + 1);
		config.partition.coarse_graph_partition_filename =
				config.partition.coarse_graph_filename + ".part."
						+ std::to_string(config.initial_partitioning.k);
		config.partition.total_graph_weight = hg.totalWeight();
		config.partition.max_part_weight =
				(1 + config.initial_partitioning.epsilon)
						* ceil(
								config.partition.total_graph_weight
										/ static_cast<double>(config.initial_partitioning.k));
		config.partition.total_graph_weight = hg.totalWeight();

		//Coarsening-Parameters
		config.coarsening.contraction_limit =
				config.coarsening.contraction_limit_multiplier
						* config.initial_partitioning.k;
		config.coarsening.hypernode_weight_fraction =
				config.coarsening.max_allowed_weight_multiplier
						/ config.coarsening.contraction_limit;
		config.coarsening.max_allowed_node_weight =
				config.coarsening.hypernode_weight_fraction
						* config.partition.total_graph_weight;



		//Refinement-Parameters
		config.fm_local_search.beta = log(hg.numNodes());
	}

	static inline Configuration copyConfigAndSetValues(const Configuration& config, std::function<void(Configuration&)> f) {
		Configuration new_config(config);
		f(new_config);
		return new_config;
	}


};

}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_CONFIGURATIONMANAGER_H_ */
