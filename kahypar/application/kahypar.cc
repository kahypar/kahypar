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

#include <boost/program_options.hpp>

#if defined(_MSC_VER)
#include <Windows.h>
#include <process.h>
#else
#include <sys/ioctl.h>
#endif

#include <cctype>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/io/partitioning_output.h"
#include "kahypar/io/sql_plottools_serializer.h"
#include "kahypar/kahypar.h"
#include "kahypar/macros.h"
#include "kahypar/utils/math.h"
#include "kahypar/utils/randomize.h"

namespace po = boost::program_options;

using kahypar::HypernodeID;
using kahypar::HyperedgeID;
using kahypar::PartitionID;
using kahypar::Hypergraph;
using kahypar::HighResClockTimepoint;
using kahypar::Partitioner;
using kahypar::Configuration;
using kahypar::Mode;
using kahypar::Objective;
using kahypar::LouvainEdgeWeight;
using kahypar::RefinementAlgorithm;
using kahypar::GlobalRebalancingMode;
using kahypar::InitialPartitioningTechnique;

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

void checkRecursiveBisectionMode(RefinementAlgorithm& algo) {
  if (algo == RefinementAlgorithm::kway_fm) {
    std::cout << "WARNING: local search algorithm is set to "
    << toString(algo)
    << ". However " << toString(RefinementAlgorithm::twoway_fm)
    << "  is better and faster." << std::endl;
    std::cout << "Should the local search algorithm be changed to "
    << toString(RefinementAlgorithm::twoway_fm) << " (Y/N)?" << std::endl;
    char answer = 'N';
    std::cin >> answer;
    answer = std::toupper(answer);
    if (answer == 'Y') {
      std::cout << "Changing local search algorithm to "
      << toString(RefinementAlgorithm::twoway_fm) << std::endl;
      algo = RefinementAlgorithm::twoway_fm;
    }
  }
}

void checkDirectKwayMode(RefinementAlgorithm& algo) {
  if (algo == RefinementAlgorithm::twoway_fm) {
    std::cout << "WARNING: local search algorithm is set to "
    << toString(algo)
    << ". This algorithm cannot be used for direct k-way partitioning with k>2."
    << std::endl;
    std::cout << "Should the local search algorithm be changed to "
    << toString(RefinementAlgorithm::kway_fm) << " (Y/N)?" << std::endl;
    char answer = 'N';
    std::cin >> answer;
    answer = std::toupper(answer);
    if (answer == 'Y') {
      std::cout << "Changing local search algorithm to "
      << toString(RefinementAlgorithm::kway_fm) << std::endl;
      algo = RefinementAlgorithm::kway_fm;
    }
  }
}


void sanityCheck(Configuration& config) {
  switch (config.partition.mode) {
    case Mode::recursive_bisection:
      // Prevent configurations that currently don't make sense.
      // If KaHyPar operates in recursive bisection mode, than the
      // initial partitioning algorithm is called on the coarsest graph to
      // create a bipartition (i.e. it is only called for k=2).
      // Therefore, the initial partitioning algorithm
      // should run in direct mode and not in recursive bisection mode (since there
      // is nothing left to bisect).
      ALWAYS_ASSERT(config.initial_partitioning.mode == Mode::direct_kway,
                    toString(config.initial_partitioning.mode));
      // Furthermore, the IP algorithm should not use the multilevel technique itself,
      // because the hypergraph is already coarse enough for initial partitioning.
      // Therefore we prevent further coarsening by enforcing flat rather than multilevel.
      // If initial partitioning is set to direct k-way, it does not make sense to use
      // multilevel as initial partitioning technique, because in this case KaHyPar
      // could just do the additional multilevel coarsening and then call the initial
      // partitioning algorithm as a flat algorithm.
      ALWAYS_ASSERT(config.initial_partitioning.technique == InitialPartitioningTechnique::flat,
                    toString(config.initial_partitioning.technique));
      checkRecursiveBisectionMode(config.local_search.algorithm);
      break;
    case Mode::direct_kway:
      // When KaHyPar runs in direct k-way mode, it makes no sense to use the initial
      // partitioner in direct multilevel mode, because we could essentially get the same
      // behavior if we would just use a smaller contraction limit in the main partitioner.
      // Since the contraction limits for main and initial partitioner are specifically tuned,
      // we currently forbid this configuration.
      ALWAYS_ASSERT(config.initial_partitioning.mode != Mode::direct_kway ||
                    config.initial_partitioning.technique == InitialPartitioningTechnique::flat,
                    toString(config.initial_partitioning.mode)
                    << " " << toString(config.initial_partitioning.technique));
      checkDirectKwayMode(config.local_search.algorithm);
      break;
  }
  switch (config.initial_partitioning.mode) {
    case Mode::recursive_bisection:
      checkRecursiveBisectionMode(config.initial_partitioning.local_search.algorithm);
      break;
    case Mode::direct_kway:
      // If the main partitioner runs in recursive bisection mode, then the initial
      // partitioner running in direct mode can use two-way FM as a local search
      // algorithm because we only perform bisections.
      if (config.partition.mode != Mode::recursive_bisection) {
        checkDirectKwayMode(config.initial_partitioning.local_search.algorithm);
      }
      break;
  }
}


void processCommandLineInput(Configuration& config, int argc, char* argv[]) {
  const int num_columns = getTerminalWidth();

  po::options_description generic_options("Generic Options", num_columns);
  generic_options.add_options()
    ("help", "show help message")
    ("verbose,v", po::value<bool>(&config.partition.verbose_output)->value_name("<bool>"),
    "Verbose partitioning output");

  po::options_description required_options("Required Options", num_columns);
  required_options.add_options()
    ("hypergraph,h",
    po::value<std::string>(&config.partition.graph_filename)->value_name("<string>")->required(),
    "Hypergraph filename")
    ("blocks,k",
    po::value<PartitionID>(&config.partition.k)->value_name("<int>")->required()->notifier(
      [&](const PartitionID) {
    config.partition.rb_lower_k = 0;
    config.partition.rb_upper_k = config.partition.k - 1;
  }),
    "Number of blocks")
    ("epsilon,e",
    po::value<double>(&config.partition.epsilon)->value_name("<double>")->required(),
    "Imbalance parameter epsilon")
    ("objective,o",
    po::value<std::string>()->value_name("<string>")->required()->notifier([&](const std::string& s) {
    if (s == "cut") {
      config.partition.objective = Objective::cut;
    } else if (s == "km1") {
      config.partition.objective = Objective::km1;
    }
  }),
    "Objective: \n"
    " - cut : cut-net metric \n"
    " - km1 : (lambda-1) metric")
    ("mode,m",
    po::value<std::string>()->value_name("<string>")->required()->notifier(
      [&](const std::string& mode) {
    config.partition.mode = kahypar::modeFromString(mode);
  }),
    "Partitioning mode: \n"
    " - (recursive) bisection \n"
    " - (direct) k-way");

  std::string config_path;
  po::options_description preset_options("Preset Options", num_columns);
  preset_options.add_options()
    ("preset,p", po::value<std::string>(&config_path)->value_name("<string>"),
    "Configuration Presets:\n"
    " - direct_kway_km1_alenex17\n"
    " - rb_cut_alenex16\n"
    " - <path-to-custom-ini-file>");

  po::options_description general_options("General Options", num_columns);
  general_options.add_options()
    ("seed",
    po::value<int>(&config.partition.seed)->value_name("<int>"),
    "Seed for random number generator \n"
    "(default: -1)")
    ("cmaxnet",
    po::value<HyperedgeID>(&config.partition.hyperedge_size_threshold)->value_name("<int>")->notifier(
      [&](const HyperedgeID) {
    if (config.partition.hyperedge_size_threshold == -1) {
      config.partition.hyperedge_size_threshold = std::numeric_limits<HyperedgeID>::max();
    }
  }),
    "Hyperedges larger than cmaxnet are ignored during partitioning process. \n"
    "(default: -1, disabled)")
    ("vcycles",
    po::value<int>(&config.partition.global_search_iterations)->value_name("<int>"),
    "# V-cycle iterations for direct k-way partitioning \n"
    "(default: 0)");

  po::options_description preprocessing_options("Preprocessing Options", num_columns);
  preprocessing_options.add_options()
    ("p-use-sparsifier",
    po::value<bool>(&config.preprocessing.enable_min_hash_sparsifier)->value_name("<bool>"),
    "Use min-hash pin sparsifier before partitioning \n"
    "(default: false)")
    ("p-sparsifier-min-median-he-size",
    po::value<HypernodeID>(&config.preprocessing.min_hash_sparsifier.min_median_he_size)->value_name("<int>"),
    "Minimum median hyperedge size necessary for sparsifier application \n"
    "(default: 28)")
    ("p-sparsifier-max-hyperedge-size",
    po::value<uint32_t>(&config.preprocessing.min_hash_sparsifier.max_hyperedge_size)->value_name("<int>"),
    "Max hyperedge size allowed considered by sparsifier")
    ("p-sparsifier-max-cluster-size",
    po::value<uint32_t>(&config.preprocessing.min_hash_sparsifier.max_cluster_size)->value_name("<int>"),
    "Max cluster size which is built by sparsifier")
    ("p-sparsifier-min-cluster-size",
    po::value<uint32_t>(&config.preprocessing.min_hash_sparsifier.min_cluster_size)->value_name("<int>"),
    "Min cluster size which is built by sparsifier")
    ("p-sparsifier-num-hash-func",
    po::value<uint32_t>(&config.preprocessing.min_hash_sparsifier.num_hash_functions)->value_name("<int>"),
    "Number of hash functions")
    ("p-sparsifier-combined-num-hash-func",
    po::value<uint32_t>(&config.preprocessing.min_hash_sparsifier.combined_num_hash_functions)->value_name("<int>"),
    "Number of combined hash functions")
    ("p-parallel-net-removal",
    po::value<bool>(&config.preprocessing.remove_parallel_hes)->value_name("<bool>"),
    "Remove parallel hyperedges before partitioning \n"
    "(default: false)")
    ("p-large-net-removal",
    po::value<bool>(&config.preprocessing.remove_always_cut_hes)->value_name("<bool>"),
    "Remove hyperedges that will always be cut because"
    " of the weight of their pins \n"
    "(default: false)")
    ("p-use-louvain",
    po::value<bool>(&config.preprocessing.enable_louvain_community_detection)->value_name("<bool>"),
    "Using louvain community detection for coarsening\n"
    "(default: false)")
    ("p-use-louvain-in-ip",
    po::value<bool>(&config.preprocessing.louvain_community_detection.enable_in_initial_partitioning)->value_name("<bool>"),
    "Using louvain community detection for coarsening during initial partitioning\n"
    "(default: false)")
    ("p-max-louvain-pass-iterations",
    po::value<int>(&config.preprocessing.louvain_community_detection.max_pass_iterations)->value_name("<int>"),
    "Maximum number of iterations over all nodes of one louvain pass\n"
    "(default: 100)")
    ("p-min-eps-improvement",
    po::value<long double>(&config.preprocessing.louvain_community_detection.min_eps_improvement)->value_name("<long double>"),
    "Minimum improvement of quality during a louvain pass which leads to further passes\n"
    "(default: 0.001)")
    ("p-louvain-edge-weight",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ptype) {
    config.preprocessing.louvain_community_detection.edge_weight = kahypar::edgeWeightFromString(ptype);
  }),
    "Weights:\n"
    " - hybrid \n"
    " - uniform\n"
    " - non_uniform\n"
    " - degree \n"
    "(default: hybrid)")
    ("p-louvain-use-bipartite-graph",
    po::value<bool>(&config.preprocessing.louvain_community_detection.use_bipartite_graph)->value_name("<bool>"),
    "If true, hypergraph is transformed into bipartite graph. If false, hypergraph is transformed into clique graph.\n"
    "(default: true)");

  po::options_description coarsening_options("Coarsening Options", num_columns);
  coarsening_options.add_options()
    ("c-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ctype) {
    config.coarsening.algorithm = kahypar::coarseningAlgorithmFromString(ctype);
  }),
    "Algorithm:\n"
    " - ml_style\n"
    " - heavy_full\n"
    " - heavy_lazy \n"
    "(default: ml_style)")
    ("c-s",
    po::value<double>(&config.coarsening.max_allowed_weight_multiplier)->value_name("<double>"),
    "The maximum weight of a vertex in the coarsest hypergraph H is:\n"
    "(s * w(H)) / (t * k)\n"
    "(default: 1)")
    ("c-t",
    po::value<HypernodeID>(&config.coarsening.contraction_limit_multiplier)->value_name("<int>"),
    "Coarsening stops when there are no more than t * k hypernodes left\n"
    "(default: 160)");


  po::options_description ip_options("Initial Partitioning Options", num_columns);
  ip_options.add_options()
    ("i-mode",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_mode) {
    config.initial_partitioning.mode = kahypar::modeFromString(ip_mode);
  }),
    "IP mode: \n"
    " - (recursive) bisection  \n"
    " - (direct) k-way\n"
    "(default: rb)")
    ("i-technique",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_technique) {
    config.initial_partitioning.technique =
      kahypar::inititalPartitioningTechniqueFromString(ip_technique);
  }),
    "IP Technique:\n"
    " - flat\n"
    " - (multi)level\n"
    "(default: multi)")
    ("i-algo",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_algo) {
    config.initial_partitioning.algo =
      kahypar::initialPartitioningAlgorithmFromString(ip_algo);
  }),
    "Algorithm used to create initial partition: pool (default)")
    ("i-c-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_ctype) {
    config.initial_partitioning.coarsening.algorithm =
      kahypar::coarseningAlgorithmFromString(ip_ctype);
  }),
    "IP Coarsening Algorithm:\n"
    " - ml_style\n"
    " - heavy_full\n"
    " - heavy_lazy \n"
    "(default: ml_style)")
    ("i-c-s",
    po::value<double>(&config.initial_partitioning.coarsening.max_allowed_weight_multiplier)->value_name("<double>"),
    "The maximum weight of a vertex in the coarsest hypergraph H is:\n"
    "(i-c-s * w(H)) / (i-c-t * k)\n"
    "(default: 1)")
    ("i-c-t",
    po::value<HypernodeID>(&config.initial_partitioning.coarsening.contraction_limit_multiplier)->value_name("<int>"),
    "IP coarsening stops when there are no more than i-c-t * k hypernodes left \n"
    "(default: 150)")
    ("i-runs",
    po::value<int>(&config.initial_partitioning.nruns)->value_name("<int>"),
    "# initial partition trials \n"
    "(default: 20)")
    ("i-r-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_rtype) {
    config.initial_partitioning.local_search.algorithm =
      kahypar::refinementAlgorithmFromString(ip_rtype);
  }),
    "IP Local Search Algorithm:\n"
    " - twoway_fm   : 2-way FM algorithm\n"
    " - kway_fm     : k-way FM algorithm\n"
    " - kway_fm_km1 : k-way FM algorithm optimizing km1 metric\n"
    " - sclap       : Size-constrained Label Propagation \n"
    "(default: twoway_fm)")
    ("i-r-fm-stop",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_stopfm) {
    config.initial_partitioning.local_search.fm.stopping_rule =
      kahypar::stoppingRuleFromString(ip_stopfm);
  }),
    "Stopping Rule for IP Local Search: \n"
    " - adaptive_opt: ALENEX'17 stopping rule \n"
    " - simple:       ALENEX'16 threshold based on i-r-i\n"
    "(default: simple)")
    ("i-r-fm-stop-i",
    po::value<int>(&config.initial_partitioning.local_search.fm.max_number_of_fruitless_moves)->value_name("<int>"),
    "Max. # fruitless moves before stopping local search \n"
    "(default: 50)")
    ("i-r-runs",
    po::value<int>(&config.initial_partitioning.local_search.iterations_per_level)->value_name("<int>")->notifier(
      [&](const int) {
    if (config.initial_partitioning.local_search.iterations_per_level == -1) {
      config.initial_partitioning.local_search.iterations_per_level =
        std::numeric_limits<int>::max();
    }
  }),
    "Max. # local search repetitions on each level \n"
    "(default:1, no limit:-1)");

  po::options_description refinement_options("Refinement Options", num_columns);
  refinement_options.add_options()
    ("r-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& rtype) {
    config.local_search.algorithm = kahypar::refinementAlgorithmFromString(rtype);
  }),
    "Local Search Algorithm:\n"
    " - twoway_fm   : 2-way FM algorithm\n"
    " - kway_fm     : k-way FM algorithm (cut) \n"
    " - kway_fm_km1 : k-way FM algorithm (km1)\n"
    " - sclap       : Size-constrained Label Propagation \n"
    "(default: twoway_fm)")
    ("r-runs",
    po::value<int>(&config.local_search.iterations_per_level)->value_name("<int>")->notifier(
      [&](const int) {
    if (config.local_search.iterations_per_level == -1) {
      config.local_search.iterations_per_level = std::numeric_limits<int>::max();
    }
  }),
    "Max. # local search repetitions on each level\n"
    "(default:1, no limit:-1)")
    ("r-sclap-runs",
    po::value<int>(&config.local_search.sclap.max_number_iterations)->value_name("<int>"),
    "Maximum # iterations for ScLaP-based refinement \n"
    "(default: -1 infinite)")
    ("r-fm-stop",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& stopfm) {
    config.local_search.fm.stopping_rule = kahypar::stoppingRuleFromString(stopfm);
  }),
    "Stopping Rule for Local Search: \n"
    " - adaptive_opt: ALENEX'17 stopping rule \n"
    " - adaptive1:    new nGP implementation \n"
    " - adaptive2:    original nGP implementation \n"
    " - simple:       threshold based on r-fm-stop-i \n"
    "(default: simple)")
    ("r-fm-stop-i",
    po::value<int>(&config.local_search.fm.max_number_of_fruitless_moves)->value_name("<int>"),
    "Max. # fruitless moves before stopping local search using simple stopping rule \n"
    "(default: 250)")
    ("r-fm-stop-alpha",
    po::value<double>(&config.local_search.fm.adaptive_stopping_alpha)->value_name("<double>"),
    "Parameter alpha for adaptive stopping rules \n"
    "(default: 1,infinity: -1)")
    ("r-fm-global-rebalancing",
    po::value<bool>()->value_name("<bool>")->notifier(
      [&](const bool global_rebalancing) {
    if (global_rebalancing) {
      config.local_search.fm.global_rebalancing = GlobalRebalancingMode::on;
    } else {
      config.local_search.fm.global_rebalancing = GlobalRebalancingMode::off;
    }
  }),
    "Use global rebalancing PQs in twoway_fm \n"
    "(default: false)");

  po::options_description cmd_line_options;
  cmd_line_options.add(generic_options)
  .add(required_options)
  .add(preset_options)
  .add(general_options)
  .add(preprocessing_options)
  .add(coarsening_options)
  .add(ip_options)
  .add(refinement_options);

  po::variables_map cmd_vm;
  po::store(po::parse_command_line(argc, argv, cmd_line_options), cmd_vm);

  // placing vm.count("help") here prevents required attributes raising an
  // error if only help was supplied
  if (cmd_vm.count("help") != 0) {
    std::cout << cmd_line_options << "n";
    exit(0);
  }

  po::notify(cmd_vm);

  std::ifstream file(config_path.c_str());
  if (!file) {
    std::cerr << "Could not load config file at: " << config_path << std::endl;
    std::exit(-1);
  }

  po::options_description ini_line_options;
  ini_line_options.add(general_options)
  .add(preprocessing_options)
  .add(coarsening_options)
  .add(ip_options)
  .add(refinement_options);

  po::store(po::parse_config_file(file, ini_line_options, true), cmd_vm);
  po::notify(cmd_vm);


  std::string epsilon_str = std::to_string(config.partition.epsilon);
  epsilon_str.erase(epsilon_str.find_last_not_of('0') + 1, std::string::npos);

  config.partition.graph_partition_filename =
    config.partition.graph_filename
    + ".part"
    + std::to_string(config.partition.k)
    + ".epsilon"
    + epsilon_str
    + ".seed"
    + std::to_string(config.partition.seed)
    + ".KaHyPar";
}

int main(int argc, char* argv[]) {
  Configuration config;

  processCommandLineInput(config, argc, argv);
  sanityCheck(config);

  if (config.partition.global_search_iterations != 0) {
    std::cerr << "Coarsened does not check if HNs are in same part." << std::endl;
    std::cerr << "Therefore v-cycles are currently disabled." << std::endl;
    std::exit(-1);
  }

  kahypar::Randomize::instance().setSeed(config.partition.seed);

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(config.partition.graph_filename,
                                          config.partition.k));

  if (config.preprocessing.enable_min_hash_sparsifier) {
    // determine whether or not to apply the sparsifier
    std::vector<HypernodeID> he_sizes;
    he_sizes.reserve(hypergraph.currentNumEdges());
    for (const auto& he : hypergraph.edges()) {
      he_sizes.push_back(hypergraph.edgeSize(he));
    }
    std::sort(he_sizes.begin(), he_sizes.end());
    if (kahypar::math::median(he_sizes) >=
        config.preprocessing.min_hash_sparsifier.min_median_he_size) {
      config.preprocessing.min_hash_sparsifier.is_active = true;
    }
  }

  if (config.preprocessing.enable_louvain_community_detection &&
      config.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::hybrid) {
    const double density = static_cast<double>(hypergraph.initialNumEdges()) /
                           static_cast<double>(hypergraph.initialNumNodes());
    if (density < 0.75) {
      config.preprocessing.louvain_community_detection.edge_weight = LouvainEdgeWeight::degree;
    } else {
      config.preprocessing.louvain_community_detection.edge_weight = LouvainEdgeWeight::uniform;
    }
  }


  if (config.partition.verbose_output) {
    kahypar::io::printHypergraphInfo(hypergraph,
                                     config.partition.graph_filename.substr(
                                       config.partition.graph_filename.find_last_of('/') + 1));
  }

  Partitioner partitioner;
  const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  partitioner.partition(hypergraph, config);
  const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;

#ifdef GATHER_STATS
  LOG("*******************************");
  LOG("***** GATHER_STATS ACTIVE *****");
  LOG("*******************************");
  kahypar::io::printPartitioningStatistics();
#endif

  kahypar::io::printPartitioningResults(hypergraph, config, elapsed_seconds);
  kahypar::io::writePartitionFile(hypergraph,
                                  config.partition.graph_partition_filename);

  kahypar::io::serializer::serialize(config, hypergraph, partitioner, elapsed_seconds);
  return 0;
}
