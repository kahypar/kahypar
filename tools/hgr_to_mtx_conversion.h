/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#pragma once

#include <fstream>
#include <sstream>
#include <string>

#include "kahypar/definitions.h"

namespace kahypar {
/// see https://www.staff.science.uu.nl/~bisse101/Mondriaan/Docs/HYPERGRAPH.html
void writeHypergraphInMatrixMarketFormat(const Hypergraph& hypergraph,
                                         const std::string& out_filename) {
  std::ofstream out_stream(out_filename.c_str());
  out_stream << "%%MatrixMarket weightedmatrix coordinate pattern general" << std::endl;
  out_stream << hypergraph.initialNumEdges() << " "
             << hypergraph.initialNumNodes() << " "
             << hypergraph.initialNumPins() << " "
             << "2" << std::endl;
  for (const auto he : hypergraph.edges()) {
    for (const auto pin : hypergraph.pins(he)) {
      out_stream << (he + 1) << " " << (pin + 1) << std::endl;
    }
  }
  for (const auto hn : hypergraph.nodes()) {
    out_stream << hypergraph.nodeWeight(hn) << std::endl;
  }
  out_stream.close();
}
}  // namespace kahypar
