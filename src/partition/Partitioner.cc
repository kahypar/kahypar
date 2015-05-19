/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "partition/Partitioner.h"

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/Configuration.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "tools/RandomFunctions.h"

#ifndef NDEBUG
#include "partition/Metrics.h"
#endif

using defs::HighResClockTimepoint;

namespace partition {
void Partitioner::performDirectKwayPartitioning(Hypergraph& hypergraph, ICoarsener& coarsener,
                                                IRefiner& refiner) {
  if (_config.partition.initial_parallel_he_removal) {
    removeParallelHyperedges(hypergraph);
  }

  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  std::vector<HyperedgeID> removed_hyperedges;
  removeLargeHyperedges(hypergraph, removed_hyperedges);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  _timings[kInitialLargeHEremoval] = end - start;

#ifndef NDEBUG
  HyperedgeWeight initial_cut = std::numeric_limits<HyperedgeWeight>::max();
#endif

  partition(hypergraph, coarsener, refiner);
  DBG(dbg_partition_vcycles, "PartitioningResult: cut=" << metrics::hyperedgeCut(hypergraph));

  for (int vcycle = 0; vcycle < _config.partition.global_search_iterations; ++vcycle) {
    const bool found_improved_cut = partitionVCycle(hypergraph, coarsener, refiner);

    DBG(dbg_partition_vcycles, "vcycle # " << vcycle << ": cut=" << metrics::hyperedgeCut(hypergraph));
    if (!found_improved_cut) {
      LOG("Cut could not be decreased in v-cycle " << vcycle << ". Stopping global search.");
      break;
    }

    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut, "Uncoarsening worsened cut:"
           << metrics::hyperedgeCut(hypergraph) << ">" << initial_cut);
    ++_config.partition.current_v_cycle;
#ifndef NDEBUG
    initial_cut = metrics::hyperedgeCut(hypergraph);
#endif
  }

  start = std::chrono::high_resolution_clock::now();
  restoreLargeHyperedges(hypergraph, removed_hyperedges);
  end = std::chrono::high_resolution_clock::now();
  _timings[kInitialLargeHErestore] = end - start;

  if (_config.partition.initial_parallel_he_removal) {
    restoreParallelHyperedges(hypergraph);
  }
}


void Partitioner::performInitialPartitioning(Hypergraph& hg) {
  io::printHypergraphInfo(hg, _config.partition.coarse_graph_filename.substr(
                            _config.partition.coarse_graph_filename.find_last_of("/") + 1));

  DBG(dbg_partition_initial_partitioning, "# unconnected hypernodes = "
      << std::to_string([&]() {
      HypernodeID count = 0;
      for (const HypernodeID hn : hg.nodes()) {
        if (hg.nodeDegree(hn) == 0) {
          ++count;
        }
      }
      return count;
    } ()));

  HmetisToCoarsenedMapping hmetis_to_hg(hg.numNodes(), 0);
  CoarsenedToHmetisMapping hg_to_hmetis;
  createMappingsForInitialPartitioning(hmetis_to_hg, hg_to_hmetis, hg);

  switch (_config.partition.initial_partitioner) {
    case InitialPartitioner::hMetis:
      io::writeHypergraphForhMetisPartitioning(hg, _config.partition.coarse_graph_filename,
                                               hg_to_hmetis);
      break;
    case InitialPartitioner::PaToH:
      io::writeHypergraphForPaToHPartitioning(hg, _config.partition.coarse_graph_filename,
                                              hg_to_hmetis);
      break;
  }

  std::vector<PartitionID> partitioning;
  std::vector<PartitionID> best_partitioning;
  partitioning.reserve(hg.numNodes());
  best_partitioning.reserve(hg.numNodes());

  HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
  HyperedgeWeight current_cut = std::numeric_limits<HyperedgeWeight>::max();

  std::uniform_int_distribution<int> int_dist;
  std::mt19937 generator(_config.partition.seed);

  for (int attempt = 0; attempt < _config.partition.initial_partitioning_attempts; ++attempt) {
    int seed = int_dist(generator);
    std::string initial_partitioner_call;
    switch (_config.partition.initial_partitioner) {
      case InitialPartitioner::hMetis:
        initial_partitioner_call = _config.partition.initial_partitioner_path + " "
                                   + _config.partition.coarse_graph_filename
                                   + " " + std::to_string(_config.partition.k)
                                   + " -seed=" + std::to_string(seed)
                                   + " -ufactor=" + std::to_string(_config.partition.hmetis_ub_factor)
                                   + (_config.partition.verbose_output ? "" : " > /dev/null");
        break;
      case InitialPartitioner::PaToH:
        initial_partitioner_call = _config.partition.initial_partitioner_path + " "
                                   + _config.partition.coarse_graph_filename
                                   + " " + std::to_string(_config.partition.k)
                                   + " SD=" + std::to_string(seed)
                                   + " FI=" + std::to_string(_config.partition.epsilon)
                                   + " PQ=Q"  // quality preset
                                   + " UM=U"  // net-cut metric
                                   + " WI=1"  // write partition info
                                   + " BO=C"  // balance on cell weights
                                   + (_config.partition.verbose_output ? " OD=2" : " > /dev/null");
        break;
    }

    LOG(initial_partitioner_call);
    std::system(initial_partitioner_call.c_str());

    io::readPartitionFile(_config.partition.coarse_graph_partition_filename, partitioning);
    ASSERT(partitioning.size() == hg.numNodes(), "Partition file has incorrect size");

    current_cut = metrics::hyperedgeCut(hg, hg_to_hmetis, partitioning);
    DBG(dbg_partition_initial_partitioning, "attempt " << attempt << " seed("
        << seed << "):" << current_cut << " - balance=" << metrics::imbalance(hg,
                                                                              hg_to_hmetis,
                                                                              partitioning));
    _stats.add("initialCut_" + std::to_string(attempt), 0, current_cut);

    if (current_cut < best_cut) {
      DBG(dbg_partition_initial_partitioning, "Attempt " << attempt
          << " improved initial cut from " << best_cut << " to " << current_cut);
      best_partitioning.swap(partitioning);
      best_cut = current_cut;
    }
    partitioning.clear();
  }

  ASSERT(best_cut != std::numeric_limits<HyperedgeWeight>::max(), "No min cut calculated");
  for (size_t i = 0; i < best_partitioning.size(); ++i) {
    hg.setNodePart(hmetis_to_hg[i], best_partitioning[i]);
  }
  hg.sortConnectivitySets();
  ASSERT(metrics::hyperedgeCut(hg) == best_cut, "Cut induced by hypergraph does not equal "
         << "best initial cut");
}
}  // namespace partition
