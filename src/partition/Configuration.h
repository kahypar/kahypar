#ifndef PARTITION_CONFIGURATION_H_
#define PARTITION_CONFIGURATION_H_

#include <string>

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
    double balance_constraint;
    std::string graph_filename;
    std::string coarse_graph_filename;
    std::string partition_filename;
    PartitioningParameters() :
        k(2),
        seed(0),
        balance_constraint(1),
        graph_filename(),
        coarse_graph_filename(),
        partition_filename() {}
  };

  struct TwoWayFMParameters {
    int max_number_of_fruitless_moves;
    TwoWayFMParameters() :
        max_number_of_fruitless_moves(0) {}
  };

  PartitioningParameters partitioning;
  CoarseningParameters coarsening;
  TwoWayFMParameters two_way_fm;
  
  Configuration() :
      partitioning(),
      coarsening(),
      two_way_fm() {}
};

} // namespace partition

#endif  // PARTITION_CONFIGURATION_H_
