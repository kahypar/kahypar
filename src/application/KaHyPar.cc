#include <string>

// If not defined, extensive self-verification is performed, which has high impact on
// total running time.
#define NSELF_VERIFICATION

#include "../lib/macros.h"
#include "../lib/definitions.h"
#include "../lib/datastructure/Hypergraph.h"
#include "../lib/io/HypergraphIO.h"
#include "../lib/io/PartitioningOutput.h"
#include "../partition/Configuration.h"
#include "../partition/Coarsener.h"
#include "../partition/Rater.h"
#include "../partition/Partitioner.h"
#include "../partition/Metrics.h"
#include "../tools/RandomFunctions.h"

using datastructure::Hypergraph;
using partition::Rater;
using partition::Coarsener;
using partition::Partitioner;
using partition::FirstRatingWins;
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

static double HYPERNODE_WEIGHT_FRACTION = 0.0375;
static HypernodeID COARSENING_LIMIT = 200;

int main (int argc, char *argv[]) {
  typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
  typedef Coarsener<HypergraphType, FirstWinsRater> FirstWinsCoarsener;
  typedef Configuration<HypergraphType> PartitionConfig;
  typedef Partitioner<HypergraphType, FirstWinsCoarsener> HypergraphPartitioner;

  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  
  PartitionConfig config;
  config.partitioning.k = 2;
  config.partitioning.seed = -1;
  config.partitioning.epsilon = 0.05;
  config.partitioning.initial_partitioning_attempts = 10;
  config.partitioning.graph_filename = "/home/schlag/repo/schlag_git/benchmark_instances/avqsmall.hgr";
  config.partitioning.graph_partition_filename = "/home/schlag/repo/schlag_git/benchmark_instances/avqsmall.hgr.part.2.KaHyPar";
  config.partitioning.coarse_graph_filename = "coarse_test.hgr";
  config.partitioning.coarse_graph_partition_filename = "coarse_test.hgr.part.2";

  Randomize::setSeed(config.partitioning.seed);
  
  io::readHypergraphFile(config.partitioning.graph_filename, num_hypernodes, num_hyperedges,
                         index_vector, edge_vector);
  HypergraphType hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);

  HypernodeWeight hypergraph_weight = 0;
  forall_hypernodes(hn, hypergraph) {
    hypergraph_weight += hypergraph.nodeWeight(*hn);
  } endfor

  config.partitioning.partition_size_upper_bound = (1 + config.partitioning.epsilon)
        * ceil(hypergraph_weight/static_cast<double>(config.partitioning.k));
  config.coarsening.threshold_node_weight = HYPERNODE_WEIGHT_FRACTION * hypergraph.numNodes();
  config.coarsening.minimal_node_count = COARSENING_LIMIT;

  config.two_way_fm.stopping_rule = StoppingRule::ADAPTIVE;
  config.two_way_fm.max_number_of_fruitless_moves = 50;
  config.two_way_fm.alpha = 4;
  config.two_way_fm.beta = log(num_hypernodes);

  io::printPartitionerConfiguration(config);
  io::printHypergraphInfo(hypergraph, config.partitioning.graph_filename.substr(
      config.partitioning.graph_filename.find_last_of("/")+1));

  HypergraphPartitioner partitioner(config);
  partitioner.partition(hypergraph);

  io::printPartitionerConfiguration(config);
  io::printHypergraphInfo(hypergraph, config.partitioning.graph_filename.substr(
      config.partitioning.graph_filename.find_last_of("/")+1));
  io::printPartitioningResults(hypergraph);

  io::writePartitionFile(hypergraph, config.partitioning.graph_partition_filename);
    
  return 0;
}
