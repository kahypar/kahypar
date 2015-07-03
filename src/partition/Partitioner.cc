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

using defs::HighResClockTimepoint;

namespace partition {
void Partitioner::performInitialPartitioning(Hypergraph& hg, const Configuration& config) {
  io::printHypergraphInfo(hg, config.partition.coarse_graph_filename.substr(
                            config.partition.coarse_graph_filename.find_last_of("/") + 1));

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

  switch (config.partition.initial_partitioner) {
    case InitialPartitioner::hMetis:
      io::writeHypergraphForhMetisPartitioning(hg, config.partition.coarse_graph_filename,
                                               hg_to_hmetis);
      break;
    case InitialPartitioner::PaToH:
      io::writeHypergraphForPaToHPartitioning(hg, config.partition.coarse_graph_filename,
                                              hg_to_hmetis);
      break;
    case InitialPartitioner::KaHyPar:
      io::writeHypergraphForhMetisPartitioning(hg, config.partition.coarse_graph_filename,
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
  std::mt19937 generator(config.partition.seed);

  for (int attempt = 0; attempt < config.partition.initial_partitioning_attempts; ++attempt) {
    int seed = int_dist(generator);
    std::string initial_partitioner_call;
    switch (config.partition.initial_partitioner) {
      case InitialPartitioner::hMetis:
        initial_partitioner_call = config.partition.initial_partitioner_path + " "
                                   + config.partition.coarse_graph_filename
                                   + " " + std::to_string(config.partition.k)
                                   + " -seed=" + std::to_string(seed)
                                   + " -ufactor=" + std::to_string(config.partition.hmetis_ub_factor < 0.1 ? 0.1 : config.partition.hmetis_ub_factor)
                                   + (config.partition.verbose_output ? "" : " > /dev/null");
        break;
      case InitialPartitioner::PaToH:
        initial_partitioner_call = config.partition.initial_partitioner_path + " "
                                   + config.partition.coarse_graph_filename
                                   + " " + std::to_string(config.partition.k)
                                   + " SD=" + std::to_string(seed)
                                   + " FI=" + std::to_string(config.partition.epsilon)
                                   + " PQ=Q"  // quality preset
                                   + " UM=U"  // net-cut metric
                                   + " WI=1"  // write partition info
                                   + " BO=C"  // balance on cell weights
                                   + (config.partition.verbose_output ? " OD=2" : " > /dev/null");
        break;
      case InitialPartitioner::KaHyPar:
        initial_partitioner_call = config.partition.initial_partitioner_path + " --hgr="
                                   + config.partition.coarse_graph_filename
                                   + " --k=" + std::to_string(config.initial_partitioning.k)
                                   + " --seed=" + std::to_string(seed)
                                   + " --epsilon=" + std::to_string(config.initial_partitioning.epsilon)
        						   + " --nruns=" + std::to_string(config.initial_partitioning.nruns)
        						   + " --refinement=" +std::to_string(config.initial_partitioning.refinement)
        						   + " --rollback=" + std::to_string(config.initial_partitioning.rollback)
        						   + " --algo=" + config.initial_partitioning.algorithm
								   + " --pool_type=" +config.initial_partitioning.pool_type
                                   + " --mode=direct"
								   + " --min_ils_iterations=" + std::to_string(config.initial_partitioning.min_ils_iterations)
        						   + " --max_stable_net_removals=" + std::to_string(config.initial_partitioning.max_stable_net_removals)
								   + " --unassigned-part=" + std::to_string(config.initial_partitioning.unassigned_part)
        						   + " --stats=false"
								   + " --rtype=" + partition::toString(config.partition.refinement_algorithm)
                                   + " --output=" + config.partition.coarse_graph_partition_filename;
        break;
    }

    LOG(initial_partitioner_call);
    LOGVAR(config.partition.hmetis_ub_factor);
    std::system(initial_partitioner_call.c_str());

    io::readPartitionFile(config.partition.coarse_graph_partition_filename, partitioning);
    ASSERT(partitioning.size() == hg.numNodes(), "Partition file has incorrect size");
    current_cut = metrics::hyperedgeCut(hg, hg_to_hmetis, partitioning);
    DBG(dbg_partition_initial_partitioning, "attempt " << attempt << " seed("
        << seed << "):" << current_cut << " - balance=" << metrics::imbalance(hg,
                                                                              hg_to_hmetis,
                                                                              partitioning,
                                                                              config.partition.k));
    Stats::instance().add("initialCut_" + std::to_string(attempt), 0, current_cut);

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
  ASSERT(metrics::hyperedgeCut(hg) == best_cut, "Cut induced by hypergraph does not equal "
         << "best initial cut");
}
}  // namespace partition
