/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "tools/bookshelf_to_hgr_converter.h"

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "No .nets file specified" << std::endl;
    std::cout << "No .hgr file specified" << std::endl;
  }
  std::string nets_filename(argv[1]);
  std::string hgr_filename = std::string(argv[2]);

  std::cout << "Converting bookshelf instance " << nets_filename << " to HGR hypergraph format: "
            << hgr_filename << "..." << std::endl;
  convertBookshelfToHgr(nets_filename, hgr_filename);
  std::cout << " ... done!" << std::endl;
  return 0;
}
