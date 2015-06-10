/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_PARTITIONER_H_
#define SRC_PARTITION_PARTITIONER_H_

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <memory>
#include <random>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"
#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "lib/utils/Stats.h"
#include "partition/Configuration.h"
#include "partition/Factories.h"
#include "partition/coarsening/HypergraphPruner.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "tools/RandomFunctions.h"

#ifndef NDEBUG
#include "partition/Metrics.h"
#endif

using defs::Hypergraph;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using defs::PartitionID;
using defs::HypernodeID;
using defs::HighResClockTimepoint;
using utils::Stats;

// Workaround for bug in gtest
// Because of different namespaces it is not possible to use
// FRIEND_TEST macro.
namespace io {
class APartitionOfAHypergraph_IsCorrectlyWrittenToFile_Test;
}

namespace metrics {
class APartitionedHypergraph;
}

namespace partition {
static const bool dbg_partition_large_he_removal = false;
static const bool dbg_partition_large_he_restore = false;
static const bool dbg_partition_initial_partitioning = true;
static const bool dbg_partition_vcycles = true;

class Partitioner {
  using CoarsenedToHmetisMapping = std::unordered_map<HypernodeID, HypernodeID>;
  using HmetisToCoarsenedMapping = std::vector<HypernodeID>;
  using PartitionWeights = std::vector<HypernodeWeight>;
  using Hyperedges = std::vector<HyperedgeID>;

  enum {
    kInitialParallelHEremoval = 0,
    kInitialLargeHEremoval = 1,
    kCoarsening = 2,
    kInitialPartitioning = 3,
    kUncoarseningRefinement = 4,
    kInitialLargeHErestore = 5,
    kInitialParallelHErestore = 6
  };

 public:
  Partitioner(const Partitioner&) = delete;
  Partitioner(Partitioner&&) = delete;
  Partitioner& operator= (const Partitioner&) = delete;
  Partitioner& operator= (Partitioner&&) = delete;

  explicit Partitioner(Configuration& config) :
    _config(config),
    _stats(),
    _timings(),
    _internals() { }

  inline void performDirectKwayPartitioning(Hypergraph& hypergraph);


  const std::array<std::chrono::duration<double>, 7> & timings() const {
    return _timings;
  }

  const Stats & stats() const {
    return _stats;
  }

  const std::string internals() const {
    return _internals;
  }

 private:
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold, RemovesHyperedgesExceedingThreshold);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold, RestoresHyperedgesExceedingThreshold);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              PartitionsUnpartitionedHypernodesAfterRestore);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              AssignsAllRemainingHypernodesToDefinedPartition);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              TriesToMinimizesCutIfNoPinOfRemainingHyperedgeIsPartitioned);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              TriesToMinimizesCutIfOnlyOnePartitionIsUsed);
  FRIEND_TEST(APartitionerWithHyperedgeSizeThreshold,
              DistributesAllRemainingHypernodesToMinimizeImbalaceIfCutCannotBeMinimized);
  FRIEND_TEST(APartitioner, UseshMetisPartitioningOnCoarsestHypergraph);
  FRIEND_TEST(APartitioner, UncoarsensTheInitiallyPartitionedHypergraph);
  FRIEND_TEST(APartitioner, CalculatesPinCountsOfAHyperedgesAfterInitialPartitioning);
  FRIEND_TEST(APartitioner, CanUseVcyclesAsGlobalSearchStrategy);
  friend class io::APartitionOfAHypergraph_IsCorrectlyWrittenToFile_Test;
  friend class metrics::APartitionedHypergraph;

  void performDirectKwayPartitioning(Hypergraph& hypergraph, ICoarsener& coarsener,
                                     IRefiner& refiner);

  inline void removeLargeHyperedges(Hypergraph& hg, Hyperedges& removed_hyperedges);
  inline void restoreLargeHyperedges(Hypergraph& hg, const Hyperedges& removed_hyperedges);
  inline void createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                                   CoarsenedToHmetisMapping& hg_to_hmetis,
                                                   const Hypergraph& hg);
  void performInitialPartitioning(Hypergraph& hg);
  inline void removeParallelHyperedges(Hypergraph& hypergraph);
  inline void restoreParallelHyperedges(Hypergraph& hypergraph);
  inline void partition(Hypergraph& hypergraph, ICoarsener& coarsener,
                        IRefiner& refiner);

  inline bool partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener,
                              IRefiner& refiner);

  Configuration& _config;
  Stats _stats;
  std::array<std::chrono::duration<double>, 7> _timings;
  std::string _internals;
};

void Partitioner::performDirectKwayPartitioning(Hypergraph& hypergraph) {
  std::unique_ptr<ICoarsener> coarsener(
    CoarsenerFactory::getInstance().createObject(
      _config.partition.coarsening_algorithm, hypergraph, _config));

  std::unique_ptr<IRefiner> refiner(RefinerFactory::getInstance().createObject(
                                      _config.partition.refinement_algorithm,
                                      hypergraph, _config));

  // TODO(schlag): fix this
  _internals.append(coarsener->policyString());
  _internals.append(" ");
  _internals.append(refiner->policyString());
  performDirectKwayPartitioning(hypergraph, *coarsener, *refiner);
}

inline void Partitioner::partition(Hypergraph& hypergraph, ICoarsener& coarsener,
                                   IRefiner& refiner) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(_config.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  _timings[kCoarsening] += end - start;

  start = std::chrono::high_resolution_clock::now();
  performInitialPartitioning(hypergraph);
  end = std::chrono::high_resolution_clock::now();
  _timings[kInitialPartitioning] = end - start;

  start = std::chrono::high_resolution_clock::now();
  const bool found_improved_cut = coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  _timings[kUncoarseningRefinement] += end - start;
}

inline bool Partitioner::partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener,
                                         IRefiner& refiner) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(_config.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  _timings[kCoarsening] += end - start;

  start = std::chrono::high_resolution_clock::now();
  const bool found_improved_cut = coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  _timings[kUncoarseningRefinement] += end - start;
  return found_improved_cut;
}



inline void Partitioner::createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                                              CoarsenedToHmetisMapping& hg_to_hmetis,
                                                              const Hypergraph& hg) {
  int i = 0;
  for (const HypernodeID hn : hg.nodes()) {
    hg_to_hmetis[hn] = i;
    hmetis_to_hg[i] = hn;
    ++i;
  }
}

inline void Partitioner::removeLargeHyperedges(Hypergraph& hg, Hyperedges& removed_hyperedges) {
  if (_config.partition.hyperedge_size_threshold != -1) {
    for (const HyperedgeID he : hg.edges()) {
      if (hg.edgeSize(he) > _config.partition.hyperedge_size_threshold) {
        DBG(dbg_partition_large_he_removal, "Hyperedge " << he << ": size ("
            << hg.edgeSize(he) << ")   exceeds threshold: "
            << _config.partition.hyperedge_size_threshold);
        removed_hyperedges.push_back(he);
        hg.removeEdge(he, false);
      }
    }
  }
  _stats.add("numInitiallyRemovedLargeHEs", _config.partition.current_v_cycle,
             removed_hyperedges.size());
  LOG("removed " << removed_hyperedges.size() << " HEs that had more than "
      << _config.partition.hyperedge_size_threshold << " pins");
}

inline void Partitioner::restoreLargeHyperedges(Hypergraph& hg,
                                                const Hyperedges& removed_hyperedges) {
  for (auto && edge = removed_hyperedges.rbegin(); edge != removed_hyperedges.rend(); ++edge) {
    DBG(dbg_partition_large_he_removal, " restore Hyperedge " << *edge);
    hg.restoreEdge(*edge);
  }
}

inline void Partitioner::removeParallelHyperedges(Hypergraph&) {
  // HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  throw std::runtime_error("Not Implemented");
  // HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  // _timings[kInitialParallelHEremoval] = end - start;
  // LOG("Initially removed parallel HEs:" << _stats.toString());
}

inline void Partitioner::restoreParallelHyperedges(Hypergraph&) {
  // HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  // HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  // _timings[kInitialParallelHErestore] = end - start;
}
}  // namespace partition

#endif  // SRC_PARTITION_PARTITIONER_H_
