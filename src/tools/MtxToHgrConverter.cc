/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <iostream>
#include <string>

#include "lib/macros.h"
#include "tools/MtxToHgrConversion.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .mtx file specified" << std::endl;
  }
  std::string mtx_filename(argv[1]);
  std::string hgr_filename(mtx_filename + ".hgr");
  std::cout << "Converting MTX matric " << mtx_filename << " to HGR hypergraph format: "
  << hgr_filename << "..." << std::endl;
  mtxconversion::convertMtxToHgr(mtx_filename, hgr_filename);
  std::cout << " ... done!" << std::endl;
  return 0;
}
