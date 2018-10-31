/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <boost/program_options.hpp>

#if defined(_MSC_VER)
#include <process.h>
#include <Windows.h>
#else
#include <sys/ioctl.h>
#endif

#include <cctype>
#include <limits>
#include <string>
#include <vector>

#include "kahypar/kahypar.h"

namespace po = boost::program_options;

namespace kahypar {
namespace platform {
int getTerminalWidth() {
  int columns = 0;
#if defined(_MSC_VER)
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
  struct winsize w = { };
  ioctl(0, TIOCGWINSZ, &w);
  columns = w.ws_col;
#endif
  return columns;
}

int getProcessID() {
#if defined(_MSC_VER)
  return _getpid();
#else
  return getpid();
#endif
}
}  // namespace platform

po::options_description createGeneralOptionsDescription(Context& context, const int num_columns) {
  po::options_description options("General Options", num_columns);
  options.add_options()
    ("seed",
    po::value<int>(&context.partition.seed)->value_name("<int>"),
    "Seed for random number generator \n"
    "(default: -1)")
    ("fixed-vertices,f",
    po::value<std::string>(&context.partition.fixed_vertex_filename)->value_name("<string>"),
    "Fixed vertex filename")
    ("part-file",
    po::value<std::string>(&context.partition.input_partition_filename)->value_name("<string>"),
    "Input Partition filename. The input partition is then refined using direct k-way V-cycles.")
    ("cmaxnet",
    po::value<HyperedgeID>(&context.partition.hyperedge_size_threshold)->value_name("<int>")->notifier(
      [&](const HyperedgeID) {
      if (context.partition.hyperedge_size_threshold == -1) {
        context.partition.hyperedge_size_threshold = std::numeric_limits<HyperedgeID>::max();
      }
    }),
    "Hyperedges larger than cmaxnet are ignored during partitioning process.")
    ("vcycles",
    po::value<uint32_t>(&context.partition.global_search_iterations)->value_name("<uint32_t>"),
    "# V-cycle iterations for direct k-way partitioning")
    ("use-individual-part-weights",
    po::value<bool>(&context.partition.use_individual_part_weights)->value_name("<bool>"),
    "# Use individual part weights specified with --partweights= option")
    ("part-weights",
    po::value<std::vector<HypernodeWeight> >(&context.partition.max_part_weights)->multitoken(),
    "Individual target part weights")
    ("objective,o",
    po::value<std::string>()->value_name("<string>")->required()->notifier([&](const std::string& s) {
      if (s == "cut") {
        context.partition.objective = Objective::cut;
      } else if (s == "km1") {
        context.partition.objective = Objective::km1;
      }
    }),
    "Objective: \n"
    " - cut : cut-net metric \n"
    " - km1 : (lambda-1) metric")
    ("mode,m",
    po::value<std::string>()->value_name("<string>")->required()->notifier(
      [&](const std::string& mode) {
      context.partition.mode = kahypar::modeFromString(mode);
    }),
    "Partitioning mode: \n"
    " - (recursive) bisection \n"
    " - (direct) k-way");
  return options;
}

po::options_description createFlowRefinementOptionsDescription(Context& context,
                                                               const int num_columns,
                                                               const bool initial_partitioning) {
  po::options_description options((initial_partitioning ?
                                   "Initial Partitioning Flow Refinement Options" :
                                   "Flow Refinement Options"), num_columns);
  options.add_options()
    ((initial_partitioning ? "i-r-flow-algorithm" : "r-flow-algorithm"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& ftype) {
      if (initial_partitioning) {
        context.initial_partitioning.local_search.flow.algorithm = kahypar::flowAlgorithmFromString(ftype);
      } else {
        context.local_search.flow.algorithm = kahypar::flowAlgorithmFromString(ftype);
      }
    }),
    "Flow Algorithms:\n"
    " - edmond_karp       : Edmond-Karp Max-Flow algorithm\n"
    " - goldberg_tarjan   : GoldbergTarjan Max-Flow algorithm\n"
    " - boykov_kolmogorov : Boykov-Kolmogorov Max-Flow algorithm\n"
    " - ibfs              : IBFS Max-Flow algorithm\n"
    "(default: ibfs)")
    ((initial_partitioning ? "i-r-flow-network" : "r-flow-network"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& type) {
      if (initial_partitioning) {
        context.initial_partitioning.local_search.flow.network = kahypar::flowNetworkFromString(type);
      } else {
        context.local_search.flow.network = kahypar::flowNetworkFromString(type);
      }
    }),
    "Flow Networks:\n"
    " - lawler : Lawler Network\n"
    " - heuer  : Heuer Network (Removes all hypernodes with d(v) <= 3)\n"
    " - wong   : Wong Network (Model each HE with |e| = 2 as graph edge)\n"
    " - hybrid : Hybrid Network (Combination of Heuer + Wong Network)\n"
    "(default: hybrid)")
    ((initial_partitioning ? "i-r-flow-execution-policy" : "r-flow-execution-policy"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& ftype) {
      if (initial_partitioning) {
        context.initial_partitioning.local_search.flow.execution_policy = kahypar::flowExecutionPolicyFromString(ftype);
      } else {
        context.local_search.flow.execution_policy = kahypar::flowExecutionPolicyFromString(ftype);
      }
    }),
    "Flow Execution Modes:\n"
    " - constant    : Execute flows in each level i with i = beta * j (j \\in {1,2,...})\n"
    " - exponential : Execute flows in each level i with i = 2^j (j \\in {1,2,...})\n"
    " - multilevel  : Execute flows in each level i with i = |V|/2^j (j \\in {1,2,...})\n"
    "(default: exponential)")
    ((initial_partitioning ? "i-r-flow-alpha" : "r-flow-alpha"),
    po::value<double>((initial_partitioning ? &context.initial_partitioning.local_search.flow.alpha : &context.local_search.flow.alpha))->value_name("<double>"),
    "Determine maximum size of a flow problem during adaptive flow iterations (epsilon' = alpha * epsilon) \n"
    "(default: 16.0)")
    ((initial_partitioning ? "i-r-flow-beta" : "r-flow-beta"),
    po::value<size_t>((initial_partitioning ? &context.initial_partitioning.local_search.flow.beta : &context.local_search.flow.beta))->value_name("<size_t>"),
    "Beta of CONSTANT flow execution policy \n"
    "(default: 128)")
    ((initial_partitioning ? "i-r-flow-use-most-balanced-minimum-cut" : "r-flow-use-most-balanced-minimum-cut"),
    po::value<bool>((initial_partitioning ? &context.initial_partitioning.local_search.flow.use_most_balanced_minimum_cut : &context.local_search.flow.use_most_balanced_minimum_cut))->value_name("<bool>"),
    "Heuristic to balance a min-cut bipartition after a maximum flow computation \n"
    "(default: true)")
    ((initial_partitioning ? "i-r-flow-use-adaptive-alpha-stopping-rule" : "r-flow-use-adaptive-alpha-stopping-rule"),
    po::value<bool>((initial_partitioning ? &context.initial_partitioning.local_search.flow.use_adaptive_alpha_stopping_rule : &context.local_search.flow.use_adaptive_alpha_stopping_rule))->value_name("<bool>"),
    "Stop adaptive flow iterations, when cut equal to old cut \n"
    "(default: true)")
    ((initial_partitioning ? "i-r-flow-ignore-small-hyperedge-cut" : "r-flow-ignore-small-hyperedge-cut"),
    po::value<bool>((initial_partitioning ? &context.initial_partitioning.local_search.flow.ignore_small_hyperedge_cut : &context.local_search.flow.ignore_small_hyperedge_cut))->value_name("<bool>"),
    "If cut is small between two blocks, don't use flow refinement \n"
    "(default: true)")
    ((initial_partitioning ? "i-r-flow-use-improvement-history" : "r-flow-use-improvement-history"),
    po::value<bool>((initial_partitioning ? &context.initial_partitioning.local_search.flow.use_improvement_history : &context.local_search.flow.use_improvement_history))->value_name("<bool>"),
    "Decides if flow-based refinement is used between two adjacent blocks based on improvement history of the corresponding blocks \n"
    "(default: true)");
  return options;
}

po::options_description createCoarseningOptionsDescription(Context& context,
                                                           const int num_columns,
                                                           const bool initial_partitioning) {
  po::options_description options((initial_partitioning ? "Initial Partitioning Coarsening Options" :
                                   "Coarsening Options"), num_columns);
  options.add_options()
    ((initial_partitioning ? "i-c-type" : "c-type"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& ctype) {
      if (initial_partitioning) {
        context.initial_partitioning.coarsening.algorithm = kahypar::coarseningAlgorithmFromString(ctype);
      } else {
        context.coarsening.algorithm = kahypar::coarseningAlgorithmFromString(ctype);
      }
    }),
    "Algorithm:\n"
    " - ml_style\n"
    " - heavy_full\n"
    " - heavy_lazy")
    ((initial_partitioning ? "i-c-s" : "c-s"),
    po::value<double>((initial_partitioning ? &context.initial_partitioning.coarsening.max_allowed_weight_multiplier : &context.coarsening.max_allowed_weight_multiplier))->value_name("<double>"),
    "The maximum weight of a vertex in the coarsest hypergraph H is:\n"
    "(s * w(H)) / (t * k)\n")
    ((initial_partitioning ? "i-c-t" : "c-t"),
    po::value<HypernodeID>((initial_partitioning ? &context.initial_partitioning.coarsening.contraction_limit_multiplier : &context.coarsening.contraction_limit_multiplier))->value_name("<int>"),
    "Coarsening stops when there are no more than t * k hypernodes left")
    ((initial_partitioning ? "i-c-rating-score" : "c-rating-score"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& rating_score) {
      if (initial_partitioning) {
        context.initial_partitioning.coarsening.rating.rating_function =
          kahypar::ratingFunctionFromString(rating_score);
      } else {
        context.coarsening.rating.rating_function =
          kahypar::ratingFunctionFromString(rating_score);
      }
    }),
    "Rating function used to calculate scores for vertex pairs:\n"
    "heavy_edge "
    "edge_frequency")
    ((initial_partitioning ? "i-c-rating-use-communities" : "c-rating-use-communities"),
    po::value<bool>()->value_name("<bool>")->notifier(
      [&context, initial_partitioning](bool use_communities) {
      if (initial_partitioning) {
        context.initial_partitioning.coarsening.rating.community_policy =
          (use_communities ? CommunityPolicy::use_communities :
           CommunityPolicy::ignore_communities);
      } else {
        context.coarsening.rating.community_policy =
          (use_communities ? CommunityPolicy::use_communities :
           CommunityPolicy::ignore_communities);
      }
    }),
    "Use community information during rating. If c-rating-use-communities=true ,\n"
    "only neighbors belonging to the same community will be considered as contraction partner.")
    ((initial_partitioning ? "i-c-rating-heavy_node_penalty" : "c-rating-heavy_node_penalty"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& penalty) {
      if (initial_partitioning) {
        context.initial_partitioning.coarsening.rating.heavy_node_penalty_policy =
          kahypar::heavyNodePenaltyFromString(penalty);
      } else {
        context.coarsening.rating.heavy_node_penalty_policy =
          kahypar::heavyNodePenaltyFromString(penalty);
      }
    }),
    "Penalty function to discourage heavy vertices:\n"
    "multiplicative "
    "no_penalty")
    ((initial_partitioning ? "i-c-rating-acceptance-criterion" : "c-rating-acceptance-criterion"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& crit) {
      if (initial_partitioning) {
        context.initial_partitioning.coarsening.rating.acceptance_policy =
          kahypar::acceptanceCriterionFromString(crit);
      } else {
        context.coarsening.rating.acceptance_policy =
          kahypar::acceptanceCriterionFromString(crit);
      }
    }),
    "Acceptance/Tiebreaking criterion for contraction partners having the same score:\n"
    "random "
    "prefer_unmatched")
    ((initial_partitioning ? "i-c-fixed-vertex-acceptance-criterion" : "c-fixed-vertex-acceptance-criterion"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& crit) {
      if (initial_partitioning) {
        context.initial_partitioning.coarsening.rating.fixed_vertex_acceptance_policy =
          kahypar::fixedVertexAcceptanceCriterionFromString(crit);
      } else {
        context.coarsening.rating.fixed_vertex_acceptance_policy =
          kahypar::fixedVertexAcceptanceCriterionFromString(crit);
      }
    }),
    "Acceptance criterion for fixed vertex contractions:\n"
    "- free_vertex_only     : Allows (free, free) and (fixed, free)\n"
    "- fixed_vertex_allowed : Allows (free, free), (fixed, free), and (fixed, fixed) \n"
    "- equivalent_vertices  : Allows (free, free), (fixed, fixed)");
  return options;
}

po::options_description createRefinementOptionsDescription(Context& context,
                                                           const int num_columns,
                                                           const bool initial_partitioning) {
  po::options_description options((initial_partitioning ? "Initial Partitioning Refinement Options" :
                                   "Refinement Options"), num_columns);
  options.add_options()
    ((initial_partitioning ? "i-r-type" : "r-type"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& rtype) {
      if (initial_partitioning) {
        context.initial_partitioning.local_search.algorithm = kahypar::refinementAlgorithmFromString(rtype);
      } else {
        context.local_search.algorithm = kahypar::refinementAlgorithmFromString(rtype);
      }
    }),
    "Local Search Algorithm:\n"
    " - twoway_fm        : 2-way FM algorithm        (recursive bisection: cut & km1)\n"
    " - twoway_flow      : 2-way Flow algorithm      (recursive bisection: cut & km1)\n"
    " - twoway_fm_flow   : 2-way FM + Flow algorithm (recursive bisection: cut & km1)\n"
    " - kway_fm          : k-way FM algorithm        (direct k-way       : cut)\n"
    " - kway_fm_flow     : k-way FM + Flow algorithm (direct k-way       : cut)\n"
    " - kway_fm_km1      : k-way FM algorithm        (direct k-way       : km1)\n"
    " - kway_fm_flow_km1 : k-way FM + Flow algorithm (direct k-way       : km1)\n"
    " - kway_flow        : k-way Flow algorithm      (direct k-way       : cut & km1)")
    ((initial_partitioning ? "i-r-runs" : "r-runs"),
    po::value<int>((initial_partitioning ? &context.initial_partitioning.local_search.iterations_per_level : &context.local_search.iterations_per_level))->value_name("<int>")->notifier(
      [&context, initial_partitioning](const int) {
      if (initial_partitioning) {
        if (context.initial_partitioning.local_search.iterations_per_level == -1) {
          context.initial_partitioning.local_search.iterations_per_level = std::numeric_limits<int>::max();
        }
      } else {
        if (context.local_search.iterations_per_level == -1) {
          context.local_search.iterations_per_level = std::numeric_limits<int>::max();
        }
      }
    }),
    "Max. # local search repetitions on each level\n"
    "(no limit:-1)")
    ((initial_partitioning ? "i-r-fm-stop" : "r-fm-stop"),
    po::value<std::string>()->value_name("<string>")->notifier(
      [&context, initial_partitioning](const std::string& stopfm) {
      if (initial_partitioning) {
        context.initial_partitioning.local_search.fm.stopping_rule = kahypar::stoppingRuleFromString(stopfm);
      } else {
        context.local_search.fm.stopping_rule = kahypar::stoppingRuleFromString(stopfm);
      }
    }),
    "Stopping Rule for Local Search: \n"
    " - adaptive_opt: ALENEX'17 adaptive stopping rule \n"
    " - simple:       ALENEX'16 threshold based on r-fm-stop-i")
    ((initial_partitioning ? "i-r-fm-stop-i" : "r-fm-stop-i"),
    po::value<uint32_t>((initial_partitioning ? &context.initial_partitioning.local_search.fm.max_number_of_fruitless_moves : &context.local_search.fm.max_number_of_fruitless_moves))->value_name("<uint32_t>"),
    "Max. # fruitless moves before stopping local search using simple stopping rule")
    ((initial_partitioning ? "i-r-fm-stop-alpha" : "r-fm-stop-alpha"),
    po::value<double>((initial_partitioning ? &context.initial_partitioning.local_search.fm.adaptive_stopping_alpha : &context.local_search.fm.adaptive_stopping_alpha))->value_name("<double>"),
    "Parameter alpha for adaptive stopping rule \n"
    "(infinity: -1)");
  options.add(createFlowRefinementOptionsDescription(context, num_columns, initial_partitioning));
  return options;
}

po::options_description createInitialPartitioningOptionsDescription(Context& context,
                                                                    const int num_columns) {
  po::options_description options("Initial Partitioning Options", num_columns);
  options.add_options()
    ("i-mode",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_mode) {
      context.initial_partitioning.mode = kahypar::modeFromString(ip_mode);
    }),
    "IP mode: \n"
    " - (recursive) bisection  \n"
    " - (direct) k-way")
    ("i-technique",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_technique) {
      context.initial_partitioning.technique =
        kahypar::inititalPartitioningTechniqueFromString(ip_technique);
    }),
    "IP Technique:\n"
    " - flat\n"
    " - (multi)level")
    ("i-algo",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_algo) {
      context.initial_partitioning.algo =
        kahypar::initialPartitioningAlgorithmFromString(ip_algo);
    }),
    "Algorithm used to create initial partition: pool ")
    ("i-runs",
    po::value<uint32_t>(&context.initial_partitioning.nruns)->value_name("<uint32_t>"),
    "# initial partition trials");
  options.add(createCoarseningOptionsDescription(context, num_columns, true));
  options.add(createRefinementOptionsDescription(context, num_columns, true));
  return options;
}


po::options_description createPreprocessingOptionsDescription(Context& context,
                                                              const int num_columns) {
  po::options_description options("Preprocessing Options", num_columns);
  options.add_options()
    ("p-use-sparsifier",
    po::value<bool>(&context.preprocessing.enable_min_hash_sparsifier)->value_name("<bool>"),
    "Use min-hash pin sparsifier before partitioning")
    ("p-sparsifier-min-median-he-size",
    po::value<HypernodeID>(&context.preprocessing.min_hash_sparsifier.min_median_he_size)->value_name("<int>"),
    "Minimum median hyperedge size necessary for sparsifier application")
    ("p-sparsifier-max-hyperedge-size",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.max_hyperedge_size)->value_name("<int>"),
    "Max hyperedge size allowed considered by sparsifier")
    ("p-sparsifier-max-cluster-size",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.max_cluster_size)->value_name("<int>"),
    "Max cluster size which is built by sparsifier")
    ("p-sparsifier-min-cluster-size",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.min_cluster_size)->value_name("<int>"),
    "Min cluster size which is built by sparsifier")
    ("p-sparsifier-num-hash-func",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.num_hash_functions)->value_name("<int>"),
    "Number of hash functions")
    ("p-sparsifier-combined-num-hash-func",
    po::value<uint32_t>(&context.preprocessing.min_hash_sparsifier.combined_num_hash_functions)->value_name("<int>"),
    "Number of combined hash functions")
    ("p-detect-communities",
    po::value<bool>(&context.preprocessing.enable_community_detection)->value_name("<bool>"),
    "Using louvain community detection for coarsening")
    ("p-detect-communities-in-ip",
    po::value<bool>(&context.preprocessing.community_detection.enable_in_initial_partitioning)->value_name("<bool>"),
    "Using louvain community detection for coarsening during initial partitioning")
    ("p-max-louvain-pass-iterations",
    po::value<uint32_t>(&context.preprocessing.community_detection.max_pass_iterations)->value_name("<uint32_t>"),
    "Maximum number of iterations over all nodes of one louvain pass")
    ("p-min-eps-improvement",
    po::value<long double>(&context.preprocessing.community_detection.min_eps_improvement)->value_name("<long double>"),
    "Minimum improvement of quality during a louvain pass which leads to further passes")
    ("p-louvain-edge-weight",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ptype) {
      context.preprocessing.community_detection.edge_weight = kahypar::edgeWeightFromString(ptype);
    }),
    "Weights:\n"
    " - hybrid \n"
    " - uniform\n"
    " - non_uniform\n"
    " - degree")
    ("p-reuse-communities",
    po::value<bool>(&context.preprocessing.community_detection.reuse_communities)->value_name("<bool>"),
    "Reuse the community structure identified in the first bisection for all other bisections.");
  return options;
}

po::options_description createEvolutionaryOptionsDescription(Context& context,
                                                             const int num_columns) {
  po::options_description evolutionary_options("Evolutionary Options", num_columns);
  evolutionary_options.add_options()
    ("partition-evolutionary",
    po::value<bool>(&context.partition_evolutionary)->value_name("<bool>"),
    "Use memetic algorithm for partitioning")
    ("population-size",
    po::value<size_t>()->value_name("<size_t>")->notifier(
      [&](const size_t& pop_size) {
      context.evolutionary.population_size = pop_size;
    }),
    "Population Size for Evolutionary Partitioning\n"
    "(default 10)")
    ("gamma",
    po::value<double>()->value_name("<double>")->notifier(
      [&](const double gamma) {
      context.evolutionary.gamma = gamma;
    }),
    "The dampening factor for edge frequency\n"
    "(default 0.5)")
    ("replace-strategy",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& replace_strat) {
      context.evolutionary.replace_strategy = kahypar::replaceStrategyFromString(replace_strat);
    }),
    "Replacement Strategy for Population Management\n"
    "- worst: new partitions replace the current worst partition in the population\n"
    "- diverse: new partitions replace the most similar partition based on cut difference\n"
    "- strong-diverse: new partitions replace the most similar partition based on connectivity difference\n"
    "(for diverse/strong-diverse better partitions are not considered for replacement)\n"
    "(default: strong-diverse)")
    ("combine-strategy",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& combine_strat) {
      context.evolutionary.combine_strategy = kahypar::combineStrategyFromString(combine_strat);
    }),
    "Combine Strategy to be used for a regular combine operation\n"
    "- basic: takes two partitions and contracts nodes u & v only if they are in the same block for both partitions \n"
    "- with-edge-frequency: similar to basic, but the edge frequency information of the best \"edge_frequency_amount\" partitions is added top the rating\n"
    "- edge-frequency: creating a new partition under consideration of the \"edge_frequency_amount\" best partitions for edge frequency\n"
    "(default: basic)")
    ("mutate-strategy",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& mutate_strat) {
      context.evolutionary.mutate_strategy = kahypar::mutateStrategyFromString(mutate_strat);
    }),
    "Mutation Strategy for the mutation operation \n"
    "- new-initial-partitioning-vcycle: coarsening of a partition with completely new initial partitioning\n"
    "- vcycle: a regular vcycle on an existing partition\n"
    "(default: new-initial-partitioning-vcycle)")
    ("diversify-interval",
    po::value<int>()->value_name("<int>")->notifier(
      [&](const int& div_interval) {
      context.evolutionary.diversify_interval = div_interval;
    }),
    "The Frequency in which diversfication should be performed\n"
    "(default: -1)(-1 disables)")
    ("random-vcycles",
    po::value<bool>()->value_name("<bool>")->notifier(
      [&](const bool& random_vcycle) {
      context.evolutionary.random_vcycles = random_vcycle;
    }),
    "Whether vcycle mutations should be randomized")
    ("dynamic-population-size",
    po::value<bool>()->value_name("<bool>")->notifier(
      [&](const bool& dynamic_pop) {
      context.evolutionary.dynamic_population_size = dynamic_pop;
    }),
    "Whether the population size should be determined by runtime\n"
    "default: on)")
    ("dynamic-population-time",
    po::value<float>()->value_name("<float>")->notifier(
      [&](const float& dynamic_pop_time) {
      context.evolutionary.dynamic_population_amount_of_time = dynamic_pop_time;
    }),
    "The amount of total runtime allocated for initial population\n"
    "default: 0.15)")
    ("random-combine",
    po::value<bool>()->value_name("<bool>")->notifier(
      [&](const bool& random_combine) {
      context.evolutionary.random_combine_strategy = random_combine;
    }),
    "Whether random combines should be picked\n"
    "default: off)")
    ("unlimited-coarsening",
    po::value<bool>()->value_name("<bool>")->notifier(
      [&](const bool& unlimited_c) {
      context.evolutionary.unlimited_coarsening_contraction = unlimited_c;
    }),
    "Whether combine operations should not be limited in contraction\n"
    "default: on)")
    ("mutate-chance",
    po::value<float>()->value_name("<float>")->notifier(
      [&](const float& mutate_chance) {
      context.evolutionary.mutation_chance = mutate_chance;
    }),
    "The Chance of a mutation being selected as operation\n"
    "default: 0.5)")
    ("edge-frequency-chance",
    po::value<float>()->value_name("<float>")->notifier(
      [&](const float& edge_chance) {
      context.evolutionary.edge_frequency_chance = edge_chance;
    }),
    "The Chance of a mutation being selected as operation\n"
    "default: 0.5)");
  return evolutionary_options;
}

po::options_description createGenericOptionsDescription(Context& context,
                                                        const int num_columns) {
  po::options_description generic_options("Generic Options", num_columns);
  generic_options.add_options()
    ("help", "show help message")
    ("verbose,v", po::value<bool>(&context.partition.verbose_output)->value_name("<bool>"),
    "Verbose main partitioning output")
    ("vip", po::value<bool>(&context.initial_partitioning.verbose_output)->value_name("<bool>"),
    "Verbose initial partitioning output")
    ("quiet,q", po::value<bool>(&context.partition.quiet_mode)->value_name("<bool>"),
    "Quiet Mode: Completely suppress console output")
    ("time-limit", po::value<int>(&context.partition.time_limit)->value_name("<int>"),
    "Time limit in seconds")
    ("sp-process,s", po::value<bool>(&context.partition.sp_process_output)->value_name("<bool>"),
    "Summarize partitioning results in RESULT line compatible with sqlplottools "
    "(https://github.com/bingmann/sqlplottools)");
  return generic_options;
}

void processCommandLineInput(Context& context, int argc, char* argv[]) {
  const int num_columns = platform::getTerminalWidth();

  po::options_description generic_options = createGenericOptionsDescription(context,
                                                                            num_columns);

  po::options_description required_options("Required Options", num_columns);
  required_options.add_options()
    ("hypergraph,h",
    po::value<std::string>(&context.partition.graph_filename)->value_name("<string>")->required(),
    "Hypergraph filename")
    ("blocks,k",
    po::value<PartitionID>(&context.partition.k)->value_name("<int>")->required()->notifier(
      [&](const PartitionID) {
      context.partition.rb_lower_k = 0;
      context.partition.rb_upper_k = 0;
    }),
    "Number of blocks")
    ("epsilon,e",
    po::value<double>(&context.partition.epsilon)->value_name("<double>")->required(),
    "Imbalance parameter epsilon");

  std::string context_path;
  po::options_description preset_options("Preset Options", num_columns);
  preset_options.add_options()
    ("preset,p", po::value<std::string>(&context_path)->value_name("<string>"),
    "Context Presets (see config directory):\n"
    " - km1_direct_kway_sea18.ini\n"
    " - km1_direct_kway_sea17.ini\n"
    " - direct_kway_km1_alenex17.ini\n"
    " - rb_cut_alenex16.ini\n"
    " - <path-to-custom-ini-file>");

  po::options_description general_options = createGeneralOptionsDescription(context, num_columns);

  po::options_description preprocessing_options =
    createPreprocessingOptionsDescription(context, num_columns);

  po::options_description coarsening_options = createCoarseningOptionsDescription(context,
                                                                                  num_columns,
                                                                                  false);


  po::options_description ip_options = createInitialPartitioningOptionsDescription(context,
                                                                                   num_columns);


  po::options_description refinement_options =
    createRefinementOptionsDescription(context, num_columns, false);

  po::options_description evolutionary_options =
    createEvolutionaryOptionsDescription(context, num_columns);

  po::options_description cmd_line_options;
  cmd_line_options.add(generic_options)
  .add(required_options)
  .add(preset_options)
  .add(general_options)
  .add(preprocessing_options)
  .add(coarsening_options)
  .add(ip_options)
  .add(refinement_options)
  .add(evolutionary_options);

  po::variables_map cmd_vm;
  po::store(po::parse_command_line(argc, argv, cmd_line_options), cmd_vm);

  // placing vm.count("help") here prevents required attributes raising an
  // error if only help was supplied
  if (cmd_vm.count("help") != 0 || argc == 1) {
    kahypar::io::printBanner(context);
    LOG << cmd_line_options;
    exit(0);
  }

  po::notify(cmd_vm);

  std::ifstream file(context_path.c_str());
  if (!file) {
    std::cerr << "Could not load context file at: " << context_path << std::endl;
    std::exit(-1);
  }

  po::options_description ini_line_options;
  ini_line_options.add(general_options)
  .add(preprocessing_options)
  .add(coarsening_options)
  .add(ip_options)
  .add(refinement_options)
  .add(evolutionary_options);

  po::store(po::parse_config_file(file, ini_line_options, true), cmd_vm);
  po::notify(cmd_vm);


  std::string epsilon_str = std::to_string(context.partition.epsilon);
  epsilon_str.erase(epsilon_str.find_last_not_of('0') + 1, std::string::npos);

  context.partition.graph_partition_filename =
    context.partition.graph_filename
    + ".part"
    + std::to_string(context.partition.k)
    + ".epsilon"
    + epsilon_str
    + ".seed"
    + std::to_string(context.partition.seed)
    + ".KaHyPar";

  if (context.partition.use_individual_part_weights) {
    context.partition.epsilon = 0;
  }
}


void parseIniToContext(Context& context, const std::string& ini_filename) {
  std::ifstream file(ini_filename.c_str());
  if (!file) {
    std::cerr << "Could not load context file at: " << ini_filename << std::endl;
    std::exit(-1);
  }
  const int num_columns = 80;

  po::variables_map cmd_vm;
  po::options_description ini_line_options;
  ini_line_options.add(createGeneralOptionsDescription(context, num_columns))
  .add(createGenericOptionsDescription(context, num_columns))
  .add(createPreprocessingOptionsDescription(context, num_columns))
  .add(createCoarseningOptionsDescription(context, num_columns, false))
  .add(createInitialPartitioningOptionsDescription(context, num_columns))
  .add(createRefinementOptionsDescription(context, num_columns, false))
  .add(createEvolutionaryOptionsDescription(context, num_columns));

  po::store(po::parse_config_file(file, ini_line_options, true), cmd_vm);
  po::notify(cmd_vm);

  if (context.partition.use_individual_part_weights) {
    context.partition.epsilon = 0;
  }
}
}  // namespace kahypar
