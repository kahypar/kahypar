#ifndef LIB_IO_PARTITIONINGOUTPUT_H_
#define LIB_IO_PARTITIONINGOUTPUT_H_

#include <chrono>
#include <iostream>
#include <string>

#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "lib/GitRevision.h"
#include "partition/Metrics.h"
#include "lib/datastructure/Hypergraph.h"

using partition::CoarseningScheme;
using partition::StoppingRule;

namespace io {
inline void printHypergraphInfo(const HypergraphType& hypergraph, const std::string& name) {
  std::cout << "***********************Hypergraph Information************************" << std::endl;
  std::cout << "Name : " << name << std::endl;
  std::cout << "# HEs: " << hypergraph.numEdges() << "\t [avg HE size  : "
            << metrics::avgHyperedgeDegree(hypergraph) << "]" << std::endl;
  std::cout << "# HNs: " << hypergraph.numNodes() << "\t [avg HN degree: "
            << metrics::avgHypernodeDegree(hypergraph) << "]" << std::endl;
}

template <class Configuration>
inline void printPartitionerConfiguration(const Configuration& config) {
  std::cout << "*********************Partitioning Configuration**********************" << std::endl;
  std::cout << toString(config) << std::endl;
}

inline void printPartitioningResults(const HypergraphType& hypergraph,
                              const std::chrono::duration<double>& elapsed_seconds) {
  HypernodeWeight partition_weights[2] = { 0, 0 };
  metrics::partitionWeights(hypergraph, partition_weights);
  std::cout << "***********************2-way Partition Result************************" << std::endl;
  std::cout << "Hyperedge Cut   = " << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "Imbalance       = " << metrics::imbalance(hypergraph) << std::endl;
  std::cout << "| partition 0 | = " << partition_weights[0] << std::endl;
  std::cout << "| partition 1 | = " << partition_weights[1] << std::endl;
  std::cout << "partition time  = " << elapsed_seconds.count() << " s" << std::endl;
}
} // namespace io
#endif  // LIB_IO_PARTITIONINGOUTPUT_H_
