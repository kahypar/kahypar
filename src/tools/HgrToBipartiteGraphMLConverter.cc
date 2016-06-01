/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/macros.h"

using defs::Hypergraph;

int main(int argc, char* argv[]) {
  if (argc != 2 && argc != 3) {
    std::cout << "No .hgr file specified" << std::endl;
    std::cout << "Usage: HgrToBipartiteGraphMLConverter <.hgr> [optional: <partition file>]" << std::endl;
    exit(0);
  }
  std::string hgr_filename(argv[1]);

  PartitionID max_part = 1;
  std::vector<defs::PartitionID> partition;
  if (argc == 3) {
    std::string partition_filename(argv[2]);
    std::cout << "Reading partition file: " << partition_filename << std::endl;
    io::readPartitionFile(partition_filename, partition);
    for (size_t index = 0; index < partition.size(); ++index) {
      max_part = std::max(max_part, partition[index]);
    }
  }

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename, max_part + 1));

  if (partition.size() != 0 && partition.size() != hypergraph.numNodes()) {
    std::cout << "partition file has incorrect size. Exiting." << std::endl;
    exit(-1);
  }

  for (size_t index = 0; index < partition.size(); ++index) {
    hypergraph.setNodePart(index, partition[index]);
  }

  std::string graphml_filename(hgr_filename + ".k" + std::to_string(max_part + 1) + ".graphml");

  std::cout << "write graphml file: " << graphml_filename << std::endl;
  io::writeHypergraphToGraphMLFile(hypergraph, graphml_filename);

  std::cout << " ... done!" << std::endl;
  return 0;
}
