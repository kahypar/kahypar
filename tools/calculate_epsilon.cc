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

#include <fstream>
#include <iostream>
#include <map>
#include <queue>
#include <sstream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/macros.h"

using namespace kahypar;

using BinQueue = std::priority_queue<HypernodeWeight, std::vector<HypernodeWeight>,
                                     std::greater<HypernodeWeight>>;

HypernodeWeight maxPartByLPT(const Hypergraph& hypergraph, HypernodeID k) {
  // init priority queue of bins with zeros
  BinQueue bins{std::greater<HypernodeWeight>(),
                std::vector<HypernodeWeight>(k)};

  std::map<HypernodeWeight, size_t, std::greater<HypernodeID>>
      weight_distribution;
  for (const auto hn : hypergraph.nodes()) {
    ++weight_distribution[hypergraph.nodeWeight(hn)];
  }

  for (const std::pair<HypernodeWeight, size_t> &weight_entry :
       weight_distribution) {
    HypernodeWeight node_weight = weight_entry.first;
    size_t num_nodes = weight_entry.second;
    ASSERT(bins.size() == k);
    ASSERT(num_nodes > 0);

    if (node_weight == 0) {
      continue;
    }

    for (size_t i = 0; i < num_nodes; ++i) {
      // for every node, insert the node to the smallest bin and update the queue
      HypernodeWeight bin_weight = bins.top();
      bins.pop();
      bins.push(bin_weight + node_weight);
    }
  }

  // the result is the biggest bin, i.e. the last element in the queue
  while (bins.size() > 1) {
    bins.pop();
  }
  return bins.top();
}

int main(int argc, char *argv[]) {
  if (argc != 3 && argc != 4) {
    std::cout << "Wrong number of arguments" << std::endl;
    std::cout << "Usage: BalanceConstraint <.hgr> <k> [optional: <epsilon>]" << std::endl;
    exit(0);
  }

  std::string hgr_filename(argv[1]);
  HypernodeID k = std::stoul(argv[2]);

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename, k));

  HypernodeWeight weight_per_block = (hypergraph.totalWeight() + k - 1) / k;
  HypernodeWeight max_part = maxPartByLPT(hypergraph, k);
  double factor = static_cast<double>(max_part) / static_cast<double>(weight_per_block);

  std::cout << "max_part=" << max_part << std::endl;
  std::cout << "factor=" << factor << std::endl;

  if (argc == 4) {
    double base_epsilon = std::stod(argv[3]);
    double epsilon = (1.0 + base_epsilon) * factor - 1;
    std::cout << "epsilon=" << epsilon << std::endl;
  }

  return 0;
}
