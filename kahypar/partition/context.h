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
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/partition/context_enum_classes.h"
#include "kahypar/utils/stats.h"

namespace kahypar {
struct MinHashSparsifierParameters {
  uint32_t max_hyperedge_size = std::numeric_limits<uint32_t>::max();
  uint32_t max_cluster_size = std::numeric_limits<uint32_t>::max();
  uint32_t min_cluster_size = std::numeric_limits<uint32_t>::max();
  uint32_t num_hash_functions = std::numeric_limits<uint32_t>::max();
  uint32_t combined_num_hash_functions = std::numeric_limits<uint32_t>::max();
  HypernodeID min_median_he_size = std::numeric_limits<uint32_t>::max();
  bool is_active = false;
};

struct CommunityDetection {
  bool enable_in_initial_partitioning = false;
  bool reuse_communities = false;
  LouvainEdgeWeight edge_weight = LouvainEdgeWeight::UNDEFINED;
  uint32_t max_pass_iterations = std::numeric_limits<uint32_t>::max();
  long double min_eps_improvement = std::numeric_limits<long double>::max();
};

struct PreprocessingParameters {
  bool enable_min_hash_sparsifier = false;
  bool enable_community_detection = false;
  MinHashSparsifierParameters min_hash_sparsifier = MinHashSparsifierParameters();
  CommunityDetection community_detection = CommunityDetection();
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

inline std::ostream& operator<< (std::ostream& str, const CommunityDetection& params) {
  str << "Community Detection Parameters:" << std::endl;
  str << "  use community detection in IP:      " << std::boolalpha
      << params.enable_in_initial_partitioning << std::endl;
  str << "  maximum louvain-pass iterations:    "
      << params.max_pass_iterations << std::endl;
  str << "  minimum quality improvement:        "
      << params.min_eps_improvement << std::endl;
  str << "  graph edge weight:                  "
      << params.edge_weight << std::endl;
  str << "  reuse community structure:          " << std::boolalpha
      << params.reuse_communities << std::endl;
  return str;
}


inline std::ostream& operator<< (std::ostream& str, const PreprocessingParameters& params) {
  str << "Preprocessing Parameters:" << std::endl;
  str << "  enable min hash sparsifier:         " << std::boolalpha
      << params.enable_min_hash_sparsifier << std::endl;
  str << "  enable community detection:         " << std::boolalpha
      << params.enable_community_detection << std::endl;
  if (params.enable_min_hash_sparsifier) {
    str << "-------------------------------------------------------------------------------"
        << std::endl;
    str << params.min_hash_sparsifier << std::endl;
  }
  if (params.enable_community_detection) {
    str << "-------------------------------------------------------------------------------"
        << std::endl;
    str << params.community_detection;
  }

  return str;
}

struct RatingParameters {
  RatingFunction rating_function = RatingFunction::UNDEFINED;
  CommunityPolicy community_policy = CommunityPolicy::UNDEFINED;
  HeavyNodePenaltyPolicy heavy_node_penalty_policy = HeavyNodePenaltyPolicy::UNDEFINED;
  AcceptancePolicy acceptance_policy = AcceptancePolicy::UNDEFINED;
};

inline std::ostream& operator<< (std::ostream& str, const RatingParameters& params) {
  str << "  Rating Parameters:" << std::endl;
  str << "    Rating Function:                  " << params.rating_function << std::endl;
  str << "    Use Community Structure:          " << params.community_policy << std::endl;
  str << "    Heavy Node Penalty:               " << params.heavy_node_penalty_policy << std::endl;
  str << "    Acceptance Policy:                " << params.acceptance_policy << std::endl;
  return str;
}


struct CoarseningParameters {
  CoarseningAlgorithm algorithm = CoarseningAlgorithm::UNDEFINED;
  RatingParameters rating = { };
  HypernodeID contraction_limit_multiplier = std::numeric_limits<HypernodeID>::max();
  double max_allowed_weight_multiplier = std::numeric_limits<double>::max();

  // Those will be determined dynamically
  HypernodeWeight max_allowed_node_weight = 0;
  HypernodeID contraction_limit = 0;
  double hypernode_weight_fraction = 0.0;
};

inline std::ostream& operator<< (std::ostream& str, const CoarseningParameters& params) {
  str << "Coarsening Parameters:" << std::endl;
  str << "  Algorithm:                          " << params.algorithm << std::endl;
  str << "  max-allowed-weight-multiplier:      " << params.max_allowed_weight_multiplier << std::endl;
  str << "  contraction-limit-multiplier:       " << params.contraction_limit_multiplier << std::endl;
  str << "  hypernode weight fraction:          ";
  // For the coarsening algorithm of the initial partitioning phase
  // these parameters are only known after main coarsening.
  if (params.hypernode_weight_fraction == 0) {
    str << "determined before IP";
  } else {
    str << params.hypernode_weight_fraction;
  }
  str << std::endl;
  str << "  max. allowed hypernode weight:      ";
  if (params.max_allowed_node_weight == 0) {
    str << "determined before IP";
  } else {
    str << params.max_allowed_node_weight;
  }
  str << std::endl;
  str << "  contraction limit:                  ";
  if (params.contraction_limit == 0) {
    str << "determined before IP";
  } else {
    str << params.contraction_limit;
  }
  str << std::endl << params.rating;
  return str;
}

struct LocalSearchParameters {
  struct FM {
    uint32_t max_number_of_fruitless_moves = std::numeric_limits<uint32_t>::max();
    double adaptive_stopping_alpha = std::numeric_limits<double>::max();
    RefinementStoppingRule stopping_rule = RefinementStoppingRule::UNDEFINED;
  };

  struct Sclap {
    int max_number_iterations = std::numeric_limits<int>::max();
  };

  struct Flow {
    bool enable_in_fm = false;
    FlowAlgorithm algorithm = FlowAlgorithm::UNDEFINED;
    FlowNetworkType network = FlowNetworkType::UNDEFINED;
    FlowExecutionMode execution_policy = FlowExecutionMode::UNDEFINED;
    double alpha = std::numeric_limits<double>::max();
    size_t beta = std::numeric_limits<size_t>::max();
    bool use_most_balanced_minimum_cut = false;
    bool use_adaptive_alpha_stopping_rule = false;
    bool ignore_small_hyperedge_cut = false;
    bool use_improvement_history = false;
  };

  FM fm { };
  Sclap sclap { };
  Flow flow { };
  RefinementAlgorithm algorithm = RefinementAlgorithm::UNDEFINED;
  int iterations_per_level = std::numeric_limits<int>::max();
};

inline std::ostream& operator<< (std::ostream& str, const LocalSearchParameters& params) {
  str << "Local Search Parameters:" << std::endl;
  str << "  Algorithm:                          " << params.algorithm << std::endl;
  str << "  iterations per level:               " << params.iterations_per_level << std::endl;
  if (params.algorithm == RefinementAlgorithm::twoway_fm ||
      params.algorithm == RefinementAlgorithm::kway_fm ||
      params.algorithm == RefinementAlgorithm::kway_fm_km1) {
    str << "  stopping rule:                      " << params.fm.stopping_rule << std::endl;
    if (params.fm.stopping_rule == RefinementStoppingRule::simple) {
      str << "  max. # fruitless moves:             " << params.fm.max_number_of_fruitless_moves << std::endl;
    } else {
      str << "  adaptive stopping alpha:            " << params.fm.adaptive_stopping_alpha << std::endl;
    }
  } else if (params.algorithm == RefinementAlgorithm::label_propagation) {
    str << "  max. # iterations:                  " << params.sclap.max_number_iterations << std::endl;
  }
  if (params.algorithm == RefinementAlgorithm::twoway_flow ||
      params.algorithm == RefinementAlgorithm::kway_flow ||
      params.flow.enable_in_fm) {
    str << "  Flow Refinement Parameters:" << std::endl;
    str << "    flow network:                     " << params.flow.network << std::endl;
    str << "    execution policy:                 " << params.flow.execution_policy << std::endl;
    str << "    most balanced minimum cut:        "
        << std::boolalpha << params.flow.use_most_balanced_minimum_cut << std::endl;
    str << "    alpha:                            " << params.flow.alpha << std::endl;
    if (params.flow.execution_policy == FlowExecutionMode::constant) {
      str << "    beta:                             " << params.flow.beta << std::endl;
    }
    str << "    adaptive alpha stopping rule:     "
        << std::boolalpha << params.flow.use_adaptive_alpha_stopping_rule << std::endl;
    str << "    ignore small HE cut:              "
        << std::boolalpha << params.flow.ignore_small_hyperedge_cut << std::endl;
    str << "    use improvement history:          "
        << std::boolalpha << params.flow.use_improvement_history << std::endl;
  } else if (params.algorithm == RefinementAlgorithm::do_nothing) {
    str << "  no coarsening!  " << std::endl;
  }
  return str;
}

struct InitialPartitioningParameters {
  Mode mode = Mode::UNDEFINED;
  InitialPartitioningTechnique technique = InitialPartitioningTechnique::UNDEFINED;
  InitialPartitionerAlgorithm algo = InitialPartitionerAlgorithm::UNDEFINED;
  CoarseningParameters coarsening = { };
  LocalSearchParameters local_search = { };
  uint32_t nruns = std::numeric_limits<uint32_t>::max();

  // The following parameters are only used internally and are not supposed to
  // be changed by the user.
  PartitionID k = std::numeric_limits<PartitionID>::max();
  double epsilon = std::numeric_limits<double>::max();
  HypernodeWeightVector upper_allowed_partition_weight = { };
  HypernodeWeightVector perfect_balance_partition_weight = { };
  PartitionID unassigned_part = 1;
  // Is used to get a tighter balance constraint for initial partitioning.
  // Before initial partitioning epsilon is set to init_alpha*epsilon.
  double init_alpha = 1;
  // If pool initial partitioner is used, the first 12 bits of this number decides
  // which algorithms are used.
  unsigned int pool_type = 1975;
  // Maximum iterations of the Label Propagation IP over all hypernodes
  int lp_max_iteration = 100;
  // Amount of hypernodes which are assigned around each start vertex (LP)
  int lp_assign_vertex_to_part = 5;
  bool refinement = true;
  bool verbose_output = false;
};

inline std::ostream& operator<< (std::ostream& str, const InitialPartitioningParameters& params) {
  str << "-------------------------------------------------------------------------------"
      << std::endl;
  str << "Initial Partitioning Parameters:" << std::endl;
  str << "  # IP trials:                        " << params.nruns << std::endl;
  str << "  Mode:                               " << params.mode << std::endl;
  str << "  Technique:                          " << params.technique << std::endl;
  str << "  Algorithm:                          " << params.algo << std::endl;
  str << "IP Coarsening:                        " << std::endl;
  str << params.coarsening;
  str << "IP Local Search:                      " << std::endl;
  str << params.local_search;
  str << "-------------------------------------------------------------------------------"
      << std::endl;
  return str;
}

struct PartitioningParameters {
  Mode mode = Mode::UNDEFINED;
  Objective objective = Objective::UNDEFINED;
  double epsilon = std::numeric_limits<double>::max();
  PartitionID k = std::numeric_limits<PartitionID>::max();
  PartitionID rb_lower_k = std::numeric_limits<PartitionID>::max();
  PartitionID rb_upper_k = std::numeric_limits<PartitionID>::max();
  int seed = 0;
  uint32_t global_search_iterations = std::numeric_limits<uint32_t>::max();
  mutable uint32_t current_v_cycle = 0;
  std::array<HypernodeWeight, 2> perfect_balance_part_weights { {
                                                                  std::numeric_limits<HypernodeWeight>::max(),
                                                                  std::numeric_limits<HypernodeWeight>::max()
                                                                } };
  std::array<HypernodeWeight, 2> max_part_weights { { std::numeric_limits<HypernodeWeight>::max(),
                                                      std::numeric_limits<HypernodeWeight>::max() } };
  HypernodeWeight total_graph_weight = std::numeric_limits<HypernodeWeight>::max();
  HyperedgeID hyperedge_size_threshold = std::numeric_limits<HypernodeID>::max();

  bool verbose_output = false;
  bool quiet_mode = false;
  bool sp_process_output = false;

  std::string graph_filename { };
  std::string graph_partition_filename { };
};

inline std::ostream& operator<< (std::ostream& str, const PartitioningParameters& params) {
  str << "Partitioning Parameters:" << std::endl;
  str << "  Hypergraph:                         " << params.graph_filename << std::endl;
  str << "  Partition File:                     " << params.graph_partition_filename << std::endl;
  str << "  Mode:                               " << params.mode << std::endl;
  str << "  Objective:                          " << params.objective << std::endl;
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


class Context {
 public:
  using PartitioningStats = Stats<Context>;

  PartitioningParameters partition { };
  PreprocessingParameters preprocessing { };
  CoarseningParameters coarsening { };
  InitialPartitioningParameters initial_partitioning { };
  LocalSearchParameters local_search { };
  ContextType type = ContextType::main;
  mutable PartitioningStats stats;

  Context() :
    stats(*this) { }

  Context(const Context& other) :
    partition(other.partition),
    preprocessing(other.preprocessing),
    coarsening(other.coarsening),
    initial_partitioning(other.initial_partitioning),
    local_search(other.local_search),
    type(other.type),
    stats(*this, &other.stats.topLevel()) { }

  bool isMainRecursiveBisection() const {
    return partition.mode == Mode::recursive_bisection && type == ContextType::main;
  }
};

inline std::ostream& operator<< (std::ostream& str, const Context& context) {
  str << "*******************************************************************************\n"
      << "*                            Partitioning Context                             *\n"
      << "*******************************************************************************\n"
      << context.partition
      << "-------------------------------------------------------------------------------"
      << std::endl
      << context.preprocessing
      << "-------------------------------------------------------------------------------"
      << std::endl
      << context.coarsening
      << context.initial_partitioning
      << context.local_search
      << "-------------------------------------------------------------------------------";
  return str;
}

static inline void checkRecursiveBisectionMode(RefinementAlgorithm& algo) {
  if (algo == RefinementAlgorithm::kway_fm ||
      algo == RefinementAlgorithm::kway_fm_km1 ||
      algo == RefinementAlgorithm::kway_flow ||
      algo == RefinementAlgorithm::kway_fm_flow_km1) {
    LOG << "WARNING: local search algorithm is set to"
        << algo
        << ". However, the 2-way counterpart "
        << "is better and faster.";
    LOG << "Should the local search algorithm be changed (Y/N)?";
    char answer = 'N';
    std::cin >> answer;
    answer = std::toupper(answer);
    if (answer == 'Y') {
      if (algo == RefinementAlgorithm::kway_fm || algo == RefinementAlgorithm::kway_fm_km1) {
        algo = RefinementAlgorithm::twoway_fm;
      } else if (algo == RefinementAlgorithm::kway_flow) {
        algo = RefinementAlgorithm::twoway_flow;
      } else if (algo == RefinementAlgorithm::kway_fm_flow_km1) {
        algo = RefinementAlgorithm::twoway_fm_flow;
      }
      LOG << "Changing local search algorithm to"
          << algo;
    }
  }
}

void checkDirectKwayMode(RefinementAlgorithm& algo, Objective& objective) {
  if (algo == RefinementAlgorithm::twoway_fm ||
      algo == RefinementAlgorithm::twoway_flow ||
      algo == RefinementAlgorithm::twoway_fm_flow) {
    LOG << "WARNING: local search algorithm is set to"
        << algo
        << ". This algorithm cannot be used for direct k-way partitioning with k>2.";
    LOG << "Should the local search algorithm be changed to corresponding k-way counterpart (Y/N)?";
    char answer = 'N';
    std::cin >> answer;
    answer = std::toupper(answer);
    if (answer == 'Y') {
      if (algo == RefinementAlgorithm::twoway_fm && objective == Objective::cut) {
        algo = RefinementAlgorithm::kway_fm;
      } else if (algo == RefinementAlgorithm::twoway_fm && objective == Objective::km1) {
        algo = RefinementAlgorithm::kway_fm_km1;
      } else if (algo == RefinementAlgorithm::twoway_flow) {
        algo = RefinementAlgorithm::kway_flow;
      } else if (algo == RefinementAlgorithm::twoway_fm_flow && objective == Objective::km1) {
        algo = RefinementAlgorithm::kway_fm_flow_km1;
      } else if (algo == RefinementAlgorithm::twoway_fm_flow && objective == Objective::cut) {
        algo = RefinementAlgorithm::kway_flow;
      }
      LOG << "Changing local search algorithm to"
          << algo;
    }
  }
}

static inline void sanityCheck(Context& context) {
  switch (context.partition.mode) {
    case Mode::recursive_bisection:
      // Prevent contexturations that currently don't make sense.
      // If KaHyPar operates in recursive bisection mode, than the
      // initial partitioning algorithm is called on the coarsest graph to
      // create a bipartition (i.e. it is only called for k=2).
      // Therefore, the initial partitioning algorithm
      // should run in direct mode and not in recursive bisection mode (since there
      // is nothing left to bisect).
      ALWAYS_ASSERT(context.initial_partitioning.mode == Mode::direct_kway,
                    context.initial_partitioning.mode);
      // Furthermore, the IP algorithm should not use the multilevel technique itself,
      // because the hypergraph is already coarse enough for initial partitioning.
      // Therefore we prevent further coarsening by enforcing flat rather than multilevel.
      // If initial partitioning is set to direct k-way, it does not make sense to use
      // multilevel as initial partitioning technique, because in this case KaHyPar
      // could just do the additional multilevel coarsening and then call the initial
      // partitioning algorithm as a flat algorithm.
      ALWAYS_ASSERT(context.initial_partitioning.technique == InitialPartitioningTechnique::flat,
                    context.initial_partitioning.technique);
      checkRecursiveBisectionMode(context.local_search.algorithm);
      break;
    case Mode::direct_kway:
      // When KaHyPar runs in direct k-way mode, it makes no sense to use the initial
      // partitioner in direct multilevel mode, because we could essentially get the same
      // behavior if we would just use a smaller contraction limit in the main partitioner.
      // Since the contraction limits for main and initial partitioner are specifically tuned,
      // we currently forbid this contexturation.
      ALWAYS_ASSERT(context.initial_partitioning.mode != Mode::direct_kway ||
                    context.initial_partitioning.technique == InitialPartitioningTechnique::flat,
                    context.initial_partitioning.mode
                    << context.initial_partitioning.technique);
      checkDirectKwayMode(context.local_search.algorithm, context.partition.objective);
      break;
    default:
      // should never happen, because partitioning is either done via RB or directly
      break;
  }
  switch (context.initial_partitioning.mode) {
    case Mode::recursive_bisection:
      checkRecursiveBisectionMode(context.initial_partitioning.local_search.algorithm);
      break;
    case Mode::direct_kway:
      // If the main partitioner runs in recursive bisection mode, then the initial
      // partitioner running in direct mode can use two-way FM as a local search
      // algorithm because we only perform bisections.
      if (context.partition.mode != Mode::recursive_bisection) {
        checkDirectKwayMode(context.initial_partitioning.local_search.algorithm, context.partition.objective);
      }
      break;
    default:
      // should never happen, because initial partitioning is either done via RB or directly
      break;
  }

  /*if (context.local_search.algorithm == RefinementAlgorithm::twoway_fm_flow) {
    context.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    context.local_search.flow.enable_in_fm = true;
  } else*/ if (context.local_search.algorithm == RefinementAlgorithm::kway_fm_flow_km1) {
    context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;
    context.local_search.flow.enable_in_fm = true;
  }
}
}  // namespace kahypar
