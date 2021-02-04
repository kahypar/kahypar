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

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/macros.h"

using namespace kahypar;

HypernodeID removeHeavyNodes(Hypergraph& hypergraph, HypernodeID k, double epsilon) {
  HypernodeWeight total_weight = hypergraph.totalWeight();
  HypernodeID new_k = k;
  std::vector<HypernodeID> to_remove;

  do {
    HypernodeWeight allowed_block_weight = std::floor((1 + epsilon)
                                           * ceil(static_cast<double>(total_weight) / new_k));
    to_remove.clear();
    for (const HypernodeID& hn : hypergraph.nodes()) {
      if (hypergraph.nodeWeight(hn) > allowed_block_weight) {
        to_remove.push_back(hn);
      }
    }
    for (const HypernodeID& hn : to_remove) {
      total_weight -= hypergraph.nodeWeight(hn);
      hypergraph.removeNode(hn);
      --new_k;
    }
  } while (!to_remove.empty());

  return new_k;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    std::cout << "Wrong number of arguments" << std::endl;
    std::cout << "Usage: RemoveHeavyNodes <.hgr> <output_file> <k> <e>" << std::endl;
    exit(0);
  }

  std::string hgr_filename(argv[1]);
  std::string output_filename(argv[2]);
  HypernodeID k = std::stoul(argv[3]);
  double epsilon = std::stod(argv[4]);

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename, k));

  HypernodeID new_k = removeHeavyNodes(hypergraph, k, epsilon);
  for (const HyperedgeID& he : hypergraph.edges()) {
    if (hypergraph.edgeSize(he) <= 1) {
      hypergraph.removeEdge(he);
    }
  }

  auto modified_hg = ds::reindex(hypergraph);
  std::cout << "k=" << new_k << std::endl;
  std::cout << "removed=" << (k - new_k) << std::endl;

  // change hypergraph type to avoid writing edge weights unnecessarily
  bool hasEdgeWeights = false;
  for (const HyperedgeID& edge : hypergraph.edges()) {
    hasEdgeWeights |= hypergraph.edgeWeight(edge) != 1;
  }
  if (!hasEdgeWeights) {
    modified_hg.first->setType(HypergraphType::NodeWeights);
  }

  io::writeHypergraphFile(*modified_hg.first, output_filename);

  return 0;
}
