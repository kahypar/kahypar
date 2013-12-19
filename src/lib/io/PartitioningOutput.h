#ifndef LIB_IO_PARTITIONINGOUTPUT_H_
#define LIB_IO_PARTITIONINGOUTPUT_H_

#include <iostream>
#include <string>

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
  std::cout << "*********************************************************************" << std::endl;
}

}
#endif  // LIB_IO_PARTITIONINGOUTPUT_H_
