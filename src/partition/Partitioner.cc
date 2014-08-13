/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "partition/Partitioner.h"

#include "lib/io/HypergraphIO.h"
#include "lib/io/PartitioningOutput.h"
#include "partition/Configuration.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "tools/RandomFunctions.h"

#ifndef NDEBUG
#include "partition/Metrics.h"
#endif
namespace partition {
void Partitioner::partition(Hypergraph& hypergraph, ICoarsener& coarsener,
                            IRefiner& refiner) {
  std::vector<HyperedgeID> removed_hyperedges;
  removeLargeHyperedges(hypergraph, removed_hyperedges);

  for (int vcycle = 0; vcycle < _config.partitioning.global_search_iterations; ++vcycle) {
    coarsener.coarsen(_config.coarsening.minimal_node_count);

    if (vcycle == 0) {
      performInitialPartitioning(hypergraph);
    }

#ifndef NDEBUG
    HyperedgeWeight initial_cut = metrics::hyperedgeCut(hypergraph);
#endif
    coarsener.uncoarsen(refiner);
    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut, "Uncoarsening worsened cut");
    DBG(dbg_partition_vcycles, "vcycle # " << vcycle << ": cut=" << metrics::hyperedgeCut(hypergraph));
    ++_config.partitioning.current_v_cycle;
  }

  restoreLargeHyperedges(hypergraph, removed_hyperedges);
}

void Partitioner::removeLargeHyperedges(Hypergraph& hg, std::vector<HyperedgeID>& removed_hyperedges) {
  if (_config.partitioning.hyperedge_size_threshold != -1) {
    for (auto && he : hg.edges()) {
      if (hg.edgeSize(he) > _config.partitioning.hyperedge_size_threshold) {
        DBG(dbg_partition_large_he_removal, "Hyperedge " << he << ": size ("
            << hg.edgeSize(he) << ")   exceeds threshold: "
            << _config.partitioning.hyperedge_size_threshold);
        removed_hyperedges.push_back(he);
        hg.removeEdge(he, true);
      }
    }
  }
}

void Partitioner::restoreLargeHyperedges(Hypergraph& hg, std::vector<HyperedgeID>& removed_hyperedges) {
  if (_config.partitioning.hyperedge_size_threshold != -1) {
    PartitionWeights partition_weights { 0, 0 };
    for (auto && hn : hg.nodes()) {
      if (hg.partitionIndex(hn) != Hypergraph::kInvalidPartition) {
        partition_weights[hg.partitionIndex(hn)] += hg.nodeWeight(hn);
      }
    }

    for (auto && edge = removed_hyperedges.rbegin(); edge != removed_hyperedges.rend(); ++edge) {
      DBG(dbg_partition_large_he_removal, " restore Hyperedge " << *edge);
      hg.restoreEdge(*edge);
      partitionUnpartitionedPins(*edge, hg, partition_weights);
    }
    ASSERT(metrics::imbalance(hg) <= _config.partitioning.epsilon,
           "Final assignment of unpartitioned pins violated balance constraint");
  }
}

void Partitioner::partitionUnpartitionedPins(HyperedgeID he, Hypergraph& hg,
                                             PartitionWeights& partition_weights) {
  HypernodeID num_pins = hg.edgeSize(he);
  HypernodeID num_unpartitioned_hns = hg.pinCountInPartition(he, Hypergraph::kInvalidPartition);
  HypernodeWeight unpartitioned_weight = 0;
  for (auto && pin : hg.pins(he)) {
    if (hg.partitionIndex(pin) == Hypergraph::kInvalidPartition) {
      unpartitioned_weight += hg.nodeWeight(pin);
    }
  }

  if (num_unpartitioned_hns == num_pins) {
    if (partition_weights[0] + unpartitioned_weight
        <= _config.partitioning.partition_size_upper_bound) {
      assignAllPinsToPartition(he, 0, hg, partition_weights);
    } else if (partition_weights[1] + unpartitioned_weight
               <= _config.partitioning.partition_size_upper_bound) {
      assignAllPinsToPartition(he, 1, hg, partition_weights);
    }
    return;
  }
  if ((hg.pinCountInPartition(he, 0) > 0 && hg.pinCountInPartition(he, 1) == 0) &&
      (partition_weights[0] + unpartitioned_weight
       <= _config.partitioning.partition_size_upper_bound)) {
    assignUnpartitionedPinsToPartition(he, 0, hg, partition_weights);
    return;
  }
  if ((hg.pinCountInPartition(he, 1) > 0 && hg.pinCountInPartition(he, 0) == 0) &&
      (partition_weights[1] + unpartitioned_weight
       <= _config.partitioning.partition_size_upper_bound)) {
    assignUnpartitionedPinsToPartition(he, 1, hg, partition_weights);
    return;
  }
  distributePinsAcrossPartitions(he, hg, partition_weights);
}

void Partitioner::assignUnpartitionedPinsToPartition(HyperedgeID he, PartitionID id, Hypergraph& hg,
                                                     PartitionWeights& partition_weights) {
  DBG(dbg_partition_large_he_removal,
      "Assigning unpartitioned pins of HE " << he << " to partition " << id);
  for (auto && pin : hg.pins(he)) {
    ASSERT(hg.partitionIndex(pin) == Hypergraph::kInvalidPartition || hg.partitionIndex(pin) == id,
           "HN " << pin << " is not in partition " << id << " but in "
           << hg.partitionIndex(pin));
    if (hg.partitionIndex(pin) == Hypergraph::kInvalidPartition) {
      hg.changeNodePartition(pin, Hypergraph::kInvalidPartition, id);
      partition_weights[id] += hg.nodeWeight(pin);
    }
  }
}

void Partitioner::assignAllPinsToPartition(HyperedgeID he, PartitionID id, Hypergraph& hg, PartitionWeights&
                                           partition_weights) {
  DBG(dbg_partition_large_he_removal, "Assigning all pins of HE " << he << " to partition " << id);
  for (auto && pin : hg.pins(he)) {
    ASSERT(hg.partitionIndex(pin) == Hypergraph::kInvalidPartition,
           "HN " << pin << " is not in partition " << id << " but in "
           << hg.partitionIndex(pin));
    hg.changeNodePartition(pin, Hypergraph::kInvalidPartition, id);
    partition_weights[id] += hg.nodeWeight(pin);
  }
}

void Partitioner::distributePinsAcrossPartitions(HyperedgeID he, Hypergraph& hg,
                                                 PartitionWeights& partition_weights) {
  DBG(dbg_partition_large_he_removal, "Distributing pins of HE " << he << " to both partitions");
  size_t min_partition = 0;
  for (auto && pin : hg.pins(he)) {
    if (hg.partitionIndex(pin) == Hypergraph::kInvalidPartition) {
      min_partition = std::min_element(partition_weights.begin(), partition_weights.end())
                      - partition_weights.begin();
      hg.changeNodePartition(pin, Hypergraph::kInvalidPartition, min_partition);
      partition_weights[min_partition] += hg.nodeWeight(pin);
    }
  }
}

void Partitioner::createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                                       CoarsenedToHmetisMapping& hg_to_hmetis,
                                                       const Hypergraph& hg) {
  int i = 0;
  for (auto && hn : hg.nodes()) {
    hg_to_hmetis[hn] = i;
    hmetis_to_hg[i] = hn;
    ++i;
  }
}

void Partitioner::performInitialPartitioning(Hypergraph& hg) {
  io::printHypergraphInfo(hg, _config.partitioning.coarse_graph_filename.substr(
                            _config.partitioning.coarse_graph_filename.find_last_of("/") + 1));

  DBG(dbg_partition_initial_partitioning, "# unconnected hypernodes = "
      << std::to_string([&]() {
                          HypernodeID count = 0;
                          for (auto && hn : hg.nodes()) {
                            if (hg.nodeDegree(hn) == 0) {
                              ++count;
                            }
                          }
                          return count;
                        } ()));

  HmetisToCoarsenedMapping hmetis_to_hg(hg.numNodes(), 0);
  CoarsenedToHmetisMapping hg_to_hmetis;
  createMappingsForInitialPartitioning(hmetis_to_hg, hg_to_hmetis, hg);

  io::writeHypergraphForhMetisPartitioning(hg, _config.partitioning.coarse_graph_filename,
                                           hg_to_hmetis);

  std::vector<PartitionID> partitioning;
  std::vector<PartitionID> best_partitioning;
  partitioning.reserve(hg.numNodes());
  best_partitioning.reserve(hg.numNodes());

  HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
  HyperedgeWeight current_cut = std::numeric_limits<HyperedgeWeight>::max();

  for (int attempt = 0; attempt < _config.partitioning.initial_partitioning_attempts; ++attempt) {
    int seed = Randomize::newRandomSeed();

    std::system((std::string("/home/schlag/hmetis-2.0pre1/Linux-x86_64/hmetis2.0pre1 ")
                 + _config.partitioning.coarse_graph_filename
                 + " " + std::to_string(_config.partitioning.k)
                 + " -seed=" + std::to_string(seed)
                 + " -ufactor=" + std::to_string(_config.partitioning.epsilon * 50)
                 + (_config.partitioning.verbose_output ? "" : " > /dev/null")).c_str());

    io::readPartitionFile(_config.partitioning.coarse_graph_partition_filename, partitioning);
    ASSERT(partitioning.size() == hg.numNodes(), "Partition file has incorrect size");

    current_cut = metrics::hyperedgeCut(hg, hg_to_hmetis, partitioning);
    DBG(dbg_partition_initial_partitioning, "attempt " << attempt << " seed("
        << seed << "):" << current_cut);
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
    hg.changeNodePartition(hmetis_to_hg[i], hg.partitionIndex(hmetis_to_hg[i]),
                           best_partitioning[i]);
  }
  ASSERT(metrics::hyperedgeCut(hg) == best_cut, "Cut induced by hypergraph does not equal "
         << "best initial cut");
}
} // namespace partition
