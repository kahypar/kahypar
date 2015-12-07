/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "partition/Partitioner.h"

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/Configuration.h"
#include "tools/RandomFunctions.h"

#ifndef NDEBUG
#include "partition/Metrics.h"
#endif

namespace partition {
void Partitioner::performInitialPartitioning(Hypergraph& hg, const Configuration& config) {
  io::printHypergraphInfo(hg,
                          config.partition.coarse_graph_filename.substr(
                            config.partition.coarse_graph_filename.find_last_of("/")
                            + 1));
  if (config.partition.initial_partitioner == InitialPartitioner::hMetis ||
      config.partition.initial_partitioner == InitialPartitioner::PaToH) {
    initialPartitioningViaExternalTools(hg, config);
  } else if (config.partition.initial_partitioner == InitialPartitioner::KaHyPar) {
    initialPartitioningViaKaHyPar(hg, config);
  }
  Stats::instance().addToTotal(config, "InitialCut", metrics::hyperedgeCut(hg));
}
}  // namespace partition
