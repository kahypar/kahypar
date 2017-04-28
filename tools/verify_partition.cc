/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
    std::cout << "Usage: VerifyPartition <.hgr>  <partition file>" << std::endl;
    exit(0);
  }
  std::string hgr_filename(argv[1]);

  PartitionID max_part = 1;
  std::vector<PartitionID> partition;
  if (argc == 3) {
    std::string partition_filename(argv[2]);
    std::cout << "Reading partition file: " << partition_filename << std::endl;
    io::readPartitionFile(partition_filename, partition);
    for (size_t index = 0; index < partition.size(); ++index) {
      max_part = std::max(max_part, partition[index]);
    }
  }

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename, max_part + 1));

  if (partition.size() != 0 && partition.size() != hypergraph.initialNumNodes()) {
    std::cout << "partition file has incorrect size. Exiting." << std::endl;
    exit(-1);
  }

  for (size_t index = 0; index < partition.size(); ++index) {
    hypergraph.setNodePart(index, partition[index]);
  }

  Context context;
  context.partition.k = max_part + 1;

  std::cout << "***********************" << hypergraph.k()
            << "-way Partition Result************************" << std::endl;
  std::cout << "cut=" << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "soed=" << metrics::soed(hypergraph) << std::endl;
  std::cout << "km1= " << metrics::km1(hypergraph) << std::endl;
  std::cout << "absorption= " << metrics::absorption(hypergraph) << std::endl;
  std::cout << "imbalance= " << imb(hypergraph, context.partition.k)
            << std::endl;
  return 0;
}
