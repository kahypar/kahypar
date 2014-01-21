#ifndef LIB_IO_PARTITIONINGOUTPUT_H_
#define LIB_IO_PARTITIONINGOUTPUT_H_

#include <iostream>
#include <string>

#include "../../partition/Configuration.h"
#include "../../partition/Metrics.h"

namespace io {

template <class Hypergraph>
void printHypergraphInfo(const Hypergraph& hypergraph, const std::string& name) {
  std::cout << "***********************Hypergraph Information************************" << std::endl;
  std::cout << "Name : " << name << std::endl;
  std::cout << "# HEs: " << hypergraph.numEdges() << "\t [avg HE size  : "
            << metrics::avgHyperedgeDegree(hypergraph) << "]" << std::endl;
  std::cout << "# HNs: " << hypergraph.numNodes() << "\t [avg HN degree: "
            << metrics::avgHypernodeDegree(hypergraph) << "]" << std::endl;
}

template <class Configuration>
void printPartitionerConfiguration(const Configuration& config) {
  std::cout << "*********************Partitioning Configuration**********************" << std::endl;
  std::cout << toString(config) << std::endl;
}

template <class Hypergraph>
void printPartitioningResults(const Hypergraph& hypergraph) {
  int partition_size[2] = {0, 0};
  forall_hypernodes(hn, hypergraph) {
    ++partition_size[hypergraph.partitionIndex(*hn)];
  } endfor
  std::cout << "***********************2-way Partition Result************************" << std::endl;
  std::cout << "Hyperedge Cut   = " << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "Imbalance       = " << metrics::imbalance(hypergraph) << std::endl;
  std::cout << "| partition 0 | =" << partition_size[0] << std::endl;
  std::cout << "| partition 1 | =" << partition_size[1] << std::endl;
}

}
#endif  // LIB_IO_PARTITIONINGOUTPUT_H_
