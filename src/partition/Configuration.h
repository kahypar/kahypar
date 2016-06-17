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
#include "partition/ConfigurationEnumClasses.h"

using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HypernodeWeightVector;
namespace partition {
struct Configuration {
  struct CoarseningParameters {
    CoarseningParameters() :
      max_allowed_node_weight(0),
      contraction_limit(0),
      contraction_limit_multiplier(0),
      hypernode_weight_fraction(0.0),
      max_allowed_weight_multiplier(0.0) { }

    HypernodeWeight max_allowed_node_weight;
    HypernodeID contraction_limit;
    HypernodeID contraction_limit_multiplier;
    double hypernode_weight_fraction;
    double max_allowed_weight_multiplier;
  };

  struct InitialPartitioningParameters {
    InitialPartitioningParameters() :
      k(2),
      epsilon(0.05),
      technique(InitialPartitioningTechnique::flat),
      mode(Mode::recursive_bisection),
      coarsening_algorithm(CoarseningAlgorithm::heavy_lazy),
      contraction_limit_multiplier(150),
      max_allowed_weight_multiplier(2.5),
      algo(InitialPartitionerAlgorithm::pool),
      refinement_algorithm(RefinementAlgorithm::twoway_fm),
      upper_allowed_partition_weight(),
      perfect_balance_partition_weight(),
      nruns(20),
      unassigned_part(1),
      local_search_repetitions(std::numeric_limits<int>::max()),
      init_alpha(1.0),
      seed(1),
      pool_type(1975),
      lp_max_iteration(100),
      lp_assign_vertex_to_part(5),
      refinement(true) { }

    PartitionID k;
    double epsilon;
    InitialPartitioningTechnique technique;
    Mode mode;
    CoarseningAlgorithm coarsening_algorithm;
    HypernodeID contraction_limit_multiplier;
    double max_allowed_weight_multiplier;
    InitialPartitionerAlgorithm algo;
    RefinementAlgorithm refinement_algorithm;
    HypernodeWeightVector upper_allowed_partition_weight;
    HypernodeWeightVector perfect_balance_partition_weight;
    int nruns;
    PartitionID unassigned_part;
    int local_search_repetitions;
    // Is used to get a tighter balance constraint for initial partitioning.
    // Before initial partitioning epsilon is set to init_alpha*epsilon.
    double init_alpha;
    int seed;
    // If pool initial partitioner is used, the first 12 bits of this number decides
    // which algorithms are used.
    unsigned int pool_type;
    // Maximum iterations of the Label Propagation IP over all hypernodes
    int lp_max_iteration;
    // Amojunt of hypernodes which are assigned around each start vertex (LP)
    int lp_assign_vertex_to_part;
    bool refinement;
  };

  struct PartitioningParameters {
    PartitioningParameters() :
      objective(Objective::cut),
      k(2),
      rb_lower_k(0),
      rb_upper_k(1),
      seed(0),
      initial_partitioning_attempts(1),
      global_search_iterations(1),
      current_v_cycle(0),
      num_local_search_repetitions(std::numeric_limits<int>::max()),
      epsilon(1.0),
      hmetis_ub_factor(-1.0),
      perfect_balance_part_weights({
        std::numeric_limits<HypernodeWeight>::max(),
        std::numeric_limits<HypernodeWeight>::max()
      }),
      max_part_weights({ std::numeric_limits<HypernodeWeight>::max(),
                         std::numeric_limits<HypernodeWeight>::max() }),
      total_graph_weight(0),
      hyperedge_size_threshold(-1),
      remove_hes_that_always_will_be_cut(false),
      initial_parallel_he_removal(false),
      verbose_output(false),
      collect_stats(false),
      mode(Mode::direct_kway),
      coarsening_algorithm(CoarseningAlgorithm::heavy_lazy),
      initial_partitioner(InitialPartitioner::hMetis),
      refinement_algorithm(RefinementAlgorithm::kway_fm),
      graph_filename(),
      graph_partition_filename(),
      coarse_graph_filename(),
      coarse_graph_partition_filename(),
      initial_partitioner_path() { }

    Objective objective;
    PartitionID k;
    PartitionID rb_lower_k;
    PartitionID rb_upper_k;
    int seed;
    int initial_partitioning_attempts;
    int global_search_iterations;
    int current_v_cycle;
    int num_local_search_repetitions;
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
      max_number_of_fruitless_moves(50),
      alpha(4),
      beta(0.0),
      stopping_rule(RefinementStoppingRule::simple),
      global_rebalancing(GlobalRebalancingMode::off) { }

    int max_number_of_fruitless_moves;
    double alpha;
    double beta;
    RefinementStoppingRule stopping_rule;
    GlobalRebalancingMode global_rebalancing;
  };

  struct HERFMParameters {
    HERFMParameters() :
      max_number_of_fruitless_moves(10),
      stopping_rule(RefinementStoppingRule::simple) { }

    int max_number_of_fruitless_moves;
    RefinementStoppingRule stopping_rule;
  };

  struct LPRefinementParameters {
    LPRefinementParameters() :
      max_number_iterations(3) { }

    int max_number_iterations;
  };

  PartitioningParameters partition;
  CoarseningParameters coarsening;
  InitialPartitioningParameters initial_partitioning;
  FMParameters fm_local_search;
  HERFMParameters her_fm;
  LPRefinementParameters lp_refiner;

  Configuration() :
    partition(),
    coarsening(),
    initial_partitioning(),
    fm_local_search(),
    her_fm(),
    lp_refiner() { }
};

inline std::string toString(const Configuration& config) {
  std::ostringstream oss;
  oss << std::left;
  oss << "Partitioning Parameters:" << std::endl;
  oss << std::setw(37) << "  Hypergraph: " << config.partition.graph_filename
  << std::endl;
  oss << std::setw(37) << "  Partition File: "
  << config.partition.graph_partition_filename << std::endl;
  oss << std::setw(37) << "  Coarsened Hypergraph: "
  << config.partition.coarse_graph_filename << std::endl;
  oss << std::setw(37) << "  Coarsened Partition File: "
  << config.partition.coarse_graph_partition_filename << std::endl;
  oss << std::setw(37) << "  Objective: " << toString(config.partition.objective) << std::endl;
  oss << std::setw(37) << "  k: " << config.partition.k << std::endl;
  oss << std::setw(37) << "  epsilon: " << config.partition.epsilon
  << std::endl;
  oss << std::setw(37) << "  seed: " << config.partition.seed << std::endl;
  oss << std::setw(37) << "  # global search iterations: "
  << config.partition.global_search_iterations << std::endl;
  oss << std::setw(37) << "  hyperedge size threshold: "
  << config.partition.hyperedge_size_threshold << std::endl;
  oss << std::setw(37) << "  initially remove parallel HEs: "
  << std::boolalpha << config.partition.initial_parallel_he_removal
  << std::endl;
  oss << std::setw(37) << "  remove HEs that always will be cut: "
  << std::boolalpha
  << config.partition.remove_hes_that_always_will_be_cut << std::endl;
  oss << std::setw(37) << "  total_graph_weight: "
  << config.partition.total_graph_weight << std::endl;
  oss << std::setw(37) << "  L_opt0: "
  << config.partition.perfect_balance_part_weights[0] << std::endl;
  oss << std::setw(37) << "  L_opt1: "
  << config.partition.perfect_balance_part_weights[1] << std::endl;
  oss << std::setw(37) << "  L_max0: " << config.partition.max_part_weights[0]
  << std::endl;
  oss << std::setw(37) << "  L_max1: " << config.partition.max_part_weights[1]
  << std::endl;
  oss << std::setw(37) << " Mode: " << toString(config.partition.mode)
  << std::endl;
  oss << std::setw(37) << "  Coarsening Algorithm: "
  << toString(config.partition.coarsening_algorithm) << std::endl;
  oss << std::setw(37) << "    max-allowed-weight-multiplier: "
  << config.coarsening.max_allowed_weight_multiplier << std::endl;
  oss << std::setw(37) << "    contraction-limit-multiplier: "
  << config.coarsening.contraction_limit_multiplier << std::endl;
  oss << std::setw(37) << "    hypernode weight fraction: "
  << config.coarsening.hypernode_weight_fraction << std::endl;
  oss << std::setw(37) << "    max. allowed hypernode weight: "
  << config.coarsening.max_allowed_node_weight << std::endl;
  oss << std::setw(37) << "    contraction limit: "
  << config.coarsening.contraction_limit << std::endl;
  oss << std::setw(37) << "  Initial Partitioner (IP): "
  << toString(config.partition.initial_partitioner) << std::endl;
  oss << std::setw(37) << "    hmetis_ub_factor: "
  << config.partition.hmetis_ub_factor << std::endl;
  oss << std::setw(37) << "    # initial partitionings: "
  << config.partition.initial_partitioning_attempts << std::endl;
  oss << std::setw(37) << "    initial partitioner path: "
  << config.partition.initial_partitioner_path << std::endl;
  oss << std::setw(37) << "    IP Mode: "
  << toString(config.initial_partitioning.mode) << std::endl;
  oss << std::setw(37) << "    IP Technique: "
  << toString(config.initial_partitioning.technique) << std::endl;
  oss << std::setw(37) << "    IP Coarsening Algorithm: "
  << toString(config.initial_partitioning.coarsening_algorithm) << std::endl;
  oss << std::setw(37) << "      max-allowed-weight-multiplier: "
  << config.initial_partitioning.max_allowed_weight_multiplier << std::endl;
  oss << std::setw(37) << "      contraction-limit-multiplier: "
  << config.initial_partitioning.contraction_limit_multiplier << std::endl;
  oss << std::setw(37) << "    IP Algorithm: "
  << toString(config.initial_partitioning.algo) << std::endl;
  oss << std::setw(37) << "    IP Refinement Algorithm: "
  << toString(config.initial_partitioning.refinement_algorithm) << std::endl;
  oss << std::setw(37) << "  Refinement Algorithm: "
  << toString(config.partition.refinement_algorithm) << std::endl;
  if (config.partition.refinement_algorithm == RefinementAlgorithm::twoway_fm ||
      config.partition.refinement_algorithm
      == RefinementAlgorithm::kway_fm ||
      config.partition.refinement_algorithm
      == RefinementAlgorithm::kway_fm_maxgain) {
    oss << std::setw(37) << "    stopping rule: "
    << toString(config.fm_local_search.stopping_rule) << std::endl;
    oss << std::setw(37) << "    use global rebalancing: "
    << toString(config.fm_local_search.global_rebalancing) << std::endl;
    oss << std::setw(37) << "    max. # fruitless moves: "
    << config.fm_local_search.max_number_of_fruitless_moves
    << std::endl;
    oss << std::setw(37) << "    random walk stop alpha: "
    << config.fm_local_search.alpha << std::endl;
    oss << std::setw(37) << "    random walk stop beta : "
    << config.fm_local_search.beta << std::endl;
  }
  if (config.partition.refinement_algorithm
      == RefinementAlgorithm::hyperedge) {
    oss << std::setw(37) << "    stopping rule: "
    << toString(config.her_fm.stopping_rule) << std::endl;
    oss << std::setw(37) << "    max. # fruitless moves: "
    << config.her_fm.max_number_of_fruitless_moves << std::endl;
  }
  if (config.partition.refinement_algorithm
      == RefinementAlgorithm::label_propagation) {
    oss << std::setw(37) << "    max. # iterations: "
    << config.lp_refiner.max_number_iterations << std::endl;
  }
  oss << std::setw(37) << "    max. # local search reps: "
  << config.partition.num_local_search_repetitions << std::endl;
  return oss.str();
}
}  // namespace partition
#endif  // SRC_PARTITION_CONFIGURATION_H_
