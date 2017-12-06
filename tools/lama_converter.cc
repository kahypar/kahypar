/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/macros.h"
#include "tools/mtx_to_hgr_conversion.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .mtx file specified" << std::endl;
  }
  std::string mtx_filename(argv[1]);
  std::string hgr_filename(mtx_filename + ".lama.hgr");
  std::cout << "Converting MTX matrix " << mtx_filename << " to HGR hypergraph format: "
            << hgr_filename << "..." << std::endl;
  mtxconversion::convertMtxToHgrForNonsymmetricParallelSPM(mtx_filename, hgr_filename);
  std::cout << " ... done!" << std::endl;
  return 0;
}
