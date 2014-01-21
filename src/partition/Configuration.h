#ifndef PARTITION_CONFIGURATION_H_
#define PARTITION_CONFIGURATION_H_

#include <limits>
#include <iomanip>
#include <string>
#include <sstream>

namespace partition {

template <class Hypergraph>
struct Configuration{
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::PartitionID PartitionID;
  
  struct CoarseningParameters {
    HypernodeWeight threshold_node_weight;
    HypernodeID minimal_node_count;
    CoarseningParameters() :
        threshold_node_weight(0),
        minimal_node_count(0) {}
  };

  struct PartitioningParameters {
    PartitionID k;
    int seed;
    int initial_partitioning_attempts;
    double epsilon;
    HypernodeWeight partition_size_upper_bound;
    std::string graph_filename;
    std::string graph_partition_filename;
    std::string coarse_graph_filename;
    std::string coarse_graph_partition_filename;
    PartitioningParameters() :
        k(2),
        seed(0),
        initial_partitioning_attempts(1),
        epsilon(1.0),
        partition_size_upper_bound(std::numeric_limits<HypernodeWeight>::max()),
        graph_filename(),
        graph_partition_filename(),
        coarse_graph_filename(),
        coarse_graph_partition_filename() {}
  };

  struct TwoWayFMParameters {
    int max_number_of_fruitless_moves;
    TwoWayFMParameters() :
        max_number_of_fruitless_moves(50) {}
  };

  PartitioningParameters partitioning;
  CoarseningParameters coarsening;
  TwoWayFMParameters two_way_fm;
  
  Configuration() :
      partitioning(),
      coarsening(),
      two_way_fm() {}
};

template <class Configuration>
std::string toString(const Configuration& config) {
  std::ostringstream oss;
  oss << std::left;
  oss << "Partitioning Parameters:"  << std::endl;
  oss << std::setw(28) << "  Hypergraph: "  << config.partitioning.graph_filename.substr(
      config.partitioning.graph_filename.find_last_of("/")+1) << std::endl;
  oss << std::setw(28) << "  Partition File: "
      << config.partitioning.graph_partition_filename.substr(
          config.partitioning.graph_partition_filename.find_last_of("/")+1)
      << std::endl;
  oss << std::setw(28) << "  Coarsened Hypergraph: " << config.partitioning.coarse_graph_filename
      << std::endl;
  oss << std::setw(28) << "  Coarsened Partition File: "
      << config.partitioning.coarse_graph_partition_filename
      << std::endl;
  oss << std::setw(28) << "  k: " << config.partitioning.k << std::endl;
  oss << std::setw(28) << "  epsilon: " << config.partitioning.epsilon
      << std::endl;
  oss << std::setw(28) << "  L_max: " << config.partitioning.partition_size_upper_bound
      << std::endl;
  oss << std::setw(28) << "  seed: " << config.partitioning.seed << std::endl;
  oss << std::setw(28) << "  # initial partitionings: "
      << config.partitioning.initial_partitioning_attempts << std::endl;
  oss << "Coarsening Parameters:" << std::endl;
  oss << std::setw(28) << "  max. hypernode weight: " << config.coarsening.threshold_node_weight
      << std::endl;
  oss << std::setw(28) << "  min. # hypernodes: " << config.coarsening.minimal_node_count
      << std::endl;
  oss << "2-Way-FM Refinement Parameters:" << std::endl;
  oss << std::setw(28) << "  max. # fruitless moves: "
      << config.two_way_fm.max_number_of_fruitless_moves;
  return oss.str();
}

} // namespace partition

#endif  // PARTITION_CONFIGURATION_H_
