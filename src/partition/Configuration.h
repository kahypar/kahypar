#ifndef PARTITION_CONFIGURATION_H_
#define PARTITION_CONFIGURATION_H_

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace partition {
enum class StoppingRule { SIMPLE, ADAPTIVE };
enum class CoarseningScheme { HEAVY_EDGE_FULL, HEAVY_EDGE_HEURISTIC };

template <class Hypergraph>
struct Configuration {
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::PartitionID PartitionID;

  struct CoarseningParameters {
    CoarseningParameters() :
      threshold_node_weight(0),
      minimal_node_count(0),
      hypernode_weight_fraction(0.0),
      scheme(CoarseningScheme::HEAVY_EDGE_FULL) { }

    HypernodeWeight threshold_node_weight;
    HypernodeID minimal_node_count;
    double hypernode_weight_fraction;
    CoarseningScheme scheme;
  };

  struct PartitioningParameters {
    PartitioningParameters() :
      k(2),
      seed(0),
      initial_partitioning_attempts(1),
      epsilon(1.0),
      partition_size_upper_bound(std::numeric_limits<HypernodeWeight>::max()),
      verbose_output(false),
      graph_filename(),
      graph_partition_filename(),
      coarse_graph_filename(),
      coarse_graph_partition_filename() { }

    PartitionID k;
    int seed;
    int initial_partitioning_attempts;
    double epsilon;
    HypernodeWeight partition_size_upper_bound;
    bool verbose_output;
    std::string graph_filename;
    std::string graph_partition_filename;
    std::string coarse_graph_filename;
    std::string coarse_graph_partition_filename;
  };

  struct TwoWayFMParameters {
    TwoWayFMParameters() :
      max_number_of_fruitless_moves(50),
      alpha(4),
      beta(0.0),
      stopping_rule(StoppingRule::SIMPLE) { }

    int max_number_of_fruitless_moves;
    double alpha;
    double beta;
    StoppingRule stopping_rule;
  };

  PartitioningParameters partitioning;
  CoarseningParameters coarsening;
  TwoWayFMParameters two_way_fm;

  Configuration() :
    partitioning(),
    coarsening(),
    two_way_fm() { }
};

template <class Configuration>
std::string toString(const Configuration& config) {
  std::ostringstream oss;
  oss << std::left;
  oss << "Partitioning Parameters:" << std::endl;
  oss << std::setw(28) << "  Hypergraph: " << config.partitioning.graph_filename << std::endl;
  oss << std::setw(28) << "  Partition File: " << config.partitioning.graph_partition_filename
      << std::endl;
  oss << std::setw(28) << "  Coarsened Hypergraph: " << config.partitioning.coarse_graph_filename
      << std::endl;
  oss << std::setw(28) << "  Coarsened Partition File: "
      << config.partitioning.coarse_graph_partition_filename << std::endl;
  oss << std::setw(28) << "  k: " << config.partitioning.k << std::endl;
  oss << std::setw(28) << "  epsilon: " << config.partitioning.epsilon
      << std::endl;
  oss << std::setw(28) << "  L_max: " << config.partitioning.partition_size_upper_bound
      << std::endl;
  oss << std::setw(28) << "  seed: " << config.partitioning.seed << std::endl;
  oss << std::setw(28) << "  # initial partitionings: "
      << config.partitioning.initial_partitioning_attempts << std::endl;
  oss << "Coarsening Parameters:" << std::endl;
  oss << std::setw(28) << "  scheme: " <<
    (config.coarsening.scheme == CoarseningScheme::HEAVY_EDGE_FULL ? "heavy_full" : "heavy_heuristic")
      << std::endl;
  oss << std::setw(28) << "  hypernode weight fraction: "
      << config.coarsening.hypernode_weight_fraction << std::endl;
  oss << std::setw(28) << "  max. hypernode weight: " << config.coarsening.threshold_node_weight
      << std::endl;
  oss << std::setw(28) << "  min. # hypernodes: " << config.coarsening.minimal_node_count
      << std::endl;
  oss << "2-Way-FM Refinement Parameters:" << std::endl;
  oss << std::setw(28) << "  stopping rule: "
      << (config.two_way_fm.stopping_rule == StoppingRule::SIMPLE ? "simple" : "adaptive")
      << std::endl;
  oss << std::setw(28) << "  max. # fruitless moves: "
      << config.two_way_fm.max_number_of_fruitless_moves << std::endl;
  oss << std::setw(28) << "  random walk stop alpha: "
      << config.two_way_fm.alpha << std::endl;
  oss << std::setw(28) << "  random walk stop beta : "
      << config.two_way_fm.beta;
  return oss.str();
}
} // namespace partition

#endif  // PARTITION_CONFIGURATION_H_
