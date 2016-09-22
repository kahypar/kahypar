/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/macros.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "No .hgr file specified" << std::endl;
    std::cout << "Usage: HgrToPaToHConverter <.hgr> <outfile>" << std::endl;
    exit(0);
  }
  std::string hgr_filename(argv[1]);
  std::string out_filename(argv[2]);

  Hypergraph hypergraph(
    partition::io::createHypergraphFromFile(hgr_filename, 2));

  partition::io::writeHypergraphForPaToHPartitioning(hypergraph, out_filename);

  return 0;
}
