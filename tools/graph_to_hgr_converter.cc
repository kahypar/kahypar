/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "kahypar/macros.h"

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
  ALWAYS_ASSERT(num_nodes > 0 && num_edges > 0, V(num_nodes) << V(num_edges));

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
