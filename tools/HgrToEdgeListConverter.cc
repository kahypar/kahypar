/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "lib/io/HypergraphIO.h"
#include "lib/macros.h"
#include "HgrToEdgeListConversion.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .hgr file specified" << std::endl;
  }
  std::string hgr_filename(argv[1]);
  std::string graphml_filename(hgr_filename + ".edgelist");

  EdgeVector edges = createEdgeVector(io::createHypergraphFromFile(hgr_filename, 2));

  std::ofstream out_stream(graphml_filename.c_str());
  for (const Edge edge : edges) {
    out_stream << edge.src << " " << edge.dest << std::endl;
  }

  out_stream.close();
  std::cout << "done" << std::endl;
}
