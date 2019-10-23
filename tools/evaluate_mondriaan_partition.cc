/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/io/partitioning_output.h"
#include "kahypar/macros.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"

using namespace kahypar;

static inline double imb(const Hypergraph& hypergraph, const PartitionID k) {
  HypernodeWeight max_weight = hypergraph.partWeight(0);
  for (PartitionID i = 1; i != k; ++i) {
    max_weight = std::max(max_weight, hypergraph.partWeight(i));
  }
  return static_cast<double>(max_weight) /
         ceil(static_cast<double>(hypergraph.totalWeight()) / k) - 1.0;
}

int main(int argc, char* argv[]) {
  if (argc != 2 && argc != 3) {
    std::cout << "No .hgr file specified" << std::endl;
    std::cout << "Usage: EvaluateMondriaanPartiton <.hgr>  <partition file>" << std::endl;
    exit(0);
  }
  const std::string hgr_filename(argv[1]);
  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename, 2));


  PartitionID max_part = 1;
  std::vector<PartitionID> partition(hypergraph.initialNumNodes(), -1);
  if (argc == 3) {
    const std::string partition_filename(argv[2]);
    std::cout << "Reading partition file: " << partition_filename << std::endl;
    std::ifstream file(partition_filename);
    if (file) {
      HypernodeID vertex = -1;
      PartitionID part = -1;
      // header
      file >> vertex >> max_part;
      ALWAYS_ASSERT(vertex == hypergraph.initialNumNodes());
      while (file >> vertex >> part) {
        // one-based
        partition[vertex - 1] = part - 1;
      }
      file.close();
    } else {
      std::cerr << "Error: File not found: " << std::endl;
    }
    ALWAYS_ASSERT(std::none_of(std::begin(partition), std::end(partition),
                               [](PartitionID i) {
      return i == -1;
    }));
  }

  if (partition.size() != 0 && partition.size() != hypergraph.initialNumNodes()) {
    std::cout << "partition file has incorrect size. Exiting." << std::endl;
    exit(-1);
  }

  LOG << V(max_part);
  hypergraph.changeK(max_part);

  for (size_t index = 0; index < partition.size(); ++index) {
    hypergraph.setNodePart(index, partition[index]);
  }

  Context context;
  context.partition.k = max_part;

  for (PartitionID i = 0; i < context.partition.k; ++i) {
    LOG << i << V(hypergraph.partSize(i)) << V(hypergraph.partWeight(i));
  }

  std::cout << "***********************" << hypergraph.k()
            << "-way Partition Result************************" << std::endl;
  std::cout << "cut=" << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "soed=" << metrics::soed(hypergraph) << std::endl;
  std::cout << "km1= " << metrics::km1(hypergraph) << std::endl;
  std::cout << "absorption= " << metrics::absorption(hypergraph) << std::endl;
  std::cout << "imbalance= " << imb(hypergraph, context.partition.k)
            << std::endl;

  std::cout << "RESULT"
            << " graph=" << hgr_filename.substr(hgr_filename.find_last_of('/') + 1)
            << " k=" << context.partition.k
            << " imbalance=" << imb(hypergraph, context.partition.k)
            << " cut=" << metrics::hyperedgeCut(hypergraph)
            << " soed=" << metrics::soed(hypergraph)
            << " km1=" << metrics::km1(hypergraph) << std::endl;

  return 0;
}
