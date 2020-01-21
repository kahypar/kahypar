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
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/context/context_enum_classes.h"
#include "kahypar/partition/context/include_parameters.h"
#include "kahypar/partition/evolutionary/action.h"
#include "kahypar/partition/parallel/communicator.h"
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
  bool enable_deduplication = false;
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
  str << "  enable deduplication:               " << std::boolalpha
      << params.enable_deduplication << std::endl;
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

class Context {
 public:
  using PartitioningStats = Stats<Context>;

  PartitioningParameters partition { };
  PreprocessingParameters preprocessing { };
  CoarseningParameters coarsening { };
  InitialPartitioningParameters initial_partitioning { };
  LocalSearchParameters local_search { };
  EvolutionaryParameters evolutionary { };
  Communicator communicator{ };
  ContextType type = ContextType::main;
  mutable PartitioningStats stats;
  bool partition_evolutionary = false;

  Context() :
    stats(*this) { }

  ~Context() { }

  Context(const Context& other) :
    partition(other.partition),
    preprocessing(other.preprocessing),
    coarsening(other.coarsening),
    initial_partitioning(other.initial_partitioning),
    local_search(other.local_search),
    evolutionary(other.evolutionary),
    communicator(other.communicator),
    type(other.type),
    stats(*this, &other.stats.topLevel()),
    partition_evolutionary(other.partition_evolutionary) { }

  Context& operator= (const Context&) = delete;

  bool isMainRecursiveBisection() const {
    return partition.mode == Mode::recursive_bisection && type == ContextType::main;
  }

  std::vector<ClusterID> getCommunities() const {
    return evolutionary.communities;
  }

  void setupPartWeights(const HypernodeWeight total_hypergraph_weight) {
    if (partition.use_individual_part_weights) {
      partition.perfect_balance_part_weights = partition.max_part_weights;
    } else {
      partition.perfect_balance_part_weights.clear();
      partition.perfect_balance_part_weights.push_back(ceil(
                                                         total_hypergraph_weight
                                                         / static_cast<double>(partition.k)));
      for (PartitionID part = 1; part != partition.k; ++part) {
        partition.perfect_balance_part_weights.push_back(
          partition.perfect_balance_part_weights[0]);
      }
      partition.max_part_weights.clear();
      partition.max_part_weights.push_back((1 + partition.epsilon)
                                           * partition.perfect_balance_part_weights[0]);
      for (PartitionID part = 1; part != partition.k; ++part) {
        partition.max_part_weights.push_back(partition.max_part_weights[0]);
      }
    }
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
      << "-------------------------------------------------------------------------------"
      << std::endl;
  if (context.partition_evolutionary) {
    str << context.evolutionary
        << "-------------------------------------------------------------------------------";
  }
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
        algo = RefinementAlgorithm::kway_fm_flow;
      }
      LOG << "Changing local search algorithm to"
          << algo;
    }
  }
}

static inline void sanityCheck(const Hypergraph& hypergraph, Context& context) {
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
  if (context.partition.use_individual_part_weights && context.partition.max_part_weights.empty()) {
    LOG << "Individual block weights not specified. Please use --blockweights to specify the weight of each block";
    std::exit(0);
  }

  if (!context.partition.use_individual_part_weights &&
      !context.partition.max_part_weights.empty()) {
    LOG << "Individual block weights specified, but --use-individual-blockweights=false.";
    LOG << "Do you want to use the block weights you specified (Y/N)?";
    char answer = 'N';
    std::cin >> answer;
    answer = std::toupper(answer);
    if (answer == 'Y') {
      context.partition.use_individual_part_weights = true;
    } else {
      LOG << "Individual block weights will be ignored. Partition with imbalance epsilon="
          << context.partition.epsilon << " (Y/N)?";
      answer = 'N';
      std::cin >> answer;
      answer = std::toupper(answer);
      if (answer == 'N') {
        std::exit(0);
      }
    }
  }

  if (context.partition.use_individual_part_weights) {
    if (context.partition.max_part_weights.size() != static_cast<size_t>(context.partition.k)) {
      LOG << "k=" << context.partition.k << ",but # part weights ="
          << context.partition.max_part_weights.size();
      std::exit(-1);
    }
    const HypernodeWeight sum_part_weights =
      std::accumulate(context.partition.max_part_weights.begin(),
                      context.partition.max_part_weights.end(),
                      0);
    if (sum_part_weights < hypergraph.totalWeight()) {
      LOG << "Sum of individual part weights is less than sum of vertex weights";
      std::exit(-1);
    }
  }

  if (context.partition.mode == Mode::direct_kway &&
      context.partition.objective == Objective::cut) {
    if (context.local_search.algorithm == RefinementAlgorithm::kway_fm_km1 ||
        context.local_search.algorithm == RefinementAlgorithm::kway_fm_flow_km1) {
      LOG << "\nRefinement algorithm" << context.local_search.algorithm
          << "currently only works for connectivity (km1) optimization.";
      LOG << "Please use the corresponding cut algorithm.";
      std::exit(0);
    }
  } else if (context.partition.mode == Mode::direct_kway &&
             context.partition.objective == Objective::km1) {
    if (context.local_search.algorithm == RefinementAlgorithm::kway_fm ||
        context.local_search.algorithm == RefinementAlgorithm::kway_fm_flow) {
      LOG << "\nRefinement algorithm" << context.local_search.algorithm
          << "currently only works for cut optimization.";
      LOG << "Please use the corresponding connectivity (km1) algorithm.";
      std::exit(0);
    }
  }

  if (context.partition.global_search_iterations != 0 &&
      context.partition.mode == kahypar::Mode::recursive_bisection) {
    std::cerr << "V-Cycles are not supported in recursive bisection mode." << std::endl;
    std::exit(-1);
  }
}
}  // namespace kahypar
