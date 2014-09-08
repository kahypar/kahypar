/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/
#include <boost/program_options.hpp>

#include <chrono>
#include <memory>
#include <string>

#include "lib/core/Factory.h"
#include "lib/core/PolicyRegistry.h"
#include "lib/core/StaticDispatcher.h"
#include "lib/core/Typelist.h"
#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "lib/macros.h"
#include "lib/serializer/SQLPlotToolsSerializer.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/Partitioner.h"
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/HyperedgeCoarsener.h"
#include "partition/coarsening/HyperedgeRatingPolicies.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/LazyUpdateHeavyEdgeCoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/FMFactoryExecutor.h"
#include "partition/refinement/HyperedgeFMRefiner.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"
#include "tools/RandomFunctions.h"

namespace po = boost::program_options;

using core::StaticDispatcher;
using core::Typelist;
using core::NullType;
using core::PolicyRegistry;
using core::NullPolicy;
using core::Factory;

using partition::Rater;
using partition::ICoarsener;
using partition::IRefiner;
using partition::HeuristicHeavyEdgeCoarsener;
using partition::FullHeavyEdgeCoarsener;
using partition::LazyUpdateHeavyEdgeCoarsener;
using partition::Partitioner;
using partition::RandomRatingWins;
using partition::Configuration;
using partition::HyperedgeCoarsener;
using partition::EdgeWeightDivMultPinWeight;
using partition::FMFactoryExecutor;
using partition::KFMFactoryExecutor;
using partition::TwoWayFMRefiner;
using partition::HyperedgeFMRefiner;
using partition::KWayFMRefiner;
using partition::StoppingPolicy;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;
using partition::CloggingPolicy;
using partition::OnlyRemoveIfBothQueuesClogged;
using partition::RemoveOnlyTheCloggingEntry;
using partition::DoNotRemoveAnyCloggingEntriesAndResetEligiblity;
using partition::RefinerParameters;

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

void configurePartitionerFromCommandLineInput(Configuration& config, const po::variables_map& vm) {
  if (vm.count("hgr") && vm.count("e") && vm.count("k")) {
    config.partition.graph_filename = vm["hgr"].as<std::string>();
    config.partition.k = vm["k"].as<PartitionID>();

    config.partition.coarse_graph_filename = std::string("/tmp/PID_")
                                             + std::to_string(getpid())
                                             + std::string("_coarse_")
                                             + config.partition.graph_filename.substr(
      config.partition.graph_filename.find_last_of("/")
      + 1);
    config.partition.graph_partition_filename = config.partition.graph_filename + ".part."
                                                + std::to_string(config.partition.k)
                                                + ".KaHyPar";
    config.partition.coarse_graph_partition_filename = config.partition.coarse_graph_filename
                                                       + ".part."
                                                       + std::to_string(config.partition.k);
    config.partition.epsilon = vm["e"].as<double>();

    if (vm.count("seed")) {
      config.partition.seed = vm["seed"].as<int>();
    }
    if (vm.count("nruns")) {
      config.partition.initial_partitioning_attempts = vm["nruns"].as<int>();
    }
    if (vm.count("vcycles")) {
      config.partition.global_search_iterations = vm["vcycles"].as<int>();
    }
    if (vm.count("cmaxnet")) {
      config.partition.hyperedge_size_threshold = vm["cmaxnet"].as<HyperedgeID>();
      if (config.partition.hyperedge_size_threshold == -1) {
        config.partition.hyperedge_size_threshold = std::numeric_limits<HyperedgeID>::max();
      }
    }
    if (vm.count("ctype")) {
      config.coarsening.scheme = vm["ctype"].as<std::string>();
    }
    if (vm.count("s")) {
      config.coarsening.hypernode_weight_fraction = vm["s"].as<double>();
    }
    if (vm.count("t")) {
      config.coarsening.minimal_node_count = vm["t"].as<HypernodeID>();
    }
    if (vm.count("stopFM")) {
      config.two_way_fm.stopping_rule = vm["stopFM"].as<std::string>();
      config.her_fm.stopping_rule = vm["stopFM"].as<std::string>();
    }
    if (vm.count("FMreps")) {
      config.two_way_fm.num_repetitions = vm["FMreps"].as<int>();
      config.her_fm.num_repetitions = vm["FMreps"].as<int>();
      if (config.two_way_fm.num_repetitions == -1) {
        config.two_way_fm.num_repetitions = std::numeric_limits<int>::max();
        config.her_fm.num_repetitions = std::numeric_limits<int>::max();
      }
    }
    if (vm.count("i")) {
      config.two_way_fm.max_number_of_fruitless_moves = vm["i"].as<int>();
      config.her_fm.max_number_of_fruitless_moves = vm["i"].as<int>();
    }
    if (vm.count("alpha")) {
      config.two_way_fm.alpha = vm["alpha"].as<double>();
      if (config.two_way_fm.alpha == -1) {
        config.two_way_fm.alpha = std::numeric_limits<double>::max();
      }
    }
    if (vm.count("verbose")) {
      config.partition.verbose_output = vm["verbose"].as<bool>();
    }
    if (vm.count("rtype")) {
      if (vm["rtype"].as<std::string>() == "twoway_fm") {
        config.two_way_fm.active = true;
        config.her_fm.active = false;
      } else if (vm["rtype"].as<std::string>() == "her_fm") {
        config.two_way_fm.active = false;
        config.her_fm.active = true;
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
  config.partition.epsilon = 0.05;
  config.partition.seed = -1;
  config.partition.initial_partitioning_attempts = 10;
  config.partition.global_search_iterations = 10;
  config.partition.hyperedge_size_threshold = -1;
  config.coarsening.scheme = "heavy_full";
  config.coarsening.minimal_node_count = 100;
  config.coarsening.hypernode_weight_fraction = 0.0375;
  config.two_way_fm.stopping_rule = "simple";
  config.two_way_fm.num_repetitions = 1;
  config.two_way_fm.max_number_of_fruitless_moves = 100;
  config.two_way_fm.alpha = 4;
  config.her_fm.stopping_rule = "simple";
  config.her_fm.num_repetitions = 1;
  config.her_fm.max_number_of_fruitless_moves = 10;
}

struct CoarsenerFactoryParameters {
  CoarsenerFactoryParameters(Hypergraph& hgr, Configuration& conf) :
    hypergraph(hgr),
    config(conf) { }
  Hypergraph& hypergraph;
  Configuration& config;
};

int main(int argc, char* argv[]) {
  typedef Rater<defs::RatingType, RandomRatingWins> RandomWinsRater;
  typedef HeuristicHeavyEdgeCoarsener<RandomWinsRater> RandomWinsHeuristicCoarsener;
  typedef FullHeavyEdgeCoarsener<RandomWinsRater> RandomWinsFullCoarsener;
  typedef LazyUpdateHeavyEdgeCoarsener<RandomWinsRater> RandomWinsLazyUpdateCoarsener;
  typedef HyperedgeCoarsener<EdgeWeightDivMultPinWeight> HyperedgeCoarsener;
  typedef FMFactoryExecutor<TwoWayFMRefiner> TwoWayFMFactoryExecutor;
  typedef FMFactoryExecutor<HyperedgeFMRefiner> HyperedgeFMFactoryExecutor;
  typedef KFMFactoryExecutor<KWayFMRefiner> KWayFMFactoryExecutor;
  typedef StaticDispatcher<TwoWayFMFactoryExecutor,
                           PolicyBase,
                           TYPELIST_3(NumberOfFruitlessMovesStopsSearch, RandomWalkModelStopsSearch,
                                      nGPRandomWalkStopsSearch),
                           PolicyBase,
                           TYPELIST_1(OnlyRemoveIfBothQueuesClogged),
                           IRefiner*> TwoWayFMFactoryDispatcher;
  typedef StaticDispatcher<HyperedgeFMFactoryExecutor,
                           PolicyBase,
                           TYPELIST_3(NumberOfFruitlessMovesStopsSearch, RandomWalkModelStopsSearch,
                                      nGPRandomWalkStopsSearch),
                           PolicyBase,
                           TYPELIST_1(OnlyRemoveIfBothQueuesClogged),
                           IRefiner*> HyperedgeFMFactoryDispatcher;
  typedef StaticDispatcher<KWayFMFactoryExecutor,
                           PolicyBase,
                           TYPELIST_3(NumberOfFruitlessMovesStopsSearch, RandomWalkModelStopsSearch,
                                      nGPRandomWalkStopsSearch),
                           PolicyBase,
                           TYPELIST_1(NullPolicy),
                           IRefiner*> KWayFMFactoryDispatcher;
  typedef Factory<ICoarsener, std::string,
                  ICoarsener* (*)(CoarsenerFactoryParameters&),
                  CoarsenerFactoryParameters> CoarsenerFactory;

  PolicyRegistry::getInstance().registerPolicy("simple", new NumberOfFruitlessMovesStopsSearch());
  PolicyRegistry::getInstance().registerPolicy("adaptive1", new RandomWalkModelStopsSearch());
  PolicyRegistry::getInstance().registerPolicy("adaptive2", new nGPRandomWalkStopsSearch());

  CoarsenerFactory::getInstance().registerObject(
    "hyperedge",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new HyperedgeCoarsener(p.hypergraph, p.config);
    }
    );
  CoarsenerFactory::getInstance().registerObject(
    "heavy_heuristic",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new RandomWinsHeuristicCoarsener(p.hypergraph, p.config);
    }
    );
  CoarsenerFactory::getInstance().registerObject(
    "heavy_full",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new RandomWinsFullCoarsener(p.hypergraph, p.config);
    }
    );

  CoarsenerFactory::getInstance().registerObject(
    "heavy_lazy",
    [](CoarsenerFactoryParameters& p) -> ICoarsener* {
      return new RandomWinsLazyUpdateCoarsener(p.hypergraph, p.config);
    }
    );

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "show help message")
    ("verbose", po::value<bool>(), "Verbose partitioner output")
    ("hgr", po::value<std::string>(), "Filename of the hypergraph to be partitioned")
    ("k", po::value<PartitionID>(), "Number of partitions")
    ("e", po::value<double>(), "Imbalance parameter epsilon")
    ("seed", po::value<int>(), "Seed for random number generator")
    ("nruns", po::value<int>(),
    "# initial partition trials, the final bisection corresponds to the one with the smallest cut")
    ("vcycles", po::value<int>(), "# v-cycle iterations")
    ("cmaxnet", po::value<HyperedgeID>(), "Any hyperedges larger than cmaxnet are removed from the hypergraph before partition (disable:-1 (default))")
    ("ctype", po::value<std::string>(), "Coarsening: Scheme to be used: heavy_full (default), heavy_heuristic, heavy_lazy, hyperedge")
    ("s", po::value<double>(),
    "Coarsening: The maximum weight of a representative hypernode is: s * |hypernodes|")
    ("t", po::value<HypernodeID>(), "Coarsening: Coarsening stopps when there are no more than t hypernodes left")
    ("rtype", po::value<std::string>(), "Refinement: 2way_fm (default), her_fm")
    ("stopFM", po::value<std::string>(), "2-Way-FM | HER-FM: Stopping rule \n adaptive1: new implementation based on nGP \n adaptive2: original nGP implementation \n simple: threshold based")
    ("FMreps", po::value<int>(), "2-Way-FM | HER-FM: max. # of local search repetitions on each level (default:1, no limit:-1)")
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

  std::string result_file("temp.txt");
  if (vm.count("file")) {
    result_file = vm["file"].as<std::string>();
  }

  Configuration config;
  setDefaults(config);
  configurePartitionerFromCommandLineInput(config, vm);

  Randomize::setSeed(config.partition.seed);

  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;

  io::readHypergraphFile(config.partition.graph_filename, num_hypernodes, num_hyperedges,
                         index_vector, edge_vector);
  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector,
                        config.partition.k);

  HypernodeWeight hypergraph_weight = 0;
  for (auto && hn : hypergraph.nodes()) {
    hypergraph_weight += hypergraph.nodeWeight(hn);
  }

  config.partition.max_part_weight = (1 + config.partition.epsilon)
                                     * ceil(hypergraph_weight /
                                            static_cast<double>(config.partition.k));
  config.partition.total_graph_weight = hypergraph_weight;

  // temporary workaround; default corresponds to bipartitioning, so
  // in case of no s parameter is given as input, we calculate the correct
  // one like KaSPar.
  if (config.coarsening.hypernode_weight_fraction == 0.0375) {
    config.coarsening.hypernode_weight_fraction = (1.5 * hypergraph.numNodes()) /
                                                  (20 * config.partition.k);
  }


  config.coarsening.threshold_node_weight = config.coarsening.hypernode_weight_fraction *
                                            hypergraph_weight;
  config.two_way_fm.beta = log(num_hypernodes);

  // We use hMetis-RB as initial partitioner. If called to partition a graph into k parts
  // with an UBfactor of b, the maximal allowed partition size will be 0.5+(b/100)^(log2(k)) n.
  // In order to provide a balanced initial partitioning, we determine the UBfactor such that
  // the maximal allowed partiton size corresponds to our upper bound i.e.
  // (1+epsilon) * ceil(total_weight / k).
  double exp = 1.0 / log2(config.partition.k);
  config.partition.hmetis_ub_factor =
    50.0 * (2 * pow((1 + config.partition.epsilon), exp)
            * pow(ceil(static_cast<double>(config.partition.total_graph_weight)
                       / config.partition.k) / config.partition.total_graph_weight, exp) - 1);

  io::printPartitionerConfiguration(config);
  io::printHypergraphInfo(hypergraph, config.partition.graph_filename.substr(
                            config.partition.graph_filename.find_last_of("/") + 1));

#ifdef GATHER_STATS
  LOG("*******************************");
  LOG("***** GATHER_STATS ACTIVE *****");
  LOG("*******************************");
#endif

  Partitioner partitioner(config);
  CoarsenerFactoryParameters coarsener_parameters(hypergraph, config);

  std::unique_ptr<ICoarsener> coarsener(
    CoarsenerFactory::getInstance().createObject(config.coarsening.scheme, coarsener_parameters)
    );

  std::unique_ptr<CloggingPolicy> clogging_policy(new OnlyRemoveIfBothQueuesClogged());
  RefinerParameters refiner_parameters(hypergraph, config);
  std::unique_ptr<IRefiner> refiner(nullptr);

  if (config.two_way_fm.active) {
    if (config.partition.k == 2) {
      TwoWayFMFactoryExecutor exec;
      refiner.reset(TwoWayFMFactoryDispatcher::go(
                      PolicyRegistry::getInstance().getPolicy(config.two_way_fm.stopping_rule),
                      *(clogging_policy.get()),
                      exec, refiner_parameters));
    } else {
      std::unique_ptr<NullPolicy> null_policy(new NullPolicy());
      KWayFMFactoryExecutor exec;
      refiner.reset(KWayFMFactoryDispatcher::go(
                      PolicyRegistry::getInstance().getPolicy(config.two_way_fm.stopping_rule),
                      *(null_policy.get()),
                      exec, refiner_parameters));
    }
  } else {
    HyperedgeFMFactoryExecutor exec;
    refiner.reset(HyperedgeFMFactoryDispatcher::go(
                    PolicyRegistry::getInstance().getPolicy(config.her_fm.stopping_rule),
                    *(clogging_policy.get()),
                    exec, refiner_parameters));
  }

  HighResClockTimepoint start;
  HighResClockTimepoint end;

  start = std::chrono::high_resolution_clock::now();
  partitioner.partition(hypergraph, *coarsener, *refiner);
  end = std::chrono::high_resolution_clock::now();

#ifndef NDEBUG
  for (auto && he : hypergraph.edges()) {
    ASSERT([&]() -> bool {
             HypernodeID num_pins = 0;
             for (PartitionID i = 0; i < config.partition.k; ++i) {
               num_pins += hypergraph.pinCountInPart(he, i);
             }
             return num_pins == hypergraph.edgeSize(he);
           } (),
           "Incorrect calculation of pin counts");
  }
#endif

  std::chrono::duration<double> elapsed_seconds = end - start;

  io::printPartitioningStatistics(*coarsener, *refiner);
  io::printPartitioningResults(hypergraph, elapsed_seconds, partitioner.timings());
  io::writePartitionFile(hypergraph, config.partition.graph_partition_filename);

  std::remove(config.partition.coarse_graph_filename.c_str());
  std::remove(config.partition.coarse_graph_partition_filename.c_str());

  SQLPlotToolsSerializer::serialize(config, hypergraph, *coarsener, *refiner, elapsed_seconds,
                                    partitioner.timings(), result_file);
  return 0;
}
