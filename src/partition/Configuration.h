#ifndef PARTITION_CONFIGURATION_H_
#define PARTITION_CONFIGURATION_H_

#include <string>
#include <sstream>
#include <iomanip>

namespace partition {

template <class Hypergraph>
struct Configuration{
  struct CoarseningParameters {
    typename Hypergraph::HypernodeWeight threshold_node_weight;
    typename Hypergraph::HypernodeID minimal_node_count;
    CoarseningParameters() :
        threshold_node_weight(0),
        minimal_node_count(0) {}
  };

  struct PartitioningParameters {
    int k;
    int seed;
    int initial_partitioning_attempts;
    double balance_constraint;
    std::string graph_filename;
    std::string coarse_graph_filename;
    std::string partition_filename;
    PartitioningParameters() :
        k(2),
        seed(0),
        initial_partitioning_attempts(1),
        balance_constraint(1),
        graph_filename(),
        coarse_graph_filename(),
        partition_filename() {}
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
  oss << std::setw(28) << "  Coarsened Hypergraph: " << config.partitioning.coarse_graph_filename
      << std::endl;
  oss << std::setw(28) << "  Partitioning File: " << config.partitioning.partition_filename
      << std::endl;
  oss << std::setw(28) << "  k: " << config.partitioning.k << std::endl;
  oss << std::setw(28) << "  balance constraint: " << config.partitioning.balance_constraint
      << std::endl;
  oss << std::setw(28) << "  seed: " << config.partitioning.seed << std::endl;
  oss << std::setw(28) << "  # initial partitionings: " << config.partitioning.initial_partitioning_attempts
      << std::endl;
  oss << "Coarsening Parameters:" << std::endl;
  oss << std::setw(28) << "  max. hypernode weight: " << config.coarsening.threshold_node_weight
      << std::endl;
  oss << std::setw(28) << "  min. # hypernodes: " << config.coarsening.minimal_node_count
      << std::endl;
  oss << "2-Way-FM Refinement Parameters:" << std::endl;
  oss << std::setw(28) << "  max. # fruitless moves: " << config.two_way_fm.max_number_of_fruitless_moves;
  return oss.str();
}

} // namespace partition

#endif  // PARTITION_CONFIGURATION_H_
