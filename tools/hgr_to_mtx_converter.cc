/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2020 Nikolai Maas <nikolai.maas@student.kit.edu>
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

#include <iostream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/macros.h"
#include "tools/hgr_to_mtx_conversion.h"

using namespace kahypar;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "No .hgr file specified" << std::endl;
    std::cout << "Usage: HgrToMtx <.hgr> <outfile>" << std::endl;
    exit(0);
  }

  const std::string hypergraph_filename(argv[1]);
  const std::string out_filename(argv[2]);
  LOG << "Converting hypergraph " << hypergraph_filename << "to mtx format:"
      << out_filename << "...";
  Hypergraph hypergraph(io::createHypergraphFromFile(hypergraph_filename, 2));
  kahypar::writeHypergraphInMatrixMarketFormat(hypergraph, out_filename);
  LOG << "... done!";
  return 0;
}
