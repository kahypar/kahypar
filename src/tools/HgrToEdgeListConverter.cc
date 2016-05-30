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
  std::string graphml_filename(hgr_filename + ".edgelist");

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename,2));

  std::ofstream out_stream(graphml_filename.c_str());

  for (const defs::HypernodeID hn : hypergraph.nodes()){
    // vertex ids start with 0
    for (const defs::HyperedgeID he : hypergraph.incidentEdges(hn)) {
      const defs::HyperedgeID he_id = hypergraph.numNodes() + he;
      out_stream << hn << " " << he_id << std::endl;
    }
  }
  //backwards edges are 

  out_stream.close();
  std::cout << 'done' << std::endl;

}
