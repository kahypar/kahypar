/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_IO_PARTITIONINGOUTPUT_H_
#define SRC_LIB_IO_PARTITIONINGOUTPUT_H_

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "lib/GitRevision.h"
#include "lib/definitions.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"

using defs::Hypergraph;
using utils::Stats;

namespace io {
inline void printHypergraphInfo(const Hypergraph& hypergraph, const std::string& name) {
  std::vector<HypernodeID> he_sizes;
  he_sizes.reserve(hypergraph.numEdges());
  for (auto he : hypergraph.edges()) {
    he_sizes.push_back(hypergraph.edgeSize(he));
  }
  std::sort(he_sizes.begin(), he_sizes.end());
  std::vector<HyperedgeID> hn_degrees;
  hn_degrees.reserve(hypergraph.numNodes());
  for (auto hn : hypergraph.nodes()) {
    hn_degrees.push_back(hypergraph.nodeDegree(hn));
  }
  std::sort(hn_degrees.begin(), hn_degrees.end());

  std::cout << "***********************Hypergraph Information************************" << std::endl;
  std::cout << "Name : " << name << std::endl;
  std::cout << "Type: " << hypergraph.typeAsString() << std::endl;
  std::cout << "# HEs: " << hypergraph.numEdges()
  << "\t HE size:   [min:" << std::setw(10) << std::left
  << (he_sizes.empty() ? 0 : he_sizes[0])
  << "avg:" << std::setw(10) << std::left << metrics::avgHyperedgeDegree(hypergraph)
  << "max:" << std::setw(10) << std::left
  << (he_sizes.empty() ? 0 : he_sizes[he_sizes.size() - 1])
  << "]" << std::endl;
  std::cout << "# HNs: " << hypergraph.numNodes()
  << "\t HN degree: [min:" << std::setw(10) << std::left
  << (hn_degrees.empty() ? 0 : hn_degrees[0])
  << "avg:" << std::setw(10) << std::left << metrics::avgHypernodeDegree(hypergraph)
  << "max:" << std::setw(10) << std::left
  << (hn_degrees.empty() ? 0 : hn_degrees[hn_degrees.size() - 1])
  << "]" << std::endl;
}

template <class Configuration>
inline void printPartitionerConfiguration(const Configuration& config) {
  std::cout << "*********************Partitioning Configuration**********************" << std::endl;
  std::cout << toString(config) << std::endl;
}

inline void printPartitioningResults(const Hypergraph& hypergraph,
                                     const std::chrono::duration<double>& elapsed_seconds,
                                     const std::array<std::chrono::duration<double>, 7>& timings) {
  std::cout << "***********************" << hypergraph.k()
  << "-way Partition Result************************" << std::endl;
  std::cout << "Hyperedge Cut  (minimize) = " << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "SOED           (minimize) = " << metrics::soed(hypergraph) << std::endl;
  std::cout << "(k-1)          (minimize) = " << metrics::kMinus1(hypergraph) << std::endl;
  std::cout << "Absorption     (maximize) = " << metrics::absorption(hypergraph) << std::endl;
  std::cout << "Imbalance                 = " << metrics::imbalance(hypergraph, hypergraph.k())
  << std::endl;
  std::cout << "partition time            = " << elapsed_seconds.count() << " s" << std::endl;
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    std::cout << "|part" << i << "| = " << std::setw(10) << std::left << hypergraph.partSize(i)
    << " w(" << i << ") = " << hypergraph.partWeight(i) << std::endl;
  }

  std::cout << "     | initial parallel HE removal time  = " << timings[0].count() << " s" << std::endl;
  std::cout << "     | initial large HE removal time     = " << timings[1].count() << " s" << std::endl;
  std::cout << "     | coarsening time                   = " << timings[2].count() << " s" << std::endl;
  std::cout << "     | initial partition time            = " << timings[3].count() << " s" << std::endl;
  std::cout << "     | uncoarsening/refinement time      = " << timings[4].count() << " s" << std::endl;
  std::cout << "     | initial large HE restore time     = " << timings[5].count() << " s" << std::endl;
  std::cout << "     | initial parallel HE restore time  = " << timings[6].count() << " s" << std::endl;
}

inline void printPartitioningStatistics() {
  std::cout << "*****************************Statistics******************************" << std::endl;
  std::cout << "numRemovedParalellHEs: Number of HEs that were removed because they were parallel to some other HE." << std::endl;
  std::cout << "removedSingleNodeHEWeight: Total weight of HEs that were removed because they contained only 1 HN.\n"
  << "This sum includes the weight of previously removed parallel HEs, because we sum over the edge weights" << std::endl;
  std::cout << Stats::instance().toConsoleString();
}

inline void printConnectivityStats(const std::vector<PartitionID>& connectivity_stats) {
  std::cout << "*************************Connectivity Values*************************" << std::endl;
  for (size_t i = 0; i < connectivity_stats.size(); ++i) {
    std::cout << "# HEs with λ=" << i << ": " << connectivity_stats[i] << std::endl;
  }
}
}  // namespace io
#endif  // SRC_LIB_IO_PARTITIONINGOUTPUT_H_
