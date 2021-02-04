/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Nikolai Maas <nikolai.maas@kit.edu>
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
#include <regex>
#include <sstream>
#include <string>

#include "bookshelf_to_hgr_converter.h"
#include "kahypar/definitions.h"
#include "kahypar/macros.h"

using kahypar::HypernodeID;
using kahypar::HypernodeWeight;

void extractBookshelfWeights(
    const std::string &nodes_filename, const std::string &hgr_filename,
    std::unordered_map<std::string, HypernodeID> &node_to_hn) {
  std::ifstream input(nodes_filename);
  std::string line;
  std::vector<HypernodeWeight> weights;
  const HypernodeID included_nodes = node_to_hn.size();

  // get header line
  std::getline(input, line);
  if (line != "UCLA nodes 1.0") {
    std::cout << "File has wrong header" << std::endl;
    std::exit(-1);
  }

  // skip any comments
  std::getline(input, line);
  while (line[0] == '#') {
    std::getline(input, line);
  }

  // get number of nodes
  std::getline(input, line);
  ALWAYS_ASSERT(line.find("NumNodes") != std::string::npos, "Error");
  const HypernodeID num_nodes =
      std::stoi(line.substr(line.find_last_of(" ") + 1));
  if (num_nodes < included_nodes) {
    std::cout << "Inconsistent number of nodes in .nets file and .nodes file ("
              << included_nodes << ", " << num_nodes << ")" << std::endl;
    std::exit(-1);
  }
  weights.resize(included_nodes);

  // number of terminals
  std::getline(input, line);
  ALWAYS_ASSERT(line.find("NumTerminals") != std::string::npos, "Error");

  std::cout << V(num_nodes) << std::endl;
  std::cout << V(included_nodes) << std::endl;

  // skip empty line
  std::getline(input, line);

  const std::regex digit_regex(" ([[:digit:]]+) +([[:digit:]]+)");
  const std::regex pin_regex("(o|p)[[:digit:]]+");
  std::smatch match;
  HypernodeID detected_nodes = node_to_hn.size();

  while (std::getline(input, line)) {
    std::regex_search(line, match, pin_regex);
    const auto entry = node_to_hn.find(match.str());
    HypernodeID node;

    if (entry != node_to_hn.end()) {
      node = entry->second;
    } else {
      // isolated node that was not referenced in .nets file
      // currently, these are ignored (otherwise the node count at the start of
      // the file must be changed)
      ++detected_nodes;
      continue;
      // node = detected_nodes;
      // node_to_hn[match.str()] = node;
    }
    ALWAYS_ASSERT(node <= num_nodes, "Error");

    std::regex_search(line, match, digit_regex);
    ALWAYS_ASSERT(match.size() == 3, "Error");

    HypernodeWeight node_weight = 1;
    for (std::size_t i = 1; i < match.size(); ++i) {
      node_weight *= std::stoul(match[i]);
    }

    weights[node] = node_weight;
  }

  ALWAYS_ASSERT(num_nodes == detected_nodes, "Error");
  input.close();

  // append the weights to the end of the .hgr file
  std::ofstream output(hgr_filename, std::ios::app);

  for (const HypernodeWeight &w : weights) {
    output << w << std::endl;
  }

  output.close();
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cout << "File missing" << std::endl;
    std::cout << "Usage: BookshelfToWeightedHgr <.nets> <.nodes> <.hgr>"
              << std::endl;
    exit(0);
  }

  std::string nets_filename(argv[1]);
  std::string nodes_filename(argv[2]);
  std::string hgr_filename(argv[3]);

  std::cout << "Converting bookshelf instance " << nets_filename << " / "
            << nodes_filename
            << " to weighted HGR hypergraph format: " << hgr_filename << "..."
            << std::endl;
  auto nodes_to_hn = convertBookshelfToHgr(nets_filename, hgr_filename, true);
  extractBookshelfWeights(nodes_filename, hgr_filename, nodes_to_hn);
  std::cout << " ... done!" << std::endl;

  return 0;
}
