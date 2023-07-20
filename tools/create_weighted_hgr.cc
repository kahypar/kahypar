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

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"

using namespace kahypar;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "No .hgr file specified" << std::endl;
    std::cout << "Usage: unweighted_to_weighted_hgr <unweighted.hgr> <weighted.hgr>" << std::endl;
    exit(0);
  }

  std::string in_hgr_filename(argv[1]);
  std::string out_hgr_filename(argv[2]);

  Hypergraph hypergraph(io::createHypergraphFromFile(in_hgr_filename, 2, true, false));

  for (const HyperedgeID& he : hypergraph.edges()) {
    hypergraph.setEdgeWeight(he, hypergraph.edgeSize(he));
  }

  hypergraph.setType(Hypergraph::Type::EdgeWeights);

  io::writeHypergraphFile(hypergraph, out_hgr_filename);


  return 0;
}
