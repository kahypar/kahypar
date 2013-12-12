#ifndef PARTITION_CONFIGURATION_H_
#define PARTITION_CONFIGURATION_H_

#include <string>

namespace partition {

template <class Hypergraph>
struct Configuration{
  typename Hypergraph::HypernodeWeight threshold_node_weight;
  typename Hypergraph::HypernodeID coarsening_limit;
  int balance_constraint;
  
  std::string graph_filename;
  std::string coarse_graph_filename;
  std::string partition_filename;
  
  Configuration() :
      threshold_node_weight(0),
      coarsening_limit(0),
      balance_constraint(0),
      graph_filename(),
      coarse_graph_filename(),
      partition_filename() {}
};

} // namespace partition

#endif  // PARTITION_CONFIGURATION_H_
