#ifndef LIB_IO_PARTITIONINGOUTPUT_H_
#define LIB_IO_PARTITIONINGOUTPUT_H_

#include <chrono>
#include <iostream>
#include <string>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "lib/GitRevision.h"
#include "partition/Metrics.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/IRefiner.h"

using defs::Hypergraph;
using partition::IRefiner;
using partition::ICoarsener;

namespace io {
inline void printHypergraphInfo(const Hypergraph& hypergraph, const std::string& name) {
  std::cout << "***********************Hypergraph Information************************" << std::endl;
  std::cout << "Name : " << name << std::endl;
  std::cout << "# HEs: " << hypergraph.numEdges() << "\t [avg HE size: "
            << metrics::avgHyperedgeDegree(hypergraph) << ", 90th percentile:"
            << metrics::hyperedgeSizePercentile(hypergraph,90) << ", 95th percentile:"
            << metrics::hyperedgeSizePercentile(hypergraph,95) << "]" << std::endl;
  std::cout << "# HNs: " << hypergraph.numNodes() << "\t [avg HN degree: "
            << metrics::avgHypernodeDegree(hypergraph) << ", 90th percentile:"
            << metrics::hypernodeDegreePercentile(hypergraph,90) << ", 95th percentile:"
            << metrics::hypernodeDegreePercentile(hypergraph,95) << "]" << std::endl;
}

template <class Configuration>
inline void printPartitionerConfiguration(const Configuration& config) {
  std::cout << "*********************Partitioning Configuration**********************" << std::endl;
  std::cout << toString(config) << std::endl;
}

inline void printPartitioningResults(const Hypergraph& hypergraph,
                                     const std::chrono::duration<double>& elapsed_seconds,
                                     const std::array<std::chrono::duration<double>,3>& timings) {
  std::cout << "***********************" << hypergraph.k()
            << "-way Partition Result************************" << std::endl;
  std::cout << "Hyperedge Cut   = " << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "Imbalance       = " << metrics::imbalance(hypergraph) << std::endl;
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    std::cout << "| part" << i << " | = " << hypergraph.partWeight(i) << std::endl;
  }
  std::cout << "partition time  = " << elapsed_seconds.count() << " s" << std::endl;
  std::cout << "     | coarsening time               = " << timings[0].count() << " s" << std::endl;
  std::cout << "     | initial partition time        = " << timings[1].count() << " s" << std::endl;
  std::cout << "     | uncoarsening/refinement time  = " << timings[2].count() << " s" << std::endl;
}

inline void printPartitioningStatistics(const ICoarsener& coarsener, const IRefiner& refiner) {
  std::cout << "*****************************Statistics******************************" << std::endl;
  std::cout << coarsener.stats().toConsoleString();
  std::cout << refiner.stats().toConsoleString();
}

inline void printConnectivityStats(const std::vector<PartitionID>& connectivity_stats) {
  std::cout << "*************************Connectivity Values*************************" << std::endl;
  for (size_t i = 0; i < connectivity_stats.size(); ++i) {
    std::cout << "# HEs with λ=" << i << ": " << connectivity_stats[i] << std::endl;
  }
}

} // namespace io
#endif  // LIB_IO_PARTITIONINGOUTPUT_H_
