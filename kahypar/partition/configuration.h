/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <array>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/partition/configuration_enum_classes.h"

namespace kahypar {
struct MinHashSparsifierParameters {
  uint32_t max_hyperedge_size = 1200;
  uint32_t max_cluster_size = 10;
  uint32_t min_cluster_size = 2;
  uint32_t num_hash_functions = 5;
  uint32_t combined_num_hash_functions = 100;
  HypernodeID min_median_he_size = 28;
  bool is_active = false;
};

struct LouvainCommunityDetection {
  bool enable_in_initial_partitioning = false;
  LouvainEdgeWeight edge_weight = LouvainEdgeWeight::hybrid;
  bool use_bipartite_graph = true;
  int max_pass_iterations = 100;
  long double min_eps_improvement = 0.0001;
};

struct PreprocessingParameters {
  bool enable_min_hash_sparsifier = false;
  bool enable_louvain_community_detection = false;
  bool remove_always_cut_hes = false;
  bool remove_parallel_hes = false;
  MinHashSparsifierParameters min_hash_sparsifier = MinHashSparsifierParameters();
  LouvainCommunityDetection louvain_community_detection = LouvainCommunityDetection();
};

inline std::ostream& operator<< (std::ostream& str, const MinHashSparsifierParameters& params) {
  str << "MinHash Sparsifier Parameters:" << std::endl;
  str << "  max hyperedge size:                 "
  << params.max_hyperedge_size << std::endl;
  str << "  max cluster size:                   "
  << params.max_cluster_size << std::endl;
  str << "  min cluster size:                   "
  << params.min_cluster_size << std::endl;
  str << "  number of hash functions:           "
  << params.num_hash_functions << std::endl;
  str << "  number of combined hash functions:  "
  << params.combined_num_hash_functions << std::endl;
  str << "  active at median net size >=:       "
  << params.min_median_he_size << std::endl;
  str << "  sparsifier is active:               " << std::boolalpha
  << params.is_active << std::noboolalpha;
  return str;
}

inline std::ostream& operator<< (std::ostream& str, const LouvainCommunityDetection& params) {
  str << "Community Detection Parameters:" << std::endl;
  str << "  use louvain in IP:                  " << std::boolalpha
  << params.enable_in_initial_partitioning << std::endl;
  str << "  use bipartite graph representation: " << std::boolalpha
  << params.use_bipartite_graph << std::endl;
  str << "  maximum louvain-pass iterations:    "
  << params.max_pass_iterations << std::endl;
  str << "  minimum quality improvement:        "
  << params.min_eps_improvement << std::endl;
  str << "  graph edge weight:                  "
  << toString(params.edge_weight) << std::endl;
  return str;
}


inline std::ostream& operator<< (std::ostream& str, const PreprocessingParameters& params) {
  str << "Preprocessing Parameters:" << std::endl;
  str << "  enable min hash sparsifier:         " << std::boolalpha
  << params.enable_min_hash_sparsifier << std::endl;
  str << "  enable louvain community detection: " << std::boolalpha
  << params.enable_louvain_community_detection << std::endl;
  str << "  remove parallel HEs:                " << std::boolalpha
  << params.remove_parallel_hes << std::endl;
  str << "  remove HEs that always will be cut: " << std::boolalpha
  << params.remove_always_cut_hes << std::endl;
  if (params.enable_min_hash_sparsifier) {
    str << "---------------------------------------------------------------------" << std::endl;
    str << params.min_hash_sparsifier << std::endl;
  }
  if (params.enable_louvain_community_detection) {
    str << "---------------------------------------------------------------------" << std::endl;
    str << params.louvain_community_detection << std::endl;
  }

  return str;
}


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
    mode(Mode::recursive_bisection),
    technique(InitialPartitioningTechnique::flat),
    algo(InitialPartitionerAlgorithm::pool),
    coarsening(),
    local_search(),
    nruns(20),
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

  Mode mode;
  InitialPartitioningTechnique technique;
  InitialPartitionerAlgorithm algo;
  CoarseningParameters coarsening;
  LocalSearchParameters local_search;
  int nruns;

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
  str << "  # IP trials:                        " << params.nruns << std::endl;
  str << "  Mode:                               " << toString(params.mode) << std::endl;
  str << "  Technique:                          " << toString(params.technique) << std::endl;
  str << "  Algorithm:                          " << toString(params.algo) << std::endl;
  str << "IP Coarsening:                        " << std::endl;
  str << params.coarsening;
  str << "IP Local Search:                      " << std::endl;
  str << params.local_search;
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
    perfect_balance_part_weights({
      std::numeric_limits<HypernodeWeight>::max(),
      std::numeric_limits<HypernodeWeight>::max()
    }),
    max_part_weights({ std::numeric_limits<HypernodeWeight>::max(),
                       std::numeric_limits<HypernodeWeight>::max() }),
    total_graph_weight(0),
    hyperedge_size_threshold(std::numeric_limits<HypernodeID>::max()),
    verbose_output(false),
    collect_stats(false),
    graph_filename(),
    graph_partition_filename() { }

  Mode mode;
  Objective objective;
  double epsilon;
  PartitionID k;
  PartitionID rb_lower_k;
  PartitionID rb_upper_k;
  int seed;
  int global_search_iterations;
  int current_v_cycle;
  std::array<HypernodeWeight, 2> perfect_balance_part_weights;
  std::array<HypernodeWeight, 2> max_part_weights;
  HypernodeWeight total_graph_weight;
  HyperedgeID hyperedge_size_threshold;

  bool verbose_output;
  bool collect_stats;

  std::string graph_filename;
  std::string graph_partition_filename;
};

inline std::ostream& operator<< (std::ostream& str, const PartitioningParameters& params) {
  str << "KaHyPar Partitioning Parameters:" << std::endl;
  str << "  Hypergraph:                         " << params.graph_filename << std::endl;
  str << "  Partition File:                     " << params.graph_partition_filename << std::endl;
  str << "  Mode:                               " << toString(params.mode) << std::endl;
  str << "  Objective:                          " << toString(params.objective) << std::endl;
  str << "  k:                                  " << params.k << std::endl;
  str << "  epsilon:                            " << params.epsilon << std::endl;
  str << "  seed:                               " << params.seed << std::endl;
  str << "  # V-cycles:                         " << params.global_search_iterations << std::endl;
  str << "  hyperedge size threshold:           " << params.hyperedge_size_threshold << std::endl;
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
    preprocessing(),
    coarsening(),
    initial_partitioning(),
    local_search() { }
  PartitioningParameters partition;
  PreprocessingParameters preprocessing;
  CoarseningParameters coarsening;
  InitialPartitioningParameters initial_partitioning;
  LocalSearchParameters local_search;
};

inline std::ostream& operator<< (std::ostream& str, const Configuration& config) {
  str << config.partition
  << "---------------------------------------------------------------------" << std::endl
  << config.preprocessing
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
}  // namespace kahypar
