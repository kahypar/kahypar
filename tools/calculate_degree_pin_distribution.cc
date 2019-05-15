/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/macros.h"

using namespace kahypar;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .hgr file specified" << std::endl;
    std::cout << "Usage: DegreePinDistribution <.hgr>" << std::endl;
    exit(0);
  }
  std::string hgr_filename(argv[1]);

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename, 2));

  std::map<HyperedgeID, size_t> degree_distribution;
  std::map<HypernodeID, size_t> pin_distribution;

  for (const auto hn : hypergraph.nodes()){
    ++degree_distribution[hypergraph.nodeDegree(hn)];
  }

  for (const auto he : hypergraph.edges()){
    ++pin_distribution[hypergraph.edgeSize(he)];
  }

  std::string output_filename(hgr_filename + ".degree_pin_distribution.csv");
  const std::string instance_name = hgr_filename.substr(hgr_filename.find_last_of('/') + 1);


  std::cout << "write csv file: " << output_filename << std::endl;

  std::ofstream out_stream(output_filename.c_str());
  out_stream << "hypergraph," << "type," << "size," << "count" << std::endl;

  for (const auto& degree_count : degree_distribution){
    out_stream << instance_name <<  "," << "degree," << degree_count.first << "," << degree_count.second << std::endl;
  }

  for (const auto& size_count : pin_distribution){
    out_stream << instance_name <<  "," << "size," << size_count.first << "," << size_count.second << std::endl;
  }

  out_stream.close();
  std::cout << " ... done!" << std::endl;
  return 0;
}
