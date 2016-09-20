/***************************************************************************
 *  Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#include <boost/program_options.hpp>

#include <sys/ioctl.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include "definitions.h"
#include "io/hypergraph_io.h"
#include "io/partitioning_output.h"
#include "io/sql_plottools_serializer.h"
#include "macros.h"
#include "meta/registrar.h"
#include "partition/configuration.h"
#include "partition/factories.h"
#include "partition/initial_partitioning/initial_partitioning.h"
#include "partition/metrics.h"
#include "partition/partitioner.h"
#include "utils/randomize.h"

namespace po = boost::program_options;

using meta::Registrar;
using partition::Partitioner;
using partition::InitialPartitioner;
using partition::Configuration;
using partition::Mode;
using partition::Objective;
using partition::InitialPartitioningTechnique;
using partition::CoarseningAlgorithm;
using partition::RefinementAlgorithm;
using partition::InitialPartitionerAlgorithm;
using partition::RefinementStoppingRule;
using partition::GlobalRebalancingMode;
using partition::CoarsenerFactory;
using partition::RefinerFactory;
using partition::InitialPartitioningFactory;
using partition::DoNothingCoarsener;
using partition::RandomWinsFullCoarsener;
using partition::RandomWinsLazyUpdateCoarsener;
using partition::RandomWinsMLCoarsener;
using partition::TwoWayFMFactoryDispatcher;
using partition::KWayFMFactoryDispatcher;
using partition::KWayKMinusOneFactoryDispatcher;
// using partition::MaxGainNodeKWayFMFactoryDispatcher;
// using partition::MaxGainNodeKWayFMFactoryExecutor;
using partition::LPRefiner;
using partition::DoNothingRefiner;
using partition::IInitialPartitioner;
using partition::BFSInitialPartitioner;
using partition::LabelPropagationInitialPartitioner;
using partition::RandomInitialPartitioner;
using partition::GreedyHypergraphGrowingInitialPartitioner;
using partition::PoolInitialPartitioner;
using partition::FMGainComputationPolicy;
using partition::MaxPinGainComputationPolicy;
using partition::MaxNetGainComputationPolicy;
using partition::BFSStartNodeSelectionPolicy;
using partition::RoundRobinQueueSelectionPolicy;
using partition::GlobalQueueSelectionPolicy;
using partition::SequentialQueueSelectionPolicy;
using partition::ICoarsener;

REGISTER_COARSENER(CoarseningAlgorithm::heavy_lazy, RandomWinsLazyUpdateCoarsener);
REGISTER_COARSENER(CoarseningAlgorithm::heavy_full, RandomWinsFullCoarsener);
REGISTER_COARSENER(CoarseningAlgorithm::ml_style, RandomWinsMLCoarsener);
REGISTER_COARSENER(CoarseningAlgorithm::do_nothing, DoNothingCoarsener);

static Registrar<RefinerFactory> reg_twoway_fm_local_search(
  RefinementAlgorithm::twoway_fm,
  [](Hypergraph& hypergraph, const Configuration& config) {
  return TwoWayFMFactoryDispatcher::create(
    std::forward_as_tuple(hypergraph, config),
    PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
      config.local_search.fm.stopping_rule),
    PolicyRegistry<GlobalRebalancingMode>::getInstance().getPolicy(
      config.local_search.fm.global_rebalancing));
});

// static Registrar<RefinerFactory> reg_kway_fm_maxgain_local_search(
//   RefinementAlgorithm::kway_fm_maxgain,
//   [](Hypergraph& hypergraph, const Configuration& config) {
//   return MaxGainNodeKWayFMFactoryDispatcher::go(
//     PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
//       config.local_search.fm.stopping_rule),
//     NullPolicy(),
//     MaxGainNodeKWayFMFactoryExecutor(), hypergraph, config);
// });

static Registrar<RefinerFactory> reg_kway_fm_local_search(
  RefinementAlgorithm::kway_fm,
  [](Hypergraph& hypergraph, const Configuration& config) {
  return KWayFMFactoryDispatcher::create(
    std::forward_as_tuple(hypergraph, config),
    PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
      config.local_search.fm.stopping_rule));
});

static Registrar<RefinerFactory> reg_kway_km1_local_search(
  RefinementAlgorithm::kway_fm_km1,
  [](Hypergraph& hypergraph, const Configuration& config) {
  return KWayKMinusOneFactoryDispatcher::create(
    std::forward_as_tuple(hypergraph, config),
    PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
      config.local_search.fm.stopping_rule));
});

static Registrar<RefinerFactory> reg_lp_local_search(
  RefinementAlgorithm::label_propagation,
  [](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* {
  return new LPRefiner(hypergraph, config);
});

static Registrar<RefinerFactory> reg_do_nothing_refiner(
  RefinementAlgorithm::do_nothing,
  [](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* {
  (void)hypergraph;
  (void)config;                  // Fixing unused parameter warning
  return new DoNothingRefiner();
});

REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::simple,
                NumberOfFruitlessMovesStopsSearch);
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::adaptive_opt,
                AdvancedRandomWalkModelStopsSearch);
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::adaptive1,
                RandomWalkModelStopsSearch);
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::adaptive2,
                nGPRandomWalkStopsSearch);

REGISTER_POLICY(GlobalRebalancingMode, GlobalRebalancingMode::on,
                GlobalRebalancing);
REGISTER_POLICY(GlobalRebalancingMode, GlobalRebalancingMode::off,
                NoGlobalRebalancing);

static Registrar<InitialPartitioningFactory> reg_random(
  InitialPartitionerAlgorithm::random,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new RandomInitialPartitioner(hypergraph, config);
});

static Registrar<InitialPartitioningFactory> reg_bfs(
  InitialPartitionerAlgorithm::bfs,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new BFSInitialPartitioner<BFSStartNodeSelectionPolicy<> >(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_lp(
  InitialPartitionerAlgorithm::lp,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                FMGainComputationPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy(
  InitialPartitionerAlgorithm::greedy_sequential,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       FMGainComputationPolicy,
                                                       SequentialQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_global(
  InitialPartitionerAlgorithm::greedy_global,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       FMGainComputationPolicy,
                                                       GlobalQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_round(
  InitialPartitionerAlgorithm::greedy_round,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       FMGainComputationPolicy,
                                                       RoundRobinQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_maxpin(
  InitialPartitionerAlgorithm::greedy_sequential_maxpin,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       MaxPinGainComputationPolicy,
                                                       SequentialQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_global_maxpin(
  InitialPartitionerAlgorithm::greedy_global_maxpin,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       MaxPinGainComputationPolicy,
                                                       GlobalQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_round_maxpin(
  InitialPartitionerAlgorithm::greedy_round_maxpin,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       MaxPinGainComputationPolicy,
                                                       RoundRobinQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_maxnet(
  InitialPartitionerAlgorithm::greedy_sequential_maxnet,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       MaxNetGainComputationPolicy,
                                                       SequentialQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_global_maxnet(
  InitialPartitionerAlgorithm::greedy_global_maxnet,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       MaxNetGainComputationPolicy,
                                                       GlobalQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_round_maxnet(
  InitialPartitionerAlgorithm::greedy_round_maxnet,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                       MaxNetGainComputationPolicy,
                                                       RoundRobinQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_pool(
  InitialPartitionerAlgorithm::pool,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new PoolInitialPartitioner(hypergraph, config);
});

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
  struct winsize w;
  ioctl(0, TIOCGWINSZ, &w);

  po::options_description generic_options("Generic Options", w.ws_col);
  generic_options.add_options()
    ("help", "show help message")
    ("verbose,v", po::value<bool>(&config.partition.verbose_output)->value_name("<bool>"),
    "Verbose partitioning output");

  po::options_description required_options("Required Options", w.ws_col);
  required_options.add_options()
    ("hypergraph,h",
    po::value<std::string>(&config.partition.graph_filename)->value_name("<string>")->required()->notifier(
      [&](const std::string&) {
    config.partition.coarse_graph_filename =
      std::string("/tmp/PID_")
      + std::to_string(getpid()) + std::string("_coarse_")
      + config.partition.graph_filename.substr(
        config.partition.graph_filename.find_last_of("/") + 1);
    config.partition.graph_partition_filename =
      config.partition.graph_filename + ".part."
      + std::to_string(config.partition.k) + ".KaHyPar";
    config.partition.coarse_graph_partition_filename =
      config.partition.coarse_graph_filename + ".part."
      + std::to_string(config.partition.k);
  }),
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
    config.partition.mode = partition::modeFromString(mode);
  }),
    "Partitioning mode: \n"
    " - (recursive) bisection \n"
    " - (direct) k-way");

  std::string config_path;
  po::options_description preset_options("Preset Options", w.ws_col);
  preset_options.add_options()
    ("preset,p", po::value<std::string>(&config_path)->value_name("<string>"),
    "Configuration Presets:\n"
    " - direct_kway_km1_alenex17\n"
    " - rb_cut_alenex16\n"
    " - <path-to-custom-ini-file>");

  po::options_description general_options("General Options", w.ws_col);
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

  po::options_description preprocessing_options("Preprocessing Options", w.ws_col);
  preprocessing_options.add_options()
    ("p-use-sparsifier",
    po::value<bool>(&config.preprocessing.use_min_hash_sparsifier)->value_name("<bool>"),
    "Use min-hash pin sparsifier before partitioning \n"
    "(default: false)")
    ("p-parallel-net-removal",
    po::value<bool>(&config.preprocessing.remove_parallel_hes)->value_name("<bool>"),
    "Remove parallel hyperedges before partitioning \n"
    "(default: false)")
    ("p-large-net-removal",
    po::value<bool>(&config.preprocessing.remove_always_cut_hes)->value_name("<bool>"),
    "Remove hyperedges that will always be cut because"
    " of the weight of their pins \n"
    "(default: false)");

  po::options_description coarsening_options("Coarsening Options", w.ws_col);
  coarsening_options.add_options()
    ("c-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ctype) {
    config.coarsening.algorithm = partition::coarseningAlgorithmFromString(ctype);
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


  po::options_description ip_options("Initial Partitioning Options", w.ws_col);
  ip_options.add_options()
    ("i-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& initial_partitioner) {
    config.initial_partitioning.tool =
      partition::initialPartitionerFromString(initial_partitioner);
  }),
    "Initial Partitioner:\n"
    " - KaHyPar\n"
    " - hMetis\n"
    " - PaToH\n"
    "(default: KaHyPar)")
    ("i-mode",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_mode) {
    config.initial_partitioning.mode = partition::modeFromString(ip_mode);
  }),
    "IP mode: \n"
    " - (recursive) bisection  \n"
    " - (direct) k-way\n"
    "(default: rb)")
    ("i-technique",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_technique) {
    config.initial_partitioning.technique =
      partition::inititalPartitioningTechniqueFromString(ip_technique);
  }),
    "IP Technique:\n"
    " - flat\n"
    " - (multi)level\n"
    "(default: multi)")
    ("i-algo",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_algo) {
    config.initial_partitioning.algo =
      partition::initialPartitioningAlgorithmFromString(ip_algo);
  }),
    "Algorithm used to create initial partition: pool (default)")
    ("i-c-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& ip_ctype) {
    config.initial_partitioning.coarsening.algorithm =
      partition::coarseningAlgorithmFromString(ip_ctype);
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
      partition::refinementAlgorithmFromString(ip_rtype);
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
      partition::stoppingRuleFromString(ip_stopfm);
  }),
    "Stopping Rule for IP Local Search: \n"
    " - adaptive_opt: ALENEX'17 stopping rule \n"
    " - adaptive1:    new nGP implementation \n"
    " - adaptive2:    original nGP implementation \n"
    " - simple:       threshold based on i-r-i\n"
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
    "(default:1, no limit:-1)")
    ("i-path",
    po::value<std::string>(&config.initial_partitioning.tool_path)->value_name("<string>")->notifier(
      [&](const std::string& tool_path) {
    if (tool_path == "-") {
      switch (config.initial_partitioning.tool) {
        case InitialPartitioner::hMetis:
          config.initial_partitioning.tool_path =
            "/software/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1";
          break;
        case InitialPartitioner::PaToH:
          config.initial_partitioning.tool_path =
            "/software/patoh-Linux-x86_64/Linux-x86_64/patoh";
          break;
        case InitialPartitioner::KaHyPar:
          config.initial_partitioning.tool_path = "-";
          break;
      }
    }
  }),
    "Path to hMetis or PaToH binary.\n"
    "Only necessary if hMetis or PaToH is chosen as initial partitioner.");


  po::options_description refinement_options("Refinement Options", w.ws_col);
  refinement_options.add_options()
    ("r-type",
    po::value<std::string>()->value_name("<string>")->notifier(
      [&](const std::string& rtype) {
    config.local_search.algorithm = partition::refinementAlgorithmFromString(rtype);
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
    config.local_search.fm.stopping_rule = partition::stoppingRuleFromString(stopfm);
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
  // error of only help was supplied
  if (cmd_vm.count("help")) {
    std::cout << cmd_line_options << "n";
    exit(0);
  }

  po::notify(cmd_vm);

  std::ifstream file(config_path.c_str());
  if (!file) {
    std::cout << "Could not load config file at: " << config_path << std::endl;
    exit(0);
  }

  po::options_description ini_line_options;
  ini_line_options.add(general_options)
  .add(preprocessing_options)
  .add(coarsening_options)
  .add(ip_options)
  .add(refinement_options);

  po::store(po::parse_config_file(file, ini_line_options, true), cmd_vm);
  po::notify(cmd_vm);
}

int main(int argc, char* argv[]) {
  Configuration config;

  processCommandLineInput(config, argc, argv);
  sanityCheck(config);

  if (config.partition.global_search_iterations != 0) {
    LOG("Coarsener does not check if HNs are in same part.");
    LOG("Therefore v-cycles are currently disabled.");
    exit(0);
  }

  Randomize::instance().setSeed(config.partition.seed);

  Hypergraph hypergraph(
    io::createHypergraphFromFile(config.partition.graph_filename,
                                 config.partition.k));

  if (config.partition.verbose_output) {
    io::printHypergraphInfo(hypergraph,
                            config.partition.graph_filename.substr(
                              config.partition.graph_filename.find_last_of("/") + 1));
  }

  Partitioner partitioner;
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  partitioner.partition(hypergraph, config);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;

#ifdef GATHER_STATS
  LOG("*******************************");
  LOG("***** GATHER_STATS ACTIVE *****");
  LOG("*******************************");
  io::printPartitioningStatistics();
#endif

  io::printPartitioningResults(hypergraph, config, elapsed_seconds);
  io::writePartitionFile(hypergraph,
                         config.partition.graph_partition_filename);

  std::remove(config.partition.coarse_graph_filename.c_str());
  std::remove(config.partition.coarse_graph_partition_filename.c_str());

  io::serializer::serialize(config, hypergraph, partitioner, elapsed_seconds);
  return 0;
}
