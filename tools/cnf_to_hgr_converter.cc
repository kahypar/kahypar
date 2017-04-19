/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <boost/program_options.hpp>

#include <iostream>
#include <string>

#include "kahypar/macros.h"
#include "tools/cnf_to_hgr_conversion.h"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  using cnfconversion::HypergraphRepresentation;

  std::string cnf_filename;
  std::string hgr_filename;
  HypergraphRepresentation hypergraph_type;

  po::options_description options("Options");
  options.add_options()
    ("formula,f",
    po::value<std::string>(&cnf_filename)->value_name("<string>")->required(),
    "CNF formula filename")
    ("hypergraph,h",
    po::value<std::string>(&hgr_filename)->value_name("<string>"),
    "Hypergraph filename")
    ("representation,r",
    po::value<std::string>()->value_name("<string>")->required()->notifier(
      [&](const std::string& representation) {
    if (representation == "primal") {
      hypergraph_type = HypergraphRepresentation::Primal;
    } else if (representation == "dual") {
      hypergraph_type = HypergraphRepresentation::Dual;
    } else if (representation == "literal") {
      hypergraph_type = HypergraphRepresentation::Literal;
    } else {
      std::cerr << "Hypergraph representation invalid" << std::endl;
      std::exit(-1);
    }
  }),
    "Hypergraph Representation:\n"
    " - primal \n"
    " - dual \n"
    " - literal \n");

  po::variables_map cmd_vm;
  po::store(po::parse_command_line(argc, argv, options), cmd_vm);
  po::notify(cmd_vm);

  if (hgr_filename.empty()) {
    hgr_filename = cnf_filename + "." + toString(hypergraph_type) + ".hgr";
  } else {
    hgr_filename.insert(hgr_filename.find_last_of("."), "." + toString(hypergraph_type));
  }

  std::cout << "Converting SAT instance " << cnf_filename << " to HGR hypergraph format: "
            << hgr_filename << " ..." << std::endl;
  cnfconversion::convertInstance(cnf_filename, hgr_filename, hypergraph_type);
  std::cout << " ... done!" << std::endl;
  return 0;
}
