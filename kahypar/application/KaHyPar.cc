/***************************************************************************
 *  Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#include <boost/program_options.hpp>

#include <chrono>
#include <memory>
#include <string>

#include "definitions.h"
#include "io/hypergraph_io.h"
#include "io/partitioning_output.h"
#include "macros.h"
#include "meta/registrar.h"
#include "partition/coarsening/i_coarsener.h"
#include "partition/configuration.h"
#include "partition/factories.h"
#include "partition/initial_partitioning/bfs_initial_partitioner.h"
#include "partition/initial_partitioning/greedy_hypergraph_growing_initial_partitioner.h"
#include "partition/initial_partitioning/i_initial_partitioner.h"
#include "partition/initial_partitioning/label_propagation_initial_partitioner.h"
#include "partition/initial_partitioning/policies/ip_gain_computation_policy.h"
#include "partition/initial_partitioning/policies/ip_greedy_queue_selection_policy.h"
#include "partition/initial_partitioning/policies/ip_start_node_selection_policy.h"
#include "partition/initial_partitioning/pool_initial_partitioner.h"
#include "partition/initial_partitioning/random_initial_partitioner.h"
#include "partition/metrics.h"
#include "partition/partitioner.h"
#include "utils/randomize.h"
#include "utils/sql_plottools_serializer.h"

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
using partition::RandomWinsHeuristicCoarsener;
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
using utils::SQLPlotToolsSerializer;

static Registrar<CoarsenerFactory> reg_heavy_lazy_coarsener(
  CoarseningAlgorithm::heavy_lazy,
  [](Hypergraph& hypergraph, const Configuration& config,
     const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
  return new RandomWinsLazyUpdateCoarsener(hypergraph, config, weight_of_heaviest_node);
});

static Registrar<CoarsenerFactory> reg_heavy_partial_coarsener(
  CoarseningAlgorithm::heavy_partial,
  [](Hypergraph& hypergraph, const Configuration& config,
     const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
  return new RandomWinsHeuristicCoarsener(hypergraph, config,
                                          weight_of_heaviest_node);
});

static Registrar<CoarsenerFactory> reg_heavy_full_coarsener(
  CoarseningAlgorithm::heavy_full,
  [](Hypergraph& hypergraph, const Configuration& config,
     const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
  return new RandomWinsFullCoarsener(hypergraph, config, weight_of_heaviest_node);
});

static Registrar<CoarsenerFactory> reg_ml_coarsener(
  CoarseningAlgorithm::ml_style,
  [](Hypergraph& hypergraph, const Configuration& config,
     const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
  return new RandomWinsMLCoarsener(hypergraph, config, weight_of_heaviest_node);
});

static Registrar<CoarsenerFactory> reg_do_nothing_coarsener(
  CoarseningAlgorithm::do_nothing,
  [](Hypergraph& hypergraph, const Configuration& config,
     const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
  (void)hypergraph;
  (void)config;
  (void)weight_of_heaviest_node;                               // Fixing unused parameter warning
  return new DoNothingCoarsener();
});

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
      config.local_search.fm.stopping_rule),
    NullPolicy(), NullPolicy());
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

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_simple_stopping(
  RefinementStoppingRule::simple,
  new NumberOfFruitlessMovesStopsSearch());

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_adaptive_opt_stopping(
  RefinementStoppingRule::adaptive_opt, new AdvancedRandomWalkModelStopsSearch());

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_adaptive1_stopping(
  RefinementStoppingRule::adaptive1, new RandomWalkModelStopsSearch());

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_adaptive2_stopping(
  RefinementStoppingRule::adaptive2, new nGPRandomWalkStopsSearch());

static Registrar<PolicyRegistry<GlobalRebalancingMode> > reg_global_rebalancing(
  GlobalRebalancingMode::on,
  new GlobalRebalancing());

static Registrar<PolicyRegistry<GlobalRebalancingMode> > reg_no_global_rebalancing(
  GlobalRebalancingMode::off,
  new NoGlobalRebalancing());

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


void processCommandLineInput(Configuration& config, std::string& result_file,
                             int argc, char* argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help", "show help message")
    ("verbose", po::value<bool>(&config.partition.verbose_output),
    "Verbose partitioner output")
    ("k", po::value<PartitionID>(&config.partition.k)->required()->notifier(
      [&](const PartitionID) {
    config.partition.rb_lower_k = 0;
    config.partition.rb_upper_k = config.partition.k - 1;
  }),
    "Number of blocks")
    ("hgr", po::value<std::string>(&config.partition.graph_filename)->required()->notifier(
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
    "Filename of the hypergraph")
    ("e", po::value<double>(&config.partition.epsilon)->required(),
    "Imbalance parameter epsilon")
    ("obj", po::value<std::string>()->notifier([&](const std::string& s) {
    if (s == "cut") {
      config.partition.objective = Objective::cut;
    } else if (s == "km1") {
      config.partition.objective = Objective::km1;
    } else {
      std::cout << "No valid objective function." << std::endl;
      exit(0);
    }
  }),
    "Objective: cut, km1")
    ("seed", po::value<int>(&config.partition.seed),
    "Seed for random number generator")
    ("mode", po::value<std::string>()->notifier(
      [&](const std::string& mode) {
    config.partition.mode = partition::modeFromString(mode);
  }),
    "(rb) recursive bisection, (direct) direct k-way")
    ("init-remove-hes", po::value<bool>(&config.partition.initial_parallel_he_removal),
    "Initially remove parallel hyperedges before partitioning")
    ("ip", po::value<std::string>()->notifier(
      [&](const std::string& initial_partitioner) {
    config.initial_partitioning.tool =
      partition::initialPartitionerFromString(initial_partitioner);
  }),
    "Initial Partitioner: hMetis (default), PaToH or KaHyPar")
    ("ip-path", po::value<std::string>(&config.initial_partitioning.tool_path)->default_value("-")->notifier(
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
    "If ip!=KaHyPar: Path to Initial Partitioner Binary")
    ("ip-mode", po::value<std::string>()->notifier(
      [&](const std::string& ip_mode) {
    config.initial_partitioning.mode = partition::modeFromString(ip_mode);
  }),
    "If ip=KaHyPar: direct (direct) or recursive bisection (rb) initial partitioning")
    ("ip-technique", po::value<std::string>()->notifier(
      [&](const std::string& ip_technique) {
    config.initial_partitioning.technique =
      partition::inititalPartitioningTechniqueFromString(ip_technique);
  }),
    "If ip=KaHyPar: flat (flat) or multilevel (multi) initial partitioning")
    ("ip-algo", po::value<std::string>()->notifier(
      [&](const std::string& ip_algo) {
    config.initial_partitioning.algo =
      partition::initialPartitioningAlgorithmFromString(ip_algo);
  }),
    "If ip=KaHyPar: used initial partitioning algorithm")
    ("ip-ctype", po::value<std::string>()->notifier(
      [&](const std::string& ip_ctype) {
    config.initial_partitioning.coarsening.algorithm =
      partition::coarseningAlgorithmFromString(ip_ctype);
  }),
    "If ip=KaHyPar: used coarsening algorithm for multilevel initial partitioning")
    ("ip-nruns", po::value<int>(&config.initial_partitioning.nruns),
    "# initial partition trials")
    ("ip-s", po::value<double>(&config.initial_partitioning.coarsening.max_allowed_weight_multiplier),
    "If ip=KaHyPar: IP Coarsening: The maximum weight of a hypernode in the coarsest is:(s * w(Graph)) / (t * k)")
    ("ip-t", po::value<HypernodeID>(&config.initial_partitioning.coarsening.contraction_limit_multiplier),
    "If ip=KaHyPar: IP Coarsening: Coarsening stops when there are no more than t * k hypernodes left")
    ("ip-rtype", po::value<std::string>()->notifier(
      [&](const std::string& ip_rtype) {
    config.initial_partitioning.local_search.algorithm =
      partition::refinementAlgorithmFromString(ip_rtype);
  }),
    "If ip=KaHyPar: used refinement algorithm for multilevel initial partitioning")
    ("ip-i", po::value<int>(&config.initial_partitioning.local_search.fm.max_number_of_fruitless_moves),
    "If ip=KaHyPar:  max. # fruitless moves before stopping local search (simple)")
    ("ip-stopFM", po::value<std::string>()->notifier(
      [&](const std::string& ip_stopfm) {
    config.initial_partitioning.local_search.fm.stopping_rule =
      partition::stoppingRuleFromString(ip_stopfm);
  }),
    "If ip=KaHyPar: Stopping rule \n adaptive1: new implementation based on nGP \n adaptive2: original nGP implementation \n simple: threshold based")
    ("ip-alpha", po::value<double>(&config.initial_partitioning.init_alpha),
    "If ip=KaHyPar: Restrict initial partitioning epsilon to init-alpha*epsilon")
    ("ip-ls-reps", po::value<int>(&config.initial_partitioning.local_search.iterations_per_level)->notifier(
      [&](const int) {
    if (config.initial_partitioning.local_search.iterations_per_level == -1) {
      config.initial_partitioning.local_search.iterations_per_level =
        std::numeric_limits<int>::max();
    }
  }),
    "If ip=KaHyPar: local search repetitions (default:1, no limit:-1)")
    ("vcycles", po::value<int>(&config.partition.global_search_iterations),
    "# v-cycle iterations")
    ("cmaxnet", po::value<HyperedgeID>(&config.partition.hyperedge_size_threshold)->notifier(
      [&](const HyperedgeID) {
    if (config.partition.hyperedge_size_threshold == -1) {
      config.partition.hyperedge_size_threshold = std::numeric_limits<HyperedgeID>::max();
    }
  }),
    "Any hyperedges larger than cmaxnet are removed from the hypergraph before partition (disable:-1 (default))")
    ("work-factor", po::value<double>(&config.partition.work_factor),
    "Any hyperedges incurring more than work-factor * |pins| work will be removed")
    ("remove-always-cut-hes", po::value<bool>(&config.partition.remove_hes_that_always_will_be_cut),
    "Any hyperedges whose accumulated pin-weight is larger than Lmax will always be a cut HE and can therefore be removed (default: false)")
    ("ctype", po::value<std::string>()->notifier(
      [&](const std::string& ctype) {
    config.coarsening.algorithm = partition::coarseningAlgorithmFromString(ctype);
  }),
    "Coarsening: Scheme to be used: heavy_full (default), heavy_heuristic, heavy_lazy, hyperedge")
    ("s", po::value<double>(&config.coarsening.max_allowed_weight_multiplier),
    "Coarsening: The maximum weight of a hypernode in the coarsest is:(s * w(Graph)) / (t * k)")
    ("t", po::value<HypernodeID>(&config.coarsening.contraction_limit_multiplier),
    "Coarsening: Coarsening stops when there are no more than t * k hypernodes left")
    ("rtype", po::value<std::string>()->notifier(
      [&](const std::string& rtype) {
    config.local_search.algorithm = partition::refinementAlgorithmFromString(rtype);
  }),
    "Refinement: twoway_fm, kway_fm, kway_fm_maxgain, kway_fm_km1, sclap")
    ("sclap-max-iterations", po::value<int>(&config.local_search.sclap.max_number_iterations),
    "Refinement: maximum number of iterations for label propagation based refinement")
    ("stopFM", po::value<std::string>()->notifier(
      [&](const std::string& stopfm) {
    config.local_search.fm.stopping_rule = partition::stoppingRuleFromString(stopfm);
  }),
    "2-Way-FM | HER-FM: Stopping rule \n adaptive1: new implementation based on nGP \n adaptive2: original nGP implementation \n simple: threshold based")
    ("global-rebalancing", po::value<bool>()->notifier(
      [&](const bool global_rebalancing) {
    if (global_rebalancing) {
      config.local_search.fm.global_rebalancing = GlobalRebalancingMode::on;
    } else {
      config.local_search.fm.global_rebalancing = GlobalRebalancingMode::off;
    }
  }),
    "Use global rebalancing PQs in twoway_fm")
    ("ls-reps", po::value<int>(&config.local_search.iterations_per_level)->notifier(
      [&](const int) {
    if (config.local_search.iterations_per_level == -1) {
      config.local_search.iterations_per_level = std::numeric_limits<int>::max();
    }
  }),
    "max. # of local search repetitions on each level (default: no limit = -1)")
    ("i", po::value<int>(&config.local_search.fm.max_number_of_fruitless_moves),
    "2-Way-FM | HER-FM: max. # fruitless moves before stopping local search (simple)")
    ("alpha", po::value<double>(&config.local_search.fm.adaptive_stopping_alpha),
    "2-Way-FM: Random Walk stop alpha (adaptive) (infinity: -1)")
    ("file", po::value<std::string>(&result_file),
    "filename of result file");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);

  // placing vm.count("help") here prevents required attributes raising an
  // error of only help was supplied
  if (vm.count("help")) {
    std::cout << desc << "n";
    exit(0);
  }

  po::notify(vm);

  if (vm.count("file")) {
    result_file = vm["file"].as<std::string>();
  }
}

int main(int argc, char* argv[]) {
  Configuration config;
  std::string result_file;

  processCommandLineInput(config, result_file, argc, argv);
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

#ifndef NDEBUG
  for (const HyperedgeID he : hypergraph.edges()) {
    ASSERT([&]() -> bool {
      HypernodeID num_pins = 0;
      for (PartitionID i = 0; i < config.partition.k; ++i) {
        num_pins += hypergraph.pinCountInPart(he, i);
      }
      return num_pins == hypergraph.edgeSize(he);
    } (), "Incorrect calculation of pin counts");
  }
#endif

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

  SQLPlotToolsSerializer::serialize(config, hypergraph, partitioner,
                                    elapsed_seconds, result_file);
  return 0;
}
