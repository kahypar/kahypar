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

#include <iostream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/macros.h"
#include "tools/hgr_to_mtx_conversion.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No hypergraph file specified" << std::endl;
  }
  const std::string hypergraph_filename(argv[1]);
  const std::string mtx_filename(hypergraph_filename + ".mondriaan.mtx");
  LOG << "Converting hypergraph " << hypergraph_filename << "to mondriaan mtx format:"
      << mtx_filename << "...";
  kahypar::Hypergraph input_hypergraph = kahypar::io::createHypergraphFromFile(hypergraph_filename, 2);
  kahypar::writeHypergraphInMatrixMarketFormat(input_hypergraph, mtx_filename);
  LOG << "... done!";
  return 0;
}
