/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#include <boost/program_options.hpp>

#include <chrono>
#include <memory>
#include <string>

#include "lib/definitions.h"

#include "lib/core/Registrar.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "lib/macros.h"
#include "partition/Configuration.h"
#include "partition/Factories.h"
#include "partition/Metrics.h"
#include "partition/Partitioner.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingInitialPartitioner.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/LabelPropagationInitialPartitioner.h"
#include "partition/initial_partitioning/PoolInitialPartitioner.h"
#include "partition/initial_partitioning/RandomInitialPartitioner.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "tools/RandomFunctions.h"

#include "lib/serializer/SQLPlotToolsSerializer.h"

namespace po = boost::program_options;

using core::Registrar;
using partition::Partitioner;
using partition::InitialPartitioner;
using partition::Configuration;
using partition::Mode;
using partition::InitialPartitioningTechnique;
using partition::CoarseningAlgorithm;
using partition::RefinementAlgorithm;
using partition::InitialPartitionerAlgorithm;
using partition::RefinementStoppingRule;
using partition::CoarsenerFactory;
using partition::RefinerFactory;
using partition::InitialPartitioningFactory;
using partition::DoNothingCoarsener;
using partition::RandomWinsFullCoarsener;
using partition::RandomWinsLazyUpdateCoarsener;
using partition::RandomWinsHeuristicCoarsener;
using partition::TwoWayFMFactoryDispatcher;
using partition::HyperedgeFMFactoryDispatcher;
using partition::KWayFMFactoryDispatcher;
using partition::MaxGainNodeKWayFMFactoryDispatcher;
using partition::TwoWayFMFactoryExecutor;
using partition::HyperedgeFMFactoryExecutor;
using partition::KWayFMFactoryExecutor;
using partition::MaxGainNodeKWayFMFactoryExecutor;
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
using serializer::SQLPlotToolsSerializer;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeID;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeightVector;
using defs::HypernodeWeightVector;
using defs::HighResClockTimepoint;

InitialPartitionerAlgorithm stringToInitialPartitionerAlgorithm(std::string mode) {
  if (mode.compare("greedy_sequential") == 0) {
    return InitialPartitionerAlgorithm::greedy_sequential;
  } else if (mode.compare("greedy_global") == 0) {
    return InitialPartitionerAlgorithm::greedy_global;
  } else if (mode.compare("greedy_round") == 0) {
    return InitialPartitionerAlgorithm::greedy_round;
  } else if (mode.compare("greedy_sequential_maxpin") == 0) {
    return InitialPartitionerAlgorithm::greedy_sequential_maxpin;
  } else if (mode.compare("greedy_global_maxpin") == 0) {
    return InitialPartitionerAlgorithm::greedy_global_maxpin;
  } else if (mode.compare("greedy_round_maxpin") == 0) {
    return InitialPartitionerAlgorithm::greedy_round_maxpin;
  } else if (mode.compare("greedy_sequential_maxnet") == 0) {
    return InitialPartitionerAlgorithm::greedy_sequential_maxnet;
  } else if (mode.compare("greedy_global_maxnet") == 0) {
    return InitialPartitionerAlgorithm::greedy_global_maxnet;
  } else if (mode.compare("greedy_round_maxnet") == 0) {
    return InitialPartitionerAlgorithm::greedy_round_maxnet;
  } else if (mode.compare("lp") == 0) {
    return InitialPartitionerAlgorithm::lp;
  } else if (mode.compare("bfs") == 0) {
    return InitialPartitionerAlgorithm::bfs;
  } else if (mode.compare("random") == 0) {
    return InitialPartitionerAlgorithm::random;
  } else if (mode.compare("pool") == 0) {
    return InitialPartitionerAlgorithm::pool;
  }
  return InitialPartitionerAlgorithm::greedy_global;
}

void configurePartitionerFromCommandLineInput(Configuration& config,
                                              const po::variables_map& vm) {
  if (vm.count("hgr") && vm.count("e") && vm.count("k")) {
    config.partition.graph_filename = vm["hgr"].as<std::string>();
    config.partition.k = vm["k"].as<PartitionID>();
    config.partition.rb_lower_k = 0;
    config.partition.rb_upper_k = config.partition.k - 1;

    config.partition.coarse_graph_filename = std::string("/tmp/PID_")
                                             + std::to_string(getpid()) + std::string("_coarse_")
                                             + config.partition.graph_filename.substr(
      config.partition.graph_filename.find_last_of("/") + 1);
    config.partition.graph_partition_filename =
      config.partition.graph_filename + ".part."
      + std::to_string(config.partition.k) + ".KaHyPar";
    config.partition.coarse_graph_partition_filename =
      config.partition.coarse_graph_filename + ".part."
      + std::to_string(config.partition.k);
    config.partition.epsilon = vm["e"].as<double>();
    config.initial_partitioning.epsilon = vm["e"].as<double>();

    if (vm.count("seed")) {
      config.partition.seed = vm["seed"].as<int>();
    }
    if (vm.count("nruns")) {
      config.partition.initial_partitioning_attempts =
        vm["nruns"].as<int>();
      config.initial_partitioning.nruns = vm["nruns"].as<int>();
    }

    if (vm.count("mode")) {
      if (vm["mode"].as<std::string>() == "rb") {
        config.partition.mode = Mode::recursive_bisection;
      } else if (vm["mode"].as<std::string>() == "direct") {
        config.partition.mode = Mode::direct_kway;
      }
    }

    if (vm.count("part")) {
      if (vm["part"].as<std::string>() == "hMetis") {
        config.partition.initial_partitioner =
          InitialPartitioner::hMetis;
      } else if (vm["part"].as<std::string>() == "PaToH") {
        config.partition.initial_partitioner =
          InitialPartitioner::PaToH;
      } else if (vm["part"].as<std::string>() == "KaHyPar") {
        config.partition.initial_partitioner =
          InitialPartitioner::KaHyPar;
        if (vm.count("init-mode")) {
          if (vm["init-mode"].as<std::string>() == "rb") {
            config.initial_partitioning.init_mode =
              Mode::recursive_bisection;
          } else if (vm["init-mode"].as<std::string>() == "direct") {
            config.initial_partitioning.init_mode =
              Mode::direct_kway;
          }
        }
        if (vm.count("init-technique")) {
          if (vm["init-technique"].as<std::string>() == "flat") {
            config.initial_partitioning.init_technique =
              InitialPartitioningTechnique::flat;
          } else if (vm["init-technique"].as<std::string>()
                     == "multi") {
            config.initial_partitioning.init_technique =
              InitialPartitioningTechnique::multilevel;
          }
        }
        if (vm.count("init-algo")) {
          config.initial_partitioning.algo =
            stringToInitialPartitionerAlgorithm(
              vm["init-algo"].as<std::string>());
        }
        if (vm.count("init-alpha")) {
          config.initial_partitioning.init_alpha =
            vm["init-alpha"].as<double>();
        }

        // If we use the n-Level recursive bisection partitioner the initial partitioner is only
        // call for k=2.
        if (config.partition.mode == Mode::recursive_bisection &&
            config.initial_partitioning.init_mode
            == Mode::recursive_bisection) {
          config.initial_partitioning.init_mode = Mode::direct_kway;
        }

        if (config.initial_partitioning.init_mode == Mode::direct_kway &&
            config.initial_partitioning.init_technique
            == InitialPartitioningTechnique::multilevel) {
          config.initial_partitioning.init_technique =
            InitialPartitioningTechnique::flat;
        }
      }
    }
    if (vm.count("part-path")) {
      config.partition.initial_partitioner_path = vm["part-path"].as<
        std::string>();
    } else {
      switch (config.partition.initial_partitioner) {
        case InitialPartitioner::hMetis:
          config.partition.initial_partitioner_path =
            "/software/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1";
          break;
        case InitialPartitioner::PaToH:
          config.partition.initial_partitioner_path =
            "/software/patoh-Linux-x86_64/Linux-x86_64/patoh";
          break;
      }
    }
    if (vm.count("vcycles")) {
      config.partition.global_search_iterations = vm["vcycles"].as<int>();
    }
    if (vm.count("cmaxnet")) {
      config.partition.hyperedge_size_threshold = vm["cmaxnet"].as<
        HyperedgeID>();
      if (config.partition.hyperedge_size_threshold == -1) {
        config.partition.hyperedge_size_threshold = std::numeric_limits<
          HyperedgeID>::max();
      }
    }
    if (vm.count("ctype")) {
      if (vm["ctype"].as<std::string>() == "heavy_full") {
        config.partition.coarsening_algorithm =
          CoarseningAlgorithm::heavy_full;
      } else if (vm["ctype"].as<std::string>() == "heavy_partial") {
        config.partition.coarsening_algorithm =
          CoarseningAlgorithm::heavy_partial;
      } else if (vm["ctype"].as<std::string>() == "heavy_lazy") {
        config.partition.coarsening_algorithm =
          CoarseningAlgorithm::heavy_lazy;
      } else if (vm["ctype"].as<std::string>() == "hyperedge") {
        config.partition.coarsening_algorithm =
          CoarseningAlgorithm::hyperedge;
      } else {
        std::cout << "Illegal ctype option! Exiting..." << std::endl;
        exit(0);
      }
    }
    if (vm.count("s")) {
      config.coarsening.max_allowed_weight_multiplier =
        vm["s"].as<double>();
    }
    if (vm.count("t")) {
      config.coarsening.contraction_limit_multiplier = vm["t"].as<
        HypernodeID>();
    }
    if (vm.count("stopFM")) {
      if (vm["stopFM"].as<std::string>() == "simple") {
        config.fm_local_search.stopping_rule =
          RefinementStoppingRule::simple;
        config.her_fm.stopping_rule = RefinementStoppingRule::simple;
      } else if (vm["stopFM"].as<std::string>() == "adaptive1") {
        config.fm_local_search.stopping_rule =
          RefinementStoppingRule::adaptive1;
        config.her_fm.stopping_rule = RefinementStoppingRule::adaptive1;
      } else if (vm["stopFM"].as<std::string>() == "adaptive2") {
        config.fm_local_search.stopping_rule =
          RefinementStoppingRule::adaptive2;
        config.her_fm.stopping_rule = RefinementStoppingRule::adaptive2;
      } else {
        std::cout << "Illegal stopFM option! Exiting..." << std::endl;
        exit(0);
      }
    }
    if (vm.count("FMreps")) {
      config.fm_local_search.num_repetitions = vm["FMreps"].as<int>();
      config.her_fm.num_repetitions = vm["FMreps"].as<int>();
      if (config.fm_local_search.num_repetitions == -1) {
        config.fm_local_search.num_repetitions =
          std::numeric_limits<int>::max();
        config.her_fm.num_repetitions = std::numeric_limits<int>::max();
      }
    }
    if (vm.count("init-FMreps")) {
      config.initial_partitioning.local_search_repetitions =
        vm["init-FMreps"].as<int>();
      if (config.initial_partitioning.local_search_repetitions == -1) {
        config.initial_partitioning.local_search_repetitions =
          std::numeric_limits<int>::max();
      }
    }
    if (vm.count("i")) {
      config.fm_local_search.max_number_of_fruitless_moves = vm["i"].as<
        int>();
      config.her_fm.max_number_of_fruitless_moves = vm["i"].as<int>();
    }
    if (vm.count("alpha")) {
      config.fm_local_search.alpha = vm["alpha"].as<double>();
      if (config.fm_local_search.alpha == -1) {
        config.fm_local_search.alpha =
          std::numeric_limits<double>::max();
      }
    }
    if (vm.count("verbose")) {
      config.partition.verbose_output = vm["verbose"].as<bool>();
    }
    if (vm.count("init-remove-hes")) {
      config.partition.initial_parallel_he_removal =
        vm["init-remove-hes"].as<bool>();
    }
    if (vm.count("remove-always-cut-hes")) {
      config.partition.remove_hes_that_always_will_be_cut =
        vm["remove-always-cut-hes"].as<bool>();
    }

    if (vm.count("lp_refiner_max_iterations")) {
      config.lp_refiner.max_number_iterations =
        vm["lp_refiner_max_iterations"].as<int>();
    }
    if (vm.count("rtype")) {
      if (vm["rtype"].as<std::string>() == "twoway_fm") {
        config.partition.refinement_algorithm =
          RefinementAlgorithm::twoway_fm;
      } else if (vm["rtype"].as<std::string>() == "kway_fm") {
        config.partition.refinement_algorithm =
          RefinementAlgorithm::kway_fm;
      } else if (vm["rtype"].as<std::string>() == "kway_fm_maxgain") {
        config.partition.refinement_algorithm =
          RefinementAlgorithm::kway_fm_maxgain;
      } else if (vm["rtype"].as<std::string>() == "hyperedge") {
        config.partition.refinement_algorithm =
          RefinementAlgorithm::hyperedge;
      } else if (vm["rtype"].as<std::string>() == "sclap") {
        config.partition.refinement_algorithm =
          RefinementAlgorithm::label_propagation;
      } else {
        std::cout << "Illegal stopFM option! Exiting..." << std::endl;
        exit(0);
      }
    }
  } else {
    std::cout << "Parameter error! Exiting..." << std::endl;
    exit(0);
  }
}

void setDefaults(Configuration& config) {
  config.partition.k = 2;
  config.partition.rb_lower_k = 0;
  config.partition.rb_upper_k = 1;
  config.partition.epsilon = 0.05;
  config.partition.seed = -1;
  config.partition.initial_partitioning_attempts = 20;
  config.initial_partitioning.nruns = 20;
  config.partition.global_search_iterations = 1;
  config.partition.hyperedge_size_threshold =
    std::numeric_limits<HyperedgeID>::max();
  config.partition.coarsening_algorithm = CoarseningAlgorithm::heavy_lazy;
  config.partition.refinement_algorithm = RefinementAlgorithm::kway_fm;
  config.initial_partitioning.pool_type = 1975;
  config.initial_partitioning.init_technique =
    InitialPartitioningTechnique::flat;
  config.initial_partitioning.init_mode = Mode::recursive_bisection;
  config.initial_partitioning.algo = InitialPartitionerAlgorithm::pool;
  config.initial_partitioning.init_alpha = 1.0;
  config.coarsening.contraction_limit_multiplier = 160;
  config.coarsening.max_allowed_weight_multiplier = 2.5;
  config.coarsening.contraction_limit =
    config.coarsening.contraction_limit_multiplier * config.partition.k;
  config.coarsening.hypernode_weight_fraction =
    config.coarsening.max_allowed_weight_multiplier
    / config.coarsening.contraction_limit;
  config.fm_local_search.stopping_rule = RefinementStoppingRule::simple;
  config.fm_local_search.num_repetitions = -1;
  config.fm_local_search.max_number_of_fruitless_moves = 200;
  config.fm_local_search.alpha = 8;
  config.her_fm.stopping_rule = RefinementStoppingRule::simple;
  config.her_fm.num_repetitions = 1;
  config.her_fm.max_number_of_fruitless_moves = 10;
  config.lp_refiner.max_number_iterations = 3;
}

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

static Registrar<CoarsenerFactory> reg_do_nothing_coarsener(
  CoarseningAlgorithm::do_nothing,
  [](Hypergraph& hypergraph, const Configuration& config,
     const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {
  return new DoNothingCoarsener();
});

static Registrar<RefinerFactory> reg_twoway_fm_local_search(
  RefinementAlgorithm::twoway_fm,
  [](Hypergraph& hypergraph, const Configuration& config) {
  return TwoWayFMFactoryDispatcher::go(
    PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
      config.fm_local_search.stopping_rule),
    NullPolicy(),
    TwoWayFMFactoryExecutor(), hypergraph, config);
});

static Registrar<RefinerFactory> reg_kway_fm_maxgain_local_search(
  RefinementAlgorithm::kway_fm_maxgain,
  [](Hypergraph& hypergraph, const Configuration& config) {
  return MaxGainNodeKWayFMFactoryDispatcher::go(
    PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
      config.fm_local_search.stopping_rule),
    NullPolicy(),
    MaxGainNodeKWayFMFactoryExecutor(), hypergraph, config);
});

static Registrar<RefinerFactory> reg_kway_fm_local_search(
  RefinementAlgorithm::kway_fm,
  [](Hypergraph& hypergraph, const Configuration& config) {
  return KWayFMFactoryDispatcher::go(
    PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
      config.fm_local_search.stopping_rule),
    NullPolicy(),
    KWayFMFactoryExecutor(), hypergraph, config);
});

static Registrar<RefinerFactory> reg_lp_local_search(
  RefinementAlgorithm::label_propagation,
  [](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* {
  return new LPRefiner(hypergraph, config);
});

static Registrar<RefinerFactory> reg_do_nothing_refiner(
  RefinementAlgorithm::do_nothing,
  [](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* {
  return new DoNothingRefiner();
});

static Registrar<RefinerFactory> reg_hyperedge_local_search(
  RefinementAlgorithm::hyperedge,
  [](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* {
  return HyperedgeFMFactoryDispatcher::go(
    PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
      config.her_fm.stopping_rule),
    NullPolicy(),
    HyperedgeFMFactoryExecutor(), hypergraph, config);
});

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_simple_stopping(
  RefinementStoppingRule::simple,
  new NumberOfFruitlessMovesStopsSearch());

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_adaptive1_stopping(
  RefinementStoppingRule::adaptive1, new RandomWalkModelStopsSearch());

static Registrar<PolicyRegistry<RefinementStoppingRule> > reg_adaptive2_stopping(
  RefinementStoppingRule::adaptive2, new nGPRandomWalkStopsSearch());

static Registrar<InitialPartitioningFactory> reg_random(
  InitialPartitionerAlgorithm::random,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new RandomInitialPartitioner(hypergraph, config);
});

static Registrar<InitialPartitioningFactory> reg_bfs(
  InitialPartitionerAlgorithm::bfs,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new BFSInitialPartitioner<BFSStartNodeSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_lp(
  InitialPartitionerAlgorithm::lp,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                FMGainComputationPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy(
  InitialPartitionerAlgorithm::greedy_sequential,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       FMGainComputationPolicy,
                                                       SequentialQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_global(
  InitialPartitionerAlgorithm::greedy_global,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       FMGainComputationPolicy,
                                                       GlobalQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_round(
  InitialPartitionerAlgorithm::greedy_round,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       FMGainComputationPolicy,
                                                       RoundRobinQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_maxpin(
  InitialPartitionerAlgorithm::greedy_sequential_maxpin,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       MaxPinGainComputationPolicy,
                                                       SequentialQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_global_maxpin(
  InitialPartitionerAlgorithm::greedy_global_maxpin,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       MaxPinGainComputationPolicy,
                                                       GlobalQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_round_maxpin(
  InitialPartitionerAlgorithm::greedy_round_maxpin,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       MaxPinGainComputationPolicy,
                                                       RoundRobinQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_maxnet(
  InitialPartitionerAlgorithm::greedy_sequential_maxnet,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       MaxNetGainComputationPolicy,
                                                       SequentialQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_global_maxnet(
  InitialPartitionerAlgorithm::greedy_global_maxnet,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       MaxNetGainComputationPolicy,
                                                       GlobalQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_greedy_round_maxnet(
  InitialPartitionerAlgorithm::greedy_round_maxnet,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy,
                                                       MaxNetGainComputationPolicy,
                                                       RoundRobinQueueSelectionPolicy>(hypergraph, config);
});
static Registrar<InitialPartitioningFactory> reg_pool(
  InitialPartitionerAlgorithm::pool,
  [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* {
  return new PoolInitialPartitioner(hypergraph, config);
});

int main(int argc, char* argv[]) {
  po::options_description desc("Allowed options");
  desc.add_options()("help", "show help message")
    ("verbose", po::value<bool>(), "Verbose partitioner output")
    ("hgr", po::value<std::string>(), "Filename of the hypergraph to be partitioned")
    ("k", po::value<PartitionID>(), "Number of partitions")
    ("e", po::value<double>(), "Imbalance parameter epsilon")
    ("seed", po::value<int>(), "Seed for random number generator")
    ("mode", po::value<std::string>(), "(rb) recursive bisection, (direct) direct k-way")
    ("init-remove-hes", po::value<bool>(), "Initially remove parallel hyperedges before partitioning")
    ("nruns", po::value<int>(), "# initial partition trials, the final bisection correspondsto the one with the smallest cut")
    ("part", po::value<std::string>(), "Initial Partitioner: hMetis (default), PaToH or KaHyPar")
    ("init-technique", po::value<std::string>(), "If part=KaHyPar: flat (flat) or multilevel (multi) initial partitioning")
    ("init-mode", po::value<std::string>(), "If part=KaHyPar: direct (direct) or recursive bisection (rb) initial partitioning")
    ("init-algo", po::value<std::string>(), "If part=KaHyPar, used initial partitioning algorithm")
    ("init-alpha", po::value<double>(), "Restrict initial partitioning epsilon to init-alpha*epsilon")
    ("part-path", po::value<std::string>(), "Path to Initial Partitioner Binary")
    ("vcycles", po::value<int>(), "# v-cycle iterations")
    ("cmaxnet", po::value<HyperedgeID>(), "Any hyperedges larger than cmaxnet are removed from the hypergraph before partition (disable:-1 (default))")
    ("remove-always-cut-hes", po::value<bool>(), "Any hyperedges whose accumulated pin-weight is larger than Lmax will always be a cut HE and can therefore be removed (default: false)")
    ("ctype", po::value<std::string>(), "Coarsening: Scheme to be used: heavy_full (default), heavy_heuristic, heavy_lazy, hyperedge")
    ("s", po::value<double>(), "Coarsening: The maximum weight of a hypernode in the coarsest is:(s * w(Graph)) / (t * k)")
    ("t", po::value<HypernodeID>(), "Coarsening: Coarsening stops when there are no more than t * k hypernodes left")
    ("rtype", po::value<std::string>(), "Refinement: 2way_fm (default for k=2), her_fm, max_gain_kfm, kfm, lp_refiner")
    ("lp_refiner_max_iterations", po::value<int>(), "Refinement: maximum number of iterations for label propagation based refinement")
    ("stopFM", po::value<std::string>(), "2-Way-FM | HER-FM: Stopping rule \n adaptive1: new implementation based on nGP \n adaptive2: original nGP implementation \n simple: threshold based")
    ("FMreps", po::value<int>(), "2-Way-FM | HER-FM: max. # of local search repetitions on each level (default:1, no limit:-1)")
    ("init-FMreps", po::value<int>(), "local search repetitions during initial partitioning (default:1, no limit:-1)")
    ("i", po::value<int>(), "2-Way-FM | HER-FM: max. # fruitless moves before stopping local search (simple)")
    ("alpha", po::value<double>(), "2-Way-FM: Random Walk stop alpha (adaptive) (infinity: -1)")
    ("file", po::value<std::string>(), "filename of result file");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  std::string result_file;
  if (vm.count("file")) {
    result_file = vm["file"].as<std::string>();
  }

  Configuration config;
  setDefaults(config);
  configurePartitionerFromCommandLineInput(config, vm);

  Randomize::setSeed(config.partition.seed);

  Hypergraph hypergraph(
    io::createHypergraphFromFile(config.partition.graph_filename,
                                 config.partition.k));

// ensure that there are no single-node hyperedges
  HyperedgeID num_single_node_hes = 0;
  HyperedgeID num_unconnected_hns = 0;
  for (const HyperedgeID he : hypergraph.edges()) {
    if (hypergraph.edgeSize(he) == 1) {
      ++num_single_node_hes;
      if (hypergraph.nodeDegree(*hypergraph.pins(he).first) == 1) {
        ++num_unconnected_hns;
      }
      hypergraph.removeEdge(he, false);
    }
  }

  config.partition.total_graph_weight = hypergraph.totalWeight();

  config.coarsening.contraction_limit =
    config.coarsening.contraction_limit_multiplier * config.partition.k;

  config.coarsening.hypernode_weight_fraction =
    config.coarsening.max_allowed_weight_multiplier
    / config.coarsening.contraction_limit;

  config.partition.perfect_balance_part_weights[0] = ceil(
    config.partition.total_graph_weight
    / static_cast<double>(config.partition.k));
  config.partition.perfect_balance_part_weights[1] =
    config.partition.perfect_balance_part_weights[0];

  config.partition.max_part_weights[0] = (1 + config.partition.epsilon)
                                         * config.partition.perfect_balance_part_weights[0];
  config.partition.max_part_weights[1] = config.partition.max_part_weights[0];

  config.coarsening.max_allowed_node_weight =
    config.coarsening.hypernode_weight_fraction
    * config.partition.total_graph_weight;
  config.fm_local_search.beta = log(hypergraph.numNodes());

// We use hMetis-RB as initial partitioner. If called to partition a graph into k parts
// with an UBfactor of b, the maximal allowed partition size will be 0.5+(b/100)^(log2(k)) n.
// In order to provide a balanced initial partitioning, we determine the UBfactor such that
// the maximal allowed partiton size corresponds to our upper bound i.e.
// (1+epsilon) * ceil(total_weight / k).
  double exp = 1.0 / log2(config.partition.k);
  config.partition.hmetis_ub_factor =
    50.0
    * (2 * pow((1 + config.partition.epsilon), exp)
       * pow(
         ceil(
           static_cast<double>(config.partition.total_graph_weight)
           / config.partition.k)
         / config.partition.total_graph_weight,
         exp) - 1);

  io::printPartitionerConfiguration(config);

  if (num_single_node_hes > 0) {
    LOG(
      "\033[1m\033[31m" << "Removed " << num_single_node_hes << " hyperedges with |e|=1" << "\033[0m");
    LOG(
      "\033[1m\033[31m" << "===> " << num_unconnected_hns << " unconnected HNs could have been removed"
      << "\033[0m");
  }

  io::printHypergraphInfo(hypergraph,
                          config.partition.graph_filename.substr(
                            config.partition.graph_filename.find_last_of("/") + 1));

// the main partitioner should track stats
  config.partition.collect_stats = true;

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
