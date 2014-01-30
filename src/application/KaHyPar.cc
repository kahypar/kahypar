#include <string>
#include <memory>

// If not defined, extensive self-verification is performed, which has high impact on
// total running time.
#define NSELF_VERIFICATION

#include <boost/program_options.hpp>

#include "lib/macros.h"
#include "lib/definitions.h"
#include "lib/datastructure/Hypergraph.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/Configuration.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/Coarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/Partitioner.h"
#include "partition/Metrics.h"
#include "tools/RandomFunctions.h"

namespace po = boost::program_options;

using datastructure::Hypergraph;
using partition::Rater;
using partition::Coarsener;
using partition::ICoarsener;
using partition::Partitioner;
using partition::RandomRatingWins;
using partition::Configuration;
using partition::StoppingRule;

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
void configurePartitionerFromCommandLineInput(Config& config, po::variables_map& vm) { 
  if (vm.count("hgr") && vm.count("e")) {
    config.partitioning.graph_filename = vm["hgr"].as<std::string>();
    config.partitioning.coarse_graph_filename = config.partitioning.graph_filename;
    config.partitioning.coarse_graph_filename.insert(config.partitioning.coarse_graph_filename.find_last_of("/")+1, "coarse_" );
    config.partitioning.graph_partition_filename = config.partitioning.graph_filename + ".part.2.KaHyPar";
    config.partitioning.coarse_graph_partition_filename = config.partitioning.coarse_graph_filename + ".part.2";
    config.partitioning.epsilon = vm["e"].as<double>();

    if (vm.count("seed")) {
      config.partitioning.seed = vm["seed"].as<int>();
    }
    if (vm.count("nruns")) {
      config.partitioning.initial_partitioning_attempts = vm["nruns"].as<int>();
    }
    if (vm.count("s")) {
      config.coarsening.hypernode_weight_fraction = vm["s"].as<double>();
    }
    if (vm.count("t")) {
      config.coarsening.minimal_node_count = vm["t"].as<HypernodeID>();
    }
    if (vm.count("stopFM")) {
      if(vm["stopFM"].as<std::string>() == "simple") {
        config.two_way_fm.stopping_rule = StoppingRule::SIMPLE;
      }
    }
    if (vm.count("i")) {
      config.two_way_fm.max_number_of_fruitless_moves = vm["i"].as<int>();
    }
    if (vm.count("alpha")) {
      config.two_way_fm.alpha = vm["alpha"].as<double>();
    }
    if (vm.count("verbose")) {
      config.partitioning.verbose_output = vm["verbose"].as<bool>();
    }
  } else {
    exit(0);
  }
}

template <typename Config>
void setDefaults(Config& config) {
  config.partitioning.k = 2;
  config.partitioning.epsilon = 0.05;
  config.partitioning.seed = -1;
  config.partitioning.initial_partitioning_attempts = 50;
  config.coarsening.minimal_node_count = 100;
  config.coarsening.hypernode_weight_fraction = 0.0375;
  config.two_way_fm.stopping_rule = StoppingRule::ADAPTIVE;
  config.two_way_fm.max_number_of_fruitless_moves = 100;
  config.two_way_fm.alpha = 4;
}

int main (int argc, char *argv[]) {
  typedef Rater<HypergraphType, defs::RatingType, RandomRatingWins> RandomWinsRater;
  typedef Coarsener<HypergraphType, RandomWinsRater> RandomWinsCoarsener;
  typedef Configuration<HypergraphType> PartitionConfig;
  typedef Partitioner<HypergraphType> HypergraphPartitioner;

  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "show help message")
      ("verbose", po::value<bool>(),       "Verbose partitioner output")
      ("hgr", po::value<std::string>(),    "Filename of the hypergraph to be partitioned")
      ("e",  po::value<double>(),          "Imbalance parameter epsilon")
      ("seed", po::value<int>(),           "Seed for random number generator")
      ("nruns", po::value<int>(),          "# initial partitioning trials, the final bisection corresponds to the one with the smallest cut")
      ("s", po::value<double>(),           "Coarsening: The maximum weight of a representative hypernode is: s * |hypernodes|")
      ("t", po::value<HypernodeID>(),      "Coarsening: Coarsening stopps when there are no more than t hypernodes left")
      ("stopFM", po::value<std::string>(), "2-Way-FM: Stopping rule (adaptive | simple)")
      ("i", po::value<int>(),              "2-Way-FM: max. # fruitless moves before stopping local search (simple)")
      ("alpha", po::value<double>(),       "2-Way-FM: Random Walk stop alpha (adaptive)");
  
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 0;
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
        * ceil(hypergraph_weight/static_cast<double>(config.partitioning.k));
  config.coarsening.threshold_node_weight = config.coarsening.hypernode_weight_fraction * hypergraph.numNodes();
  config.two_way_fm.beta = log(num_hypernodes);

  io::printPartitionerConfiguration(config);
  io::printHypergraphInfo(hypergraph, config.partitioning.graph_filename.substr(
      config.partitioning.graph_filename.find_last_of("/")+1));

  HypergraphPartitioner partitioner(config);
  std::unique_ptr<ICoarsener<HypergraphType> > coarsener(new RandomWinsCoarsener(hypergraph, config));
  partitioner.partition(hypergraph, coarsener);

  io::printPartitioningResults(hypergraph);

  io::writePartitionFile(hypergraph, config.partitioning.graph_partition_filename);
    
  return 0;
}
