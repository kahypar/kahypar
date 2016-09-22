/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/io/partitioning_output.h"
#include "kahypar/macros.h"
#include "kahypar/partition/configuration.h"
#include "kahypar/partition/metrics.h"

static inline double imb(const Hypergraph& hypergraph, const PartitionID k) {
  HypernodeWeight max_weight = hypergraph.partWeight(0);
  for (PartitionID i = 1; i != k; ++i) {
    max_weight = std::max(max_weight, hypergraph.partWeight(i));
  }
  return static_cast<double>(max_weight) /
         ceil(static_cast<double>(hypergraph.totalWeight()) / k) - 1.0;
}

int main(int argc, char* argv[]) {
  if (argc != 2 && argc != 3) {
    std::cout << "No .hgr file specified" << std::endl;
    std::cout << "Usage: VerifyPartition <.hgr>  <partition file>" << std::endl;
    exit(0);
  }
  std::string hgr_filename(argv[1]);

  PartitionID max_part = 1;
  std::vector<PartitionID> partition;
  if (argc == 3) {
    std::string partition_filename(argv[2]);
    std::cout << "Reading partition file: " << partition_filename << std::endl;
    kahypar::io::readPartitionFile(partition_filename, partition);
    for (size_t index = 0; index < partition.size(); ++index) {
      max_part = std::max(max_part, partition[index]);
    }
  }

  Hypergraph hypergraph(kahypar::io::createHypergraphFromFile(hgr_filename, max_part + 1));

  if (partition.size() != 0 && partition.size() != hypergraph.initialNumNodes()) {
    std::cout << "partition file has incorrect size. Exiting." << std::endl;
    exit(-1);
  }

  for (size_t index = 0; index < partition.size(); ++index) {
    hypergraph.setNodePart(index, partition[index]);
  }

  kahypar::Configuration config;
  config.partition.k = max_part + 1;

  std::cout << "***********************" << hypergraph.k()
  << "-way Partition Result************************" << std::endl;
  std::cout << "cut=" << kahypar::metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "soed=" << kahypar::metrics::soed(hypergraph) << std::endl;
  std::cout << "km1= " << kahypar::metrics::km1(hypergraph) << std::endl;
  std::cout << "absorption= " << kahypar::metrics::absorption(hypergraph) << std::endl;
  std::cout << "imbalance= " << imb(hypergraph, config.partition.k)
  << std::endl;
  return 0;
}
