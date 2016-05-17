/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "lib/macros.h"
#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"

using defs::Hypergraph;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .hgr file specified" << std::endl;
  }
  std::string hgr_filename(argv[1]);
  std::string graphml_filename(hgr_filename + ".graphml");

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename,2));
  io::writeHypergraphToGraphMLFile(hypergraph, graphml_filename);

  std::cout << " ... done!" << std::endl;
  return 0;
}
