#ifndef PARTITION_CONFIGURATION_H_
#define PARTITION_CONFIGURATION_H_

namespace partition {

template <class Hypergraph>
struct Configuration{
  typename Hypergraph::HypernodeWeight threshold_node_weight;
  typename Hypergraph::HypernodeID coarsening_limit;
  Configuration(typename Hypergraph::HypernodeWeight threshold,
                typename Hypergraph::HypernodeID limit) :
      threshold_node_weight(threshold),
      coarsening_limit(limit) {}
};

} // namespace partition

#endif  // PARTITION_CONFIGURATION_H_
