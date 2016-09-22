/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <iostream>
#include <string>

#include "kahypar/macros.h"
#include "tools/cnf_to_hgr_conversion.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .cnf file specified" << std::endl;
  }
  std::string mtx_filename(argv[1]);
  std::string hgr_filename(mtx_filename + ".hgr");
  std::cout << "Converting SAT instance " << mtx_filename << " to HGR hypergraph format: "
  << hgr_filename << "..." << std::endl;
  cnfconversion::convertInstance(mtx_filename, hgr_filename);
  std::cout << " ... done!" << std::endl;
  return 0;
}
