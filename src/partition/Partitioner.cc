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
void Partitioner::partition(Hypergraph& hypergraph, ICoarsener& coarsener,
                            IRefiner& refiner) {
  HighResClockTimepoint start;
  HighResClockTimepoint end;

  HypergraphPruner hypergraph_pruner(hypergraph, _config, _stats);
  RemovedParallelHyperedgesStack parallel_he_stack;
  if (_config.partition.initial_parallel_he_removal) {
    removeParallelHyperedges(hypergraph, hypergraph_pruner, parallel_he_stack);
  }


  start = std::chrono::high_resolution_clock::now();
  std::vector<HyperedgeID> removed_hyperedges;
  removeLargeHyperedges(hypergraph, removed_hyperedges);
  end = std::chrono::high_resolution_clock::now();
  _timings[kInitialLargeHEremoval] = end - start;

#ifndef NDEBUG
  HyperedgeWeight initial_cut = std::numeric_limits<HyperedgeWeight>::max();
#endif

  for (int vcycle = 0; vcycle < _config.partition.global_search_iterations; ++vcycle) {
    start = std::chrono::high_resolution_clock::now();
    coarsener.coarsen(_config.coarsening.contraction_limit);
    end = std::chrono::high_resolution_clock::now();
    _timings[kCoarsening] += end - start;

    if (vcycle == 0) {
      start = std::chrono::high_resolution_clock::now();
      performInitialPartitioning(hypergraph);
      end = std::chrono::high_resolution_clock::now();
      _timings[kInitialPartitioning] = end - start;
    }

    start = std::chrono::high_resolution_clock::now();
    const bool found_improved_cut = coarsener.uncoarsen(refiner);
    end = std::chrono::high_resolution_clock::now();
    _timings[kUncoarseningRefinement] += end - start;

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
    restoreParallelHyperedges(hypergraph_pruner, parallel_he_stack);
  }
}

void Partitioner::removeParallelHyperedges(Hypergraph& hypergraph,
                                           HypergraphPruner& hypergraph_pruner,
                                           RemovedParallelHyperedgesStack& stack) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  for (const HypernodeID hn : hypergraph.nodes()) {
    stack.emplace(std::make_pair(0, 0));
    hypergraph_pruner.removeParallelHyperedges(hn, stack.top().first, stack.top().second);
  }
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  _timings[kInitialParallelHEremoval] = end - start;
  LOG("Initially removed parallel HEs:" << _stats.toString());
}

void Partitioner::restoreParallelHyperedges(HypergraphPruner& hypergraph_pruner,
                                            RemovedParallelHyperedgesStack& stack) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  while (!stack.empty()) {
    hypergraph_pruner.restoreParallelHyperedges(stack.top().first, stack.top().second);
    stack.pop();
  }
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  _timings[kInitialParallelHErestore] = end - start;
}

void Partitioner::removeLargeHyperedges(Hypergraph& hg,
                                        std::vector<HyperedgeID>& removed_hyperedges) {
  if (_config.partition.hyperedge_size_threshold != -1) {
    for (const HyperedgeID he : hg.edges()) {
      if (hg.edgeSize(he) > _config.partition.hyperedge_size_threshold) {
        DBG(dbg_partition_large_he_removal, "Hyperedge " << he << ": size ("
            << hg.edgeSize(he) << ")   exceeds threshold: "
            << _config.partition.hyperedge_size_threshold);
        removed_hyperedges.push_back(he);
        hg.removeEdge(he, true);
      }
    }
  }
  _stats.add("numInitiallyRemovedLargeHEs", _config.partition.current_v_cycle,
             removed_hyperedges.size());
  LOG("removed " << removed_hyperedges.size() << " HEs that had more than "
      << _config.partition.hyperedge_size_threshold << " pins");
}

void Partitioner::restoreLargeHyperedges(Hypergraph& hg,
                                         std::vector<HyperedgeID>& removed_hyperedges) {
  HyperedgeID conn_one_hes = 0;
  for (auto && edge = removed_hyperedges.rbegin(); edge != removed_hyperedges.rend(); ++edge) {
    DBG(dbg_partition_large_he_removal, " restore Hyperedge " << *edge);
    hg.restoreEdge(*edge);
    if (hg.connectivity(*edge) == 1) {
      ++conn_one_hes;
    }
    partitionUnpartitionedPins(*edge, hg);
  }
  LOG(conn_one_hes << " could have been assigned to one part");
  ASSERT(metrics::imbalance(hg) <= _config.partition.epsilon,
         "Final assignment of unpartitioned pins violated balance constraint");
}

void Partitioner::partitionUnpartitionedPins(HyperedgeID he, Hypergraph& hg) {
  const HypernodeID num_pins = hg.edgeSize(he);
  HypernodeID num_unpartitioned_hns = 0;
  HypernodeWeight unpartitioned_weight = 0;
  for (const HypernodeID pin : hg.pins(he)) {
    if (hg.partID(pin) == Hypergraph::kInvalidPartition) {
      ++num_unpartitioned_hns;
      unpartitioned_weight += hg.nodeWeight(pin);
    }
  }

  // First case: If HE does not contain any pins that are already partitioned,
  // we try to find a part that can store this HE as an internal HE.
  PartitionID target_part = Hypergraph::kInvalidPartition;
  HypernodeWeight target_part_weight_after_assignment = std::numeric_limits<HypernodeWeight>::max();
  if (num_unpartitioned_hns == num_pins) {
    ASSERT(hg.connectivity(he) == 0, "HE " << he << " does already contain partitioned pins");
    for (PartitionID part = 0; part < _config.partition.k; ++part) {
      const HypernodeWeight current_part_weight_after_assignment = hg.partWeight(part)
                                                                   + unpartitioned_weight;
      if (current_part_weight_after_assignment <= _config.partition.max_part_weight &&
          current_part_weight_after_assignment < target_part_weight_after_assignment) {
        target_part = part;
        target_part_weight_after_assignment = current_part_weight_after_assignment;
      }
    }
    DBG(dbg_partition_large_he_restore, "All pins unassigned: Assigned all pins of HE "
        << he << " to part " << target_part);
    assignAllPinsToPartition(he, target_part, hg);
    return;
  }

  // Second case: If the HE has a connectivity of 1, we try to internalize it
  // into that part.
  if (hg.connectivity(he) == 1) {
    const PartitionID connected_part = *hg.connectivitySet(he).begin();
    if (hg.partWeight(connected_part) + unpartitioned_weight
        <= _config.partition.max_part_weight) {
      assignUnpartitionedPinsToPartition(he, connected_part, hg);
      DBG(dbg_partition_large_he_restore, "Connectivity=1: Moved all unpartitioned pins of HE "
          << he << " to part " << connected_part);
      return;
    }
  }

  // Third case: Distribute the remaining pins such that imbalance is minimzed.
  // TODO(schlag): If we add global objective functions, this has to be adapted:
  // In this case, we would probably want to distibute the pins only into connected
  // parts.
  distributePinsAcrossPartitions(he, hg);
}

void Partitioner::assignUnpartitionedPinsToPartition(HyperedgeID he, PartitionID id, Hypergraph& hg) {
  DBG(dbg_partition_large_he_removal,
      "Assigning unpartitioned pins of HE " << he << " to partition " << id);
  for (const HypernodeID pin : hg.pins(he)) {
    ASSERT(hg.partID(pin) == Hypergraph::kInvalidPartition || hg.partID(pin) == id,
           "HN " << pin << " is not in partition " << id << " but in "
           << hg.partID(pin));
    if (hg.partID(pin) == Hypergraph::kInvalidPartition) {
      hg.setNodePart(pin, id);
    }
  }
}

void Partitioner::assignAllPinsToPartition(HyperedgeID he, PartitionID id, Hypergraph& hg) {
  DBG(dbg_partition_large_he_removal, "Assigning all pins of HE " << he << " to partition " << id);
  for (const HypernodeID pin : hg.pins(he)) {
    ASSERT(hg.partID(pin) == Hypergraph::kInvalidPartition,
           "HN " << pin << " is not in partition " << id << " but in "
           << hg.partID(pin));
    hg.setNodePart(pin, id);
  }
}

void Partitioner::distributePinsAcrossPartitions(HyperedgeID he, Hypergraph& hg) {
  DBG(dbg_partition_large_he_removal, "Distributing pins of HE " << he << " to both partitions");

  static auto comparePartWeights = [](const auto& a, const auto& b) {
                                     return a.weight < b.weight;
                                   };

  PartitionID min_partition = 0;

  for (const HypernodeID pin : hg.pins(he)) {
    if (hg.partID(pin) == Hypergraph::kInvalidPartition) {
      min_partition = std::min_element(hg.partInfos().begin(), hg.partInfos().end(),
                                       comparePartWeights)
                      - hg.partInfos().begin();
      hg.setNodePart(pin, min_partition);
    }
  }
}

void Partitioner::createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                                       CoarsenedToHmetisMapping& hg_to_hmetis,
                                                       const Hypergraph& hg) {
  int i = 0;
  for (const HypernodeID hn : hg.nodes()) {
    hg_to_hmetis[hn] = i;
    hmetis_to_hg[i] = hn;
    ++i;
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
    case InitialPartitioner::KaHyPar:
      io::writeHypergraphForhMetisPartitioning(hg, _config.partition.coarse_graph_filename,
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
                                   + " PQ=Q" // quality preset
                                   + " UM=U" // net-cut metric
                                   + " WI=1" // write partition info
                                   + " BO=C" // balance on cell weights
                                   + (_config.partition.verbose_output ? " OD=2" : " > /dev/null");
        break;
      case InitialPartitioner::KaHyPar:
        initial_partitioner_call = _config.partition.initial_partitioner_path + " --hgr="
                                   + _config.partition.coarse_graph_filename
                                   + " --k=" + std::to_string(_config.partition.k)
                                   + " --seed=" + std::to_string(seed)
                                   + " --epsilon=" + std::to_string(_config.partition.epsilon)
                                   + " --mode=random"
                                   + " --output=" + _config.partition.coarse_graph_partition_filename;
        break;
    }

    LOG(initial_partitioner_call);
    std::system(initial_partitioner_call.c_str());

    io::readPartitionFile(_config.partition.coarse_graph_partition_filename, partitioning);
    ASSERT(partitioning.size() == hg.numNodes(), "Partition file has incorrect size");

    current_cut = metrics::hyperedgeCut(hg, hg_to_hmetis, partitioning);
    DBG(dbg_partition_initial_partitioning, "attempt " << attempt << " seed("
        << seed << "):" << current_cut << " - balance=" << metrics::imbalance(hg, hg_to_hmetis, partitioning));
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
} // namespace partition
