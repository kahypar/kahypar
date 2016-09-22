/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <iostream>
#include <string>

#include "kahypar/macros.h"
#include "tools/mtx_to_hgr_conversion.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .mtx file specified" << std::endl;
  }
  std::string mtx_filename(argv[1]);
  std::string hgr_filename(mtx_filename + ".hgr");
  std::cout << "Converting MTX matrix " << mtx_filename << " to HGR hypergraph format: "
  << hgr_filename << "..." << std::endl;
  mtxconversion::convertMtxToHgr(mtx_filename, hgr_filename);
  std::cout << " ... done!" << std::endl;
  return 0;
}
