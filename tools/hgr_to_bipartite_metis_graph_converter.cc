/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/macros.h"

using namespace kahypar;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .hgr file specified" << std::endl;
  }
  std::string hgr_filename(argv[1]);
  std::string graphml_filename(hgr_filename + ".graph");

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename, 2));

  std::ofstream out_stream(graphml_filename.c_str());

  // Vertices: hypernodes + hyperedges
  // Edges: One edge for each pin!

  out_stream << hypergraph.initialNumNodes() + hypergraph.initialNumEdges() << " " << hypergraph.initialNumPins() << std::endl;

  for (const HypernodeID& hn : hypergraph.nodes()) {
    // vertex ids start with 1
    for (const HyperedgeID& he : hypergraph.incidentEdges(hn)) {
      const HyperedgeID he_id = hypergraph.initialNumNodes() + he + 1;
      out_stream << he_id << " ";
    }
    out_stream << std::endl;
  }

  for (const HyperedgeID& he : hypergraph.edges()) {
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      const HypernodeID hn_id = pin + 1;
      out_stream << hn_id << " ";
    }
    out_stream << std::endl;
  }

  out_stream.close();
  std::cout << " ... done!" << std::endl;
  return 0;
}
