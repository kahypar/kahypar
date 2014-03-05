/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

// If not defined, extensive self-verification is performed, which has high impact on
// total running time.
#define NSELF_VERIFICATION
#include <boost/program_options.hpp>

#include <chrono>
#include <memory>
#include <string>

#include "lib/datastructure/Hypergraph.h"
#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "lib/macros.h"
#include "lib/sqlite/SQLiteSerializer.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/Partitioner.h"
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "tools/RandomFunctions.h"

namespace po = boost::program_options;

using datastructure::Hypergraph;
using partition::Rater;
using partition::ICoarsener;
using partition::IRefiner;
using partition::HeuristicHeavyEdgeCoarsener;
using partition::FullHeavyEdgeCoarsener;
using partition::Partitioner;
using partition::RandomRatingWins;
using partition::Configuration;
using partition::StoppingRule;
using partition::TwoWayFMRefiner;
using partition::CoarseningScheme;
using partition::RandomWalkModelStopsSearch;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::nGPRandomWalkStopsSearch;

typedef Hypergraph<defs::HyperNodeID, defs::HyperEdgeID,
                   defs::HyperNodeWeight, defs::HyperEdgeWeight, defs::PartitionID> HypergraphType;
typedef HypergraphType::HypernodeID HypernodeID;
typedef HypergraphType::HypernodeWeight HypernodeWeight;
typedef HypergraphType::HyperedgeID HyperedgeID;
typedef HypergraphType::HyperedgeIndexVector HyperedgeIndexVector;
typedef HypergraphType::HyperedgeVector HyperedgeVector;
typedef HypergraphType::HyperedgeWeightVector HyperedgeWeightVector;
typedef HypergraphType::HypernodeWeightVector HypernodeWeightVector;

template <typename Config>
void configurePartitionerFromCommandLineInput(Config& config, const po::variables_map& vm) {
  if (vm.count("hgr") && vm.count("e")) {
    config.partitioning.graph_filename = vm["hgr"].as<std::string>();
    config.partitioning.coarse_graph_filename = config.partitioning.graph_filename;
    config.partitioning.coarse_graph_filename.insert(config.partitioning.coarse_graph_filename.find_last_of(
                                                       "/") + 1, std::string("PID_") + std::to_string(getpid()) + "_coarse_");
    config.partitioning.graph_partition_filename = config.partitioning.graph_filename + ".part.2.KaHyPar";
    config.partitioning.coarse_graph_partition_filename = config.partitioning.coarse_graph_filename + ".part.2";
    config.partitioning.epsilon = vm["e"].as<double>();

    if (vm.count("seed")) {
      config.partitioning.seed = vm["seed"].as<int>();
    }
    if (vm.count("nruns")) {
      config.partitioning.initial_partitioning_attempts = vm["nruns"].as<int>();
    }
    if (vm.count("vcycles")) {
      config.partitioning.global_search_iterations = vm["vcycles"].as<int>();
    }
    if (vm.count("cmaxnet")) {
      config.partitioning.hyperedge_size_threshold = vm["cmaxnet"].as<HyperedgeID>();
      if (config.partitioning.hyperedge_size_threshold == -1) {
        config.partitioning.hyperedge_size_threshold = std::numeric_limits<HyperedgeID>::max();
      }
    }
    if (vm.count("ctype")) {
      if (vm["ctype"].as<std::string>() == "heavy_heuristic") {
        config.coarsening.scheme = CoarseningScheme::HEAVY_EDGE_HEURISTIC;
      }
    }
    if (vm.count("s")) {
      config.coarsening.hypernode_weight_fraction = vm["s"].as<double>();
    }
    if (vm.count("t")) {
      config.coarsening.minimal_node_count = vm["t"].as<HypernodeID>();
    }
    if (vm.count("stopFM")) {
      if (vm["stopFM"].as<std::string>() == "simple") {
        config.two_way_fm.stopping_rule = StoppingRule::SIMPLE;
      } else if (vm["stopFM"].as<std::string>() == "adaptive1") {
        config.two_way_fm.stopping_rule = StoppingRule::ADAPTIVE1;
      } else if (vm["stopFM"].as<std::string>() == "adaptive2") {
        config.two_way_fm.stopping_rule = StoppingRule::ADAPTIVE2;
      } else {
        std::cout << "Illegal stopFM option! Exiting..." << std::endl;
        exit(0);
      }
    }
    if (vm.count("FMreps")) {
      config.two_way_fm.num_repetitions = vm["FMreps"].as<int>();
      if (config.two_way_fm.num_repetitions == -1) {
        config.two_way_fm.num_repetitions = std::numeric_limits<int>::max();
      }
    }
    if (vm.count("i")) {
      config.two_way_fm.max_number_of_fruitless_moves = vm["i"].as<int>();
    }
    if (vm.count("alpha")) {
      config.two_way_fm.alpha = vm["alpha"].as<double>();
      if (config.two_way_fm.alpha == -1) {
        config.two_way_fm.alpha = std::numeric_limits<double>::max();
      }
    }
    if (vm.count("verbose")) {
      config.partitioning.verbose_output = vm["verbose"].as<bool>();
    }
  } else {
    std::cout << "Parameter error! Exiting..." << std::endl;
    exit(0);
  }
}

template <typename Config>
void setDefaults(Config& config) {
  config.partitioning.k = 2;
  config.partitioning.epsilon = 0.05;
  config.partitioning.seed = -1;
  config.partitioning.initial_partitioning_attempts = 10;
  config.partitioning.global_search_iterations = 10;
  config.partitioning.hyperedge_size_threshold = 50;
  config.coarsening.scheme = CoarseningScheme::HEAVY_EDGE_FULL;
  config.coarsening.minimal_node_count = 100;
  config.coarsening.hypernode_weight_fraction = 0.0375;
  config.two_way_fm.stopping_rule = StoppingRule::ADAPTIVE1;
  config.two_way_fm.num_repetitions = 1;
  config.two_way_fm.max_number_of_fruitless_moves = 100;
  config.two_way_fm.alpha = 4;
}

int main(int argc, char* argv[]) {
  typedef Rater<HypergraphType, defs::RatingType, RandomRatingWins> RandomWinsRater;
  typedef HeuristicHeavyEdgeCoarsener<HypergraphType, RandomWinsRater> RandomWinsHeuristicCoarsener;
  typedef FullHeavyEdgeCoarsener<HypergraphType, RandomWinsRater> RandomWinsFullCoarsener;
  typedef Configuration<HypergraphType> PartitionConfig;
  typedef Partitioner<HypergraphType> HypergraphPartitioner;

  typedef std::chrono::time_point<std::chrono::high_resolution_clock> HighResClockTimepoint;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "show help message")
    ("verbose", po::value<bool>(), "Verbose partitioner output")
    ("hgr", po::value<std::string>(), "Filename of the hypergraph to be partitioned")
    ("e", po::value<double>(), "Imbalance parameter epsilon")
    ("seed", po::value<int>(), "Seed for random number generator")
    ("nruns", po::value<int>(),
    "# initial partitioning trials, the final bisection corresponds to the one with the smallest cut")
    ("vcycles", po::value<int>(), "# v-cycle iterations")
    ("cmaxnet", po::value<HyperedgeID>(), "Any hyperedges larger than cmaxnet are removed from the hypergraph before partitioning (disable:-1)")
    ("ctype", po::value<std::string>(), "Coarsening: Scheme to be used: heavy_full (default), heavy_heuristic")
    ("s", po::value<double>(),
    "Coarsening: The maximum weight of a representative hypernode is: s * |hypernodes|")
    ("t", po::value<HypernodeID>(), "Coarsening: Coarsening stopps when there are no more than t hypernodes left")
    ("stopFM", po::value<std::string>(), "2-Way-FM: Stopping rule \n adaptive1: new implementation based on nGP \n adaptive2: original nGP implementation \n simple: threshold based")
    ("FMreps", po::value<int>(), "2-Way-FM: max. # of local search repetitions on each level (default:1, no limit:-1)")
    ("i", po::value<int>(), "2-Way-FM: max. # fruitless moves before stopping local search (simple)")
    ("alpha", po::value<double>(), "2-Way-FM: Random Walk stop alpha (adaptive) (infinity: -1)")
    ("db", po::value<std::string>(), "experiment db filename");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
  }

  std::string db_name("temp.db");
  if (vm.count("db")) {
    db_name = vm["db"].as<std::string>();
  }

  PartitionConfig config;
  setDefaults(config);
  configurePartitionerFromCommandLineInput(config, vm);

  Randomize::setSeed(config.partitioning.seed);

  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;

  io::readHypergraphFile(config.partitioning.graph_filename, num_hypernodes, num_hyperedges,
                         index_vector, edge_vector);
  HypergraphType hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);

  HypernodeWeight hypergraph_weight = 0;
  forall_hypernodes(hn, hypergraph) {
    hypergraph_weight += hypergraph.nodeWeight(*hn);
  } endfor

  config.partitioning.partition_size_upper_bound = (1 + config.partitioning.epsilon)
                                                   * ceil(hypergraph_weight / static_cast<double>(config.partitioning.k));
  config.coarsening.threshold_node_weight = config.coarsening.hypernode_weight_fraction * hypergraph.numNodes();
  config.two_way_fm.beta = log(num_hypernodes);

  io::printPartitionerConfiguration(config);
  io::printHypergraphInfo(hypergraph, config.partitioning.graph_filename.substr(
                            config.partitioning.graph_filename.find_last_of("/") + 1));

  HypergraphPartitioner partitioner(config);
  std::unique_ptr<ICoarsener<HypergraphType> > coarsener(nullptr);
  if (config.coarsening.scheme == CoarseningScheme::HEAVY_EDGE_FULL) {
    coarsener.reset(new RandomWinsFullCoarsener(hypergraph, config));
  } else {
    coarsener.reset(new RandomWinsHeuristicCoarsener(hypergraph, config));
  }

  std::unique_ptr<IRefiner<HypergraphType> > refiner(nullptr);
  switch (config.two_way_fm.stopping_rule) {
    case StoppingRule::SIMPLE:
      refiner.reset(new TwoWayFMRefiner<HypergraphType,
                                        NumberOfFruitlessMovesStopsSearch>(hypergraph, config));
      break;
    case StoppingRule::ADAPTIVE1:
      refiner.reset(new TwoWayFMRefiner<HypergraphType,
                                        RandomWalkModelStopsSearch>(hypergraph, config));
      break;
    case StoppingRule::ADAPTIVE2:
      refiner.reset(new TwoWayFMRefiner<HypergraphType,
                                        nGPRandomWalkStopsSearch>(hypergraph, config));
      break;
  }

  HighResClockTimepoint start;
  HighResClockTimepoint end;

  start = std::chrono::high_resolution_clock::now();
  partitioner.partition(hypergraph, *coarsener, *refiner);
  end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed_seconds = end - start;

  io::printPartitioningResults(hypergraph, elapsed_seconds);
  io::writePartitionFile(hypergraph, config.partitioning.graph_partition_filename);

  std::remove(config.partitioning.coarse_graph_filename.c_str());
  std::remove(config.partitioning.coarse_graph_partition_filename.c_str());

  serializer::SQLiteBenchmarkSerializer sqlite_serializer(db_name);
  sqlite_serializer.dumpPartitioningResult(config, hypergraph, elapsed_seconds);
  return 0;
}
