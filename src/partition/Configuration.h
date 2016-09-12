/***************************************************************************
 *  Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "definitions.h"
#include "partition/ConfigurationEnumClasses.h"

using defs::HypernodeWeight;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HypernodeWeightVector;
namespace partition {
struct CoarseningParameters {
  CoarseningAlgorithm algorithm = CoarseningAlgorithm::heavy_lazy;
  HypernodeID contraction_limit_multiplier = 160;
  double max_allowed_weight_multiplier = 3.25;

  HypernodeWeight max_allowed_node_weight = 0;
  HypernodeID contraction_limit = 0;
  double hypernode_weight_fraction = 0.0;
};

inline std::ostream& operator<< (std::ostream& str, const CoarseningParameters& params) {
  str << "Coarsening Parameters:" << std::endl;
  str << "  Algorithm:                          " << toString(params.algorithm) << std::endl;
  str << "  max-allowed-weight-multiplier:      " << params.max_allowed_weight_multiplier << std::endl;
  str << "  contraction-limit-multiplier:       " << params.contraction_limit_multiplier << std::endl;
  if (params.hypernode_weight_fraction != 0.0) {
    str << "  hypernode weight fraction:          " << params.hypernode_weight_fraction << std::endl;
  }
  if (params.max_allowed_node_weight != 0) {
    str << "  max. allowed hypernode weight:      " << params.max_allowed_node_weight << std::endl;
  }
  if (params.contraction_limit != 0) {
    str << "  contraction limit:                  " << params.contraction_limit << std::endl;
  }
  return str;
}

struct LocalSearchParameters {
  struct FM {
    int max_number_of_fruitless_moves = 350;
    double adaptive_stopping_alpha = 1.0;
    double adaptive_stopping_beta = 0.0;
    RefinementStoppingRule stopping_rule = RefinementStoppingRule::simple;
    GlobalRebalancingMode global_rebalancing = GlobalRebalancingMode::off;
  };

  struct Sclap {
    int max_number_iterations = std::numeric_limits<int>::max();
  };

  LocalSearchParameters() :
    fm(),
    sclap(),
    algorithm(RefinementAlgorithm::kway_fm),
    iterations_per_level(std::numeric_limits<int>::max()) { }

  FM fm;
  Sclap sclap;
  RefinementAlgorithm algorithm;
  int iterations_per_level;
};

inline std::ostream& operator<< (std::ostream& str, const LocalSearchParameters& params) {
  str << "Local Search Parameters:" << std::endl;
  str << "  Algorithm:                          " << toString(params.algorithm) << std::endl;
  str << "  iterations per level:               " << params.iterations_per_level << std::endl;
  if (params.algorithm == RefinementAlgorithm::twoway_fm ||
      params.algorithm == RefinementAlgorithm::kway_fm ||
      params.algorithm == RefinementAlgorithm::kway_fm_km1) {
    str << "  stopping rule:                      " << toString(params.fm.stopping_rule) << std::endl;
    if (params.fm.stopping_rule == RefinementStoppingRule::simple) {
      str << "  max. # fruitless moves:             " << params.fm.max_number_of_fruitless_moves << std::endl;
    } else {
      str << "  adaptive stopping alpha:            " << params.fm.adaptive_stopping_alpha << std::endl;
      str << "  adaptive stopping beta:             " << params.fm.adaptive_stopping_beta << std::endl;
    }
    str << "  use global rebalancing:             " << toString(params.fm.global_rebalancing) << std::endl;
  } else if (params.algorithm == RefinementAlgorithm::label_propagation) {
    str << "  max. # iterations:                  " << params.sclap.max_number_iterations << std::endl;
  } else if (params.algorithm == RefinementAlgorithm::do_nothing) {
    str << "  no coarsening!  " << std::endl;
  }
  return str;
}

struct InitialPartitioningParameters {
  InitialPartitioningParameters() :
    tool(InitialPartitioner::KaHyPar),
    mode(Mode::recursive_bisection),
    technique(InitialPartitioningTechnique::flat),
    algo(InitialPartitionerAlgorithm::pool),
    coarsening(),
    local_search(),
    nruns(20),
    hmetis_ub_factor(-1.0),
    tool_path(),
    k(2),
    epsilon(0.05),
    upper_allowed_partition_weight(),
    perfect_balance_partition_weight(),
    unassigned_part(1),
    init_alpha(1.0),
    pool_type(1975),
    lp_max_iteration(100),
    lp_assign_vertex_to_part(5),
    refinement(true) {
    // Specifically tuned for IP
    coarsening.contraction_limit_multiplier = 150;
    coarsening.max_allowed_weight_multiplier = 2.5;

    local_search.algorithm = RefinementAlgorithm::twoway_fm;
    local_search.fm.max_number_of_fruitless_moves = 50;
    local_search.fm.stopping_rule = RefinementStoppingRule::simple;
    // Since initial partitioning starts local search with all HNs, global
    // rebalancing doesn't do anything is this case and just induces additional
    // overhead.
    local_search.fm.global_rebalancing = GlobalRebalancingMode::off;
  }

  InitialPartitioner tool;
  Mode mode;
  InitialPartitioningTechnique technique;
  InitialPartitionerAlgorithm algo;
  CoarseningParameters coarsening;
  LocalSearchParameters local_search;
  int nruns;
  // If hMetis-R is used as initial partitioner, we have to calculate the
  // appropriate ub_factor to ensure that the final initial partition is
  // balanced.
  double hmetis_ub_factor;
  // If external tools are used to do initial partitioning, tool_path stores
  // the path to binary.
  std::string tool_path;

  // The following parameters are only used internally and are not supposed to
  // be changed by the user.
  PartitionID k;
  double epsilon;
  HypernodeWeightVector upper_allowed_partition_weight;
  HypernodeWeightVector perfect_balance_partition_weight;
  PartitionID unassigned_part;
  // Is used to get a tighter balance constraint for initial partitioning.
  // Before initial partitioning epsilon is set to init_alpha*epsilon.
  double init_alpha = 1.0;
  // If pool initial partitioner is used, the first 12 bits of this number decides
  // which algorithms are used.
  unsigned int pool_type = 1975;
  // Maximum iterations of the Label Propagation IP over all hypernodes
  int lp_max_iteration = 100;
  // Amount of hypernodes which are assigned around each start vertex (LP)
  int lp_assign_vertex_to_part = 5;
  bool refinement = true;
};

inline std::ostream& operator<< (std::ostream& str, const InitialPartitioningParameters& params) {
  str << "---------------------------------------------------------------------" << std::endl;
  str << "Initial Partitioning Parameters:" << std::endl;
  str << "  Tool:                               " << toString(params.tool) << std::endl;
  str << "  # IP trials:                        " << params.nruns << std::endl;
  if (params.tool == InitialPartitioner::KaHyPar) {
    str << "  Mode:                               " << toString(params.mode) << std::endl;
    str << "  Technique:                          " << toString(params.technique) << std::endl;
    str << "  Algorithm:                          " << toString(params.algo) << std::endl;
    str << "IP Coarsening:                        " << std::endl;
    str << params.coarsening;
    str << "IP Local Search:                      " << std::endl;
    str << params.local_search;
  } else {
    str << "  IP Path:                            " << params.tool_path << std::endl;
    str << "  hmetis_ub_factor:                   " << params.hmetis_ub_factor << std::endl;
  }
  str << "---------------------------------------------------------------------" << std::endl;
  return str;
}

struct PartitioningParameters {
  PartitioningParameters() :
    mode(Mode::direct_kway),
    objective(Objective::cut),
    epsilon(0.03),
    k(2),
    rb_lower_k(0),
    rb_upper_k(1),
    seed(0),
    global_search_iterations(0),
    current_v_cycle(0),
    work_factor(std::numeric_limits<double>::max()),
    perfect_balance_part_weights({
      std::numeric_limits<HypernodeWeight>::max(),
      std::numeric_limits<HypernodeWeight>::max()
    }),
    max_part_weights({ std::numeric_limits<HypernodeWeight>::max(),
                       std::numeric_limits<HypernodeWeight>::max() }),
    total_graph_weight(0),
    hyperedge_size_threshold(std::numeric_limits<HypernodeID>::max()),
    remove_hes_that_always_will_be_cut(false),
    initial_parallel_he_removal(false),
    verbose_output(false),
    collect_stats(false),
    graph_filename(),
    graph_partition_filename(),
    coarse_graph_filename(),
    coarse_graph_partition_filename() { }

  Mode mode;
  Objective objective;
  double epsilon;
  PartitionID k;
  PartitionID rb_lower_k;
  PartitionID rb_upper_k;
  int seed;
  int global_search_iterations;
  int current_v_cycle;
  double work_factor;
  std::array<HypernodeWeight, 2> perfect_balance_part_weights;
  std::array<HypernodeWeight, 2> max_part_weights;
  HypernodeWeight total_graph_weight;
  HyperedgeID hyperedge_size_threshold;
  bool remove_hes_that_always_will_be_cut;
  bool initial_parallel_he_removal;
  bool verbose_output;
  bool collect_stats;

  std::string graph_filename;
  std::string graph_partition_filename;
  std::string coarse_graph_filename;
  std::string coarse_graph_partition_filename;
};

inline std::ostream& operator<< (std::ostream& str, const PartitioningParameters& params) {
  str << "KaHyPar Partitioning Parameters:" << std::endl;
  str << "  Hypergraph:                         " << params.graph_filename << std::endl;
  str << "  Partition File:                     " << params.graph_partition_filename << std::endl;
  str << "  Coarsened Hypergraph:               " << params.coarse_graph_filename << std::endl;
  str << "  Coarsened Partition File:           " << params.coarse_graph_partition_filename << std::endl;
  str << "  Mode:                               " << toString(params.mode) << std::endl;
  str << "  Objective:                          " << toString(params.objective) << std::endl;
  str << "  k:                                  " << params.k << std::endl;
  str << "  epsilon:                            " << params.epsilon << std::endl;
  str << "  seed:                               " << params.seed << std::endl;
  str << "  # V-cycles:                         " << params.global_search_iterations << std::endl;
  str << "  hyperedge size threshold:           " << params.hyperedge_size_threshold << std::endl;
  str << "  work factor:                        " << params.work_factor << std::endl;
  str << "  initially remove parallel HEs:      " << std::boolalpha
  << params.initial_parallel_he_removal
  << std::endl;
  str << "  remove HEs that always will be cut: " << std::boolalpha
  << params.remove_hes_that_always_will_be_cut
  << std::endl;
  str << "  total hypergraph weight:            " << params.total_graph_weight << std::endl;
  str << "  L_opt0:                             " << params.perfect_balance_part_weights[0]
  << std::endl;
  str << "  L_opt1:                             " << params.perfect_balance_part_weights[1]
  << std::endl;
  str << "  L_max0:                             " << params.max_part_weights[0] << std::endl;
  str << "  L_max1:                             " << params.max_part_weights[1] << std::endl;
  return str;
}


struct Configuration {
  Configuration() :
    partition(),
    coarsening(),
    initial_partitioning(),
    local_search() { }
  PartitioningParameters partition;
  CoarseningParameters coarsening;
  InitialPartitioningParameters initial_partitioning;
  LocalSearchParameters local_search;
};

inline std::ostream& operator<< (std::ostream& str, const Configuration& config) {
  str << config.partition
  << "---------------------------------------------------------------------" << std::endl
  << config.coarsening
  << config.initial_partitioning
  << config.local_search
  << "---------------------------------------------------------------------" << std::endl;
  return str;
}

inline std::string toString(const Configuration& config) {
  std::ostringstream oss;
  oss << config << std::endl;
  return oss.str();
}
}  // namespace partition
