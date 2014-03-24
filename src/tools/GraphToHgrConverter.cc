/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "lib/macros.h"

void parseHeader(std::string& header_line, int& num_nodes, int& num_edges) {
  std::istringstream line_stream(header_line);
  line_stream >> num_nodes >> num_edges;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .mtx file specified" << std::endl;
  }
  std::string graph_filename(argv[1]);
  std::string hgr_filename(graph_filename + ".hgr");
  std::cout << "Converting graph " << graph_filename << " to HGR hypergraph format: "
  << hgr_filename << "..." << std::endl;

  std::string line;
  std::ifstream in_stream(graph_filename.c_str());
  std::getline(in_stream, line);

  int num_nodes = -1;
  int num_edges = -1;
  parseHeader(line, num_nodes, num_edges);
  ALWAYS_ASSERT(num_nodes > 0 && num_edges > 0);

  std::ofstream out_stream(hgr_filename.c_str());
  out_stream << num_edges << " " << num_nodes << std::endl;
  for (int i = 1; i <= num_nodes; ++i) {
    std::getline(in_stream, line);
    std::istringstream line_stream(line);
    int node;
    while (line_stream >> node) {
      if (node > i) {
        out_stream << i << " " << node << std::endl;
      }
    }
  }
  in_stream.close();
  out_stream.close();
  std::cout << " ... done!" << std::endl;
  return 0;
}
