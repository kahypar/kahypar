#include <string>

// If not defined, extensive self-verification is performed, which has high impact on
// total running time.
#define NSELF_VERIFICATION

#include "../lib/macros.h"
#include "../lib/definitions.h"
#include "../lib/datastructure/Hypergraph.h"
#include "../partition/Configuration.h"
#include "../partition/Coarsener.h"
#include "../partition/Rater.h"
#include "../partition/Partitioner.h"
#include "../partition/Metrics.h"

using datastructure::Hypergraph;
using partition::Rater;
using partition::Coarsener;
using partition::Partitioner;
using partition::FirstRatingWins;
using partition::Configuration;

typedef Hypergraph<defs::HyperNodeID, defs::HyperEdgeID,
                   defs::HyperNodeWeight, defs::HyperEdgeWeight> HypergraphType;
typedef HypergraphType::HypernodeID HypernodeID;
typedef HypergraphType::HyperedgeID HyperedgeID;
typedef HypergraphType::HyperedgeIndexVector HyperedgeIndexVector;
typedef HypergraphType::HyperedgeVector HyperedgeVector;
typedef HypergraphType::HyperedgeWeightVector HyperedgeWeightVector;
typedef HypergraphType::HypernodeWeightVector HypernodeWeightVector;

static double HYPERNODE_WEIGHT_FRACTION = 0.0375;
static HypernodeID COARSENING_LIMIT = 100;

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
  config.graph_filename = "/home/schlag/repo/schlag_git/benchmark_instances/ibm03.hgr";
  config.coarse_graph_filename = "coarse_test.hgr";
  config.partition_filename = "coarse_test.hgr.part.2";
  config.coarsening_limit = COARSENING_LIMIT;
  
  io::readHypergraphFile(config.graph_filename, num_hypernodes, num_hyperedges, index_vector,
                         edge_vector);
  HypergraphType hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);
  config.threshold_node_weight = HYPERNODE_WEIGHT_FRACTION * hypergraph.numNodes();

  HypergraphPartitioner partitioner(config);
  partitioner.partition(hypergraph);

  std::cout << "mincut    = " << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "imbalance = " << metrics::imbalance(hypergraph) << std::endl;
    
  return 0;
}
