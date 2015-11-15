/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_CONFIGURATION_H_
#define SRC_PARTITION_CONFIGURATION_H_

#include <array>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "lib/definitions.h"

using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HypernodeWeightVector;

namespace partition {
enum class Mode
	: std::uint8_t {
		recursive_bisection, direct_kway
};

enum class InitialPartitioningTechnique
	: std::uint8_t {
		multilevel, flat
};

enum class InitialPartitioner
	: std::uint8_t {
		hMetis, PaToH, KaHyPar
};

enum class CoarseningAlgorithm
	: std::uint8_t {
		heavy_full, heavy_partial, heavy_lazy, hyperedge, do_nothing
};

enum class RefinementAlgorithm
	: std::uint8_t {
		twoway_fm,
	kway_fm,
	kway_fm_maxgain,
	hyperedge,
	label_propagation,
	do_nothing
};

enum class InitialPartitionerAlgorithm
	: std::uint8_t {
		greedy_sequential,
	greedy_global,
	greedy_round,
	greedy_sequential_maxpin,
	greedy_global_maxpin,
	greedy_round_maxpin,
	greedy_sequential_maxnet,
	greedy_global_maxnet,
	greedy_round_maxnet,
	greedy_test,
	bfs,
	random,
	lp,
	pool

};

enum class RefinementStoppingRule
	: std::uint8_t {
		simple, adaptive1, adaptive2
};

static std::string toString(const Mode& mode) {
	switch (mode) {
	case Mode::recursive_bisection:
		return std::string("rb");
	case Mode::direct_kway:
		return std::string("direct");
	}
	return std::string("UNDEFINED");
}

static std::string toString(const InitialPartitioningTechnique& technique) {
	switch (technique) {
	case InitialPartitioningTechnique::flat:
		return std::string("flat");
	case InitialPartitioningTechnique::multilevel:
		return std::string("multilevel");
	}
	return std::string("UNDEFINED");
}

static std::string toString(const InitialPartitioner& algo) {
	switch (algo) {
	case InitialPartitioner::hMetis:
		return std::string("hMetis");
	case InitialPartitioner::PaToH:
		return std::string("PaToH");
	case InitialPartitioner::KaHyPar:
		return std::string("KaHyPar");
	}
	return std::string("UNDEFINED");
}

static std::string toString(const CoarseningAlgorithm& algo) {
	switch (algo) {
	case CoarseningAlgorithm::heavy_full:
		return std::string("heavy_full");
	case CoarseningAlgorithm::heavy_partial:
		return std::string("heavy_partial");
	case CoarseningAlgorithm::heavy_lazy:
		return std::string("heavy_lazy");
	case CoarseningAlgorithm::hyperedge:
		return std::string("hyperedge");
	case CoarseningAlgorithm::do_nothing:
		return std::string("do_nothing");
	}
	return std::string("UNDEFINED");
}

static std::string toString(const RefinementAlgorithm& algo) {
	switch (algo) {
	case RefinementAlgorithm::twoway_fm:
		return std::string("twoway_fm");
	case RefinementAlgorithm::kway_fm:
		return std::string("kway_fm");
	case RefinementAlgorithm::kway_fm_maxgain:
		return std::string("kway_fm_maxgain");
	case RefinementAlgorithm::hyperedge:
		return std::string("hyperedge");
	case RefinementAlgorithm::label_propagation:
		return std::string("label_propagation");
	case RefinementAlgorithm::do_nothing:
		return std::string("do_nothing");
	}
	return std::string("UNDEFINED");
}

static std::string toString(const InitialPartitionerAlgorithm& algo) {
	switch (algo) {
	case InitialPartitionerAlgorithm::greedy_sequential:
		return std::string("greedy_sequential");
	case InitialPartitionerAlgorithm::greedy_global:
		return std::string("greedy_global");
	case InitialPartitionerAlgorithm::greedy_round:
		return std::string("greedy_round");
	case InitialPartitionerAlgorithm::greedy_sequential_maxpin:
		return std::string("greedy_maxpin");
	case InitialPartitionerAlgorithm::greedy_global_maxpin:
		return std::string("greedy_global_maxpin");
	case InitialPartitionerAlgorithm::greedy_round_maxpin:
		return std::string("greedy_round_maxpin");
	case InitialPartitionerAlgorithm::greedy_sequential_maxnet:
		return std::string("greedy_maxnet");
	case InitialPartitionerAlgorithm::greedy_global_maxnet:
		return std::string("greedy_global_maxnet");
	case InitialPartitionerAlgorithm::greedy_round_maxnet:
		return std::string("greedy_round_maxnet");
	case InitialPartitionerAlgorithm::bfs:
		return std::string("bfs");
	case InitialPartitionerAlgorithm::random:
		return std::string("random");
	case InitialPartitionerAlgorithm::lp:
		return std::string("lp");
	case InitialPartitionerAlgorithm::greedy_test:
		return std::string("greedy_test");
	case InitialPartitionerAlgorithm::pool:
		return std::string("pool");
	}
	return std::string("UNDEFINED");
}

static std::string toString(const RefinementStoppingRule& algo) {
	switch (algo) {
	case RefinementStoppingRule::simple:
		return std::string("simple");
	case RefinementStoppingRule::adaptive1:
		return std::string("adaptive1");
	case RefinementStoppingRule::adaptive2:
		return std::string("adaptive2");
	}
	return std::string("UNDEFINED");
}

struct Configuration {
	struct CoarseningParameters {
		CoarseningParameters() :
				max_allowed_node_weight(0), contraction_limit(0), contraction_limit_multiplier(
						0), hypernode_weight_fraction(0.0), max_allowed_weight_multiplier(
						0.0) {
		}

		HypernodeWeight max_allowed_node_weight;
		HypernodeID contraction_limit;
		HypernodeID contraction_limit_multiplier;
		double hypernode_weight_fraction;
		double max_allowed_weight_multiplier;
	};

	struct InitialPartitioningParameters {
		InitialPartitioningParameters() :
				k(2), epsilon(0.05), init_technique(
						InitialPartitioningTechnique::flat), init_mode(
						Mode::recursive_bisection), algo(
						InitialPartitionerAlgorithm::pool), upper_allowed_partition_weight(), perfect_balance_partition_weight(), nruns(
						20), unassigned_part(1), local_search_repetitions(1), init_alpha(
						1.0), seed(1), pool_type(1975), rollback(false), refinement(
						true) {
		}

		PartitionID k;
		double epsilon;
		InitialPartitioningTechnique init_technique;
		Mode init_mode;
		InitialPartitionerAlgorithm algo;
		HypernodeWeightVector upper_allowed_partition_weight;
		HypernodeWeightVector perfect_balance_partition_weight;
		int nruns;
		PartitionID unassigned_part;
		int local_search_repetitions;
		double init_alpha;
		int seed;
		unsigned int pool_type;
		bool rollback;
		bool refinement;
	};

	struct PartitioningParameters {
		PartitioningParameters() :
				k(2), rb_lower_k(0), rb_upper_k(1), seed(0), initial_partitioning_attempts(
						1), global_search_iterations(1), current_v_cycle(0), epsilon(
						1.0), hmetis_ub_factor(-1.0), perfect_balance_part_weights(
						{ std::numeric_limits<HypernodeWeight>::max(),
								std::numeric_limits<HypernodeWeight>::max() }), max_part_weights(
						{ std::numeric_limits<HypernodeWeight>::max(),
								std::numeric_limits<HypernodeWeight>::max() }), total_graph_weight(
						0), hyperedge_size_threshold(-1), remove_hes_that_always_will_be_cut(
						false), initial_parallel_he_removal(false), verbose_output(
						false), collect_stats(false), mode(Mode::direct_kway), coarsening_algorithm(
						CoarseningAlgorithm::heavy_lazy), initial_partitioner(
						InitialPartitioner::hMetis), refinement_algorithm(
						RefinementAlgorithm::kway_fm), graph_filename(), graph_partition_filename(), coarse_graph_filename(), coarse_graph_partition_filename(), initial_partitioner_path() {
		}

		PartitionID k;
		PartitionID rb_lower_k;
		PartitionID rb_upper_k;
		int seed;
		int initial_partitioning_attempts;
		int global_search_iterations;
		int current_v_cycle;
		double epsilon;
		double hmetis_ub_factor;
		std::array<HypernodeWeight, 2> perfect_balance_part_weights;
		std::array<HypernodeWeight, 2> max_part_weights;
		HypernodeWeight total_graph_weight;
		HyperedgeID hyperedge_size_threshold;
		bool remove_hes_that_always_will_be_cut;
		bool initial_parallel_he_removal;
		bool verbose_output;
		bool collect_stats;
		Mode mode;
		CoarseningAlgorithm coarsening_algorithm;
		InitialPartitioner initial_partitioner;
		RefinementAlgorithm refinement_algorithm;
		std::string graph_filename;
		std::string graph_partition_filename;
		std::string coarse_graph_filename;
		std::string coarse_graph_partition_filename;
		std::string initial_partitioner_path;
	};

	struct FMParameters {
		FMParameters() :
				max_number_of_fruitless_moves(50), num_repetitions(1), alpha(4), beta(
						0.0), stopping_rule(RefinementStoppingRule::simple) {
		}

		int max_number_of_fruitless_moves;
		int num_repetitions;
		double alpha;
		double beta;
		RefinementStoppingRule stopping_rule;
	};

	struct HERFMParameters {
		HERFMParameters() :
				max_number_of_fruitless_moves(10), num_repetitions(1), stopping_rule(
						RefinementStoppingRule::simple) {
		}

		int max_number_of_fruitless_moves;
		int num_repetitions;
		RefinementStoppingRule stopping_rule;
	};

	struct LPRefinementParameters {
		LPRefinementParameters() :
				max_number_iterations(3) {
		}

		int max_number_iterations;
	};

	PartitioningParameters partition;
	CoarseningParameters coarsening;
	InitialPartitioningParameters initial_partitioning;
	FMParameters fm_local_search;
	HERFMParameters her_fm;
	LPRefinementParameters lp_refiner;

	Configuration() :
			partition(), coarsening(), initial_partitioning(), fm_local_search(), her_fm(), lp_refiner() {
	}
};

inline std::string toString(const Configuration& config) {
	std::ostringstream oss;
	oss << std::left;
	oss << "Partitioning Parameters:" << std::endl;
	oss << std::setw(35) << "  Hypergraph: " << config.partition.graph_filename
			<< std::endl;
	oss << std::setw(35) << "  Partition File: "
			<< config.partition.graph_partition_filename << std::endl;
	oss << std::setw(35) << "  Coarsened Hypergraph: "
			<< config.partition.coarse_graph_filename << std::endl;
	oss << std::setw(35) << "  Coarsened Partition File: "
			<< config.partition.coarse_graph_partition_filename << std::endl;
	oss << std::setw(35) << "  k: " << config.partition.k << std::endl;
	oss << std::setw(35) << "  epsilon: " << config.partition.epsilon
			<< std::endl;
	oss << std::setw(35) << "  seed: " << config.partition.seed << std::endl;
	oss << std::setw(35) << "  # global search iterations: "
			<< config.partition.global_search_iterations << std::endl;
	oss << std::setw(35) << "  hyperedge size threshold: "
			<< config.partition.hyperedge_size_threshold << std::endl;
	oss << std::setw(35) << "  initially remove parallel HEs: "
			<< std::boolalpha << config.partition.initial_parallel_he_removal
			<< std::endl;
	oss << std::setw(35) << "  remove HEs that always will be cut HEs: "
			<< std::boolalpha
			<< config.partition.remove_hes_that_always_will_be_cut << std::endl;
	oss << std::setw(35) << "  total_graph_weight: "
			<< config.partition.total_graph_weight << std::endl;
	oss << std::setw(35) << "  L_opt0: "
			<< config.partition.perfect_balance_part_weights[0] << std::endl;
	oss << std::setw(35) << "  L_opt1: "
			<< config.partition.perfect_balance_part_weights[1] << std::endl;
	oss << std::setw(35) << "  L_max0: " << config.partition.max_part_weights[0]
			<< std::endl;
	oss << std::setw(35) << "  L_max1: " << config.partition.max_part_weights[1]
			<< std::endl;
	oss << std::setw(35) << " Mode: " << toString(config.partition.mode)
			<< std::endl;
	oss << std::setw(35) << "  Coarsening Algorithm: "
			<< toString(config.partition.coarsening_algorithm) << std::endl;
	oss << std::setw(35) << "  Initial Partitioner: "
			<< toString(config.partition.initial_partitioner) << std::endl;
	oss << std::setw(35) << "  Initial Partitioning Mode: "
			<< toString(config.initial_partitioning.init_mode) << " "
			<< toString(config.initial_partitioning.init_technique)
			<< std::endl;
	oss << std::setw(35) << "  Initial Partitioning Algorithm: "
			<< toString(config.initial_partitioning.algo) << std::endl;
	oss << std::setw(35) << "  Refinement Algorithm: "
			<< toString(config.partition.refinement_algorithm) << std::endl;
	oss << "Coarsening Parameters:" << std::endl;
	oss << std::setw(35) << "  max-allowed-weight-multiplier: "
			<< config.coarsening.max_allowed_weight_multiplier << std::endl;
	oss << std::setw(35) << "  contraction-limit-multiplier: "
			<< config.coarsening.contraction_limit_multiplier << std::endl;
	oss << std::setw(35) << "  hypernode weight fraction: "
			<< config.coarsening.hypernode_weight_fraction << std::endl;
	oss << std::setw(35) << "  max. allowed hypernode weight: "
			<< config.coarsening.max_allowed_node_weight << std::endl;
	oss << std::setw(35) << "  contraction limit: "
			<< config.coarsening.contraction_limit << std::endl;
	oss << "Initial Partitioning Parameters:" << std::endl;
	oss << std::setw(35) << "  hmetis_ub_factor: "
			<< config.partition.hmetis_ub_factor << std::endl;
	oss << std::setw(35) << "  # initial partitionings: "
			<< config.partition.initial_partitioning_attempts << std::endl;
	oss << std::setw(35) << "  initial partitioner path: "
			<< config.partition.initial_partitioner_path << std::endl;
	oss << "Refinement Parameters:" << std::endl;
	if (config.partition.refinement_algorithm == RefinementAlgorithm::twoway_fm
			|| config.partition.refinement_algorithm
					== RefinementAlgorithm::kway_fm
			|| config.partition.refinement_algorithm
					== RefinementAlgorithm::kway_fm_maxgain) {
		oss << std::setw(35) << "  stopping rule: "
				<< toString(config.fm_local_search.stopping_rule) << std::endl;
		oss << std::setw(35) << "  max. # repetitions: "
				<< config.fm_local_search.num_repetitions << std::endl;
		oss << std::setw(35) << "  max. # fruitless moves: "
				<< config.fm_local_search.max_number_of_fruitless_moves
				<< std::endl;
		oss << std::setw(35) << "  random walk stop alpha: "
				<< config.fm_local_search.alpha << std::endl;
		oss << std::setw(35) << "  random walk stop beta : "
				<< config.fm_local_search.beta << std::endl;
	}
	if (config.partition.refinement_algorithm
			== RefinementAlgorithm::hyperedge) {
		oss << std::setw(35) << "  stopping rule: "
				<< toString(config.her_fm.stopping_rule) << std::endl;
		oss << std::setw(35) << "  max. # repetitions: "
				<< config.her_fm.num_repetitions << std::endl;
		oss << std::setw(35) << "  max. # fruitless moves: "
				<< config.her_fm.max_number_of_fruitless_moves << std::endl;
	}
	if (config.partition.refinement_algorithm
			== RefinementAlgorithm::label_propagation) {
		oss << std::setw(35) << "  max. # iterations: "
				<< config.lp_refiner.max_number_iterations << std::endl;
	}
	return oss.str();
}
}  // namespace partition
#endif  // SRC_PARTITION_CONFIGURATION_H_
