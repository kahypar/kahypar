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
#include "lib/io/PartitioningOutput.h"
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
using datastructure::extractPartAsUnpartitionedHypergraphForBisection;

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
  using HypergraphPtr = std::unique_ptr<Hypergraph, void (*)(Hypergraph*)>;
  using MappingStack = std::vector<std::vector<HypernodeID> >;

  enum {
    kInitialParallelHEremoval = 0,
    kInitialLargeHEremoval = 1,
    kCoarsening = 2,
    kInitialPartitioning = 3,
    kUncoarseningRefinement = 4,
    kInitialLargeHErestore = 5,
    kInitialParallelHErestore = 6
  };

  enum class RBHypergraphState : std::uint8_t {
    unpartitioned,
    partitionedAndPart1Extracted,
    finished
  };

  struct RBState {
    HypergraphPtr hypergraph;
    RBHypergraphState state;
    PartitionID lower_k;
    PartitionID upper_k;
    RBState(HypergraphPtr h, RBHypergraphState s, const PartitionID lk, const PartitionID uk) :
      hypergraph(std::move(h)),
      state(s),
      lower_k(lk),
      upper_k(uk) { }
  };

 public:
  explicit Partitioner() :
    _timings(),
    _internals() { }

  Partitioner(const Partitioner&) = delete;
  Partitioner& operator= (const Partitioner&) = delete;

  Partitioner(Partitioner&&) = delete;
  Partitioner& operator= (Partitioner&&) = delete;

  inline void partition(Hypergraph& hypergraph, const Configuration& config);

  const std::array<std::chrono::duration<double>, 7> & timings() const {
    return _timings;
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

  inline void performDirectKwayPartitioning(Hypergraph& hypergraph, const Configuration& config);

  inline void performRecursiveBisectionPartitioning(Hypergraph& hypergraph,
                                                    const Configuration& config);

  inline void removeLargeHyperedges(Hypergraph& hg, Hyperedges& removed_hyperedges,
                                    const Configuration& config);
  inline void restoreLargeHyperedges(Hypergraph& hg, const Hyperedges& removed_hyperedges);
  inline void createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                                   CoarsenedToHmetisMapping& hg_to_hmetis,
                                                   const Hypergraph& hg);
  void performInitialPartitioning(Hypergraph& hg, const Configuration& config);
  inline void removeParallelHyperedges(Hypergraph& hypergraph, const Configuration& config);
  inline void restoreParallelHyperedges(Hypergraph& hypergraph);
  inline void partition(Hypergraph& hypergraph, ICoarsener& coarsener,
                        IRefiner& refiner, const Configuration& config,
                        const PartitionID k1, const PartitionID k2);

  inline bool partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener,
                              IRefiner& refiner, const Configuration& config,
                              const int vcycle, const PartitionID k1, const PartitionID k2);
  inline HypernodeID originalHypernode(const HypernodeID hn,
                                       const MappingStack& mapping_stack) const;
  inline double calculateRelaxedEpsilon(const HypernodeWeight original_hypergraph_weight,
                                        const HypernodeWeight current_hypergraph_weight,
                                        const PartitionID k,
                                        const Configuration& original_config) const;

  inline Configuration createConfigurationForCurrentBisection(const Configuration& original_config,
                                                              const Hypergraph& original_hypergraph,
                                                              const Hypergraph& current_hypergraph,
                                                              const PartitionID current_k ,
                                                              const PartitionID k0,
                                                              const PartitionID k1) const;

  std::array<std::chrono::duration<double>, 7> _timings;
  std::string _internals;
};

inline void Partitioner::partition(Hypergraph& hypergraph, const Configuration& config) {
  if (config.partition.initial_parallel_he_removal) {
    removeParallelHyperedges(hypergraph, config);
  }

  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  std::vector<HyperedgeID> removed_hyperedges;
  removeLargeHyperedges(hypergraph, removed_hyperedges, config);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  _timings[kInitialLargeHEremoval] = end - start;

  switch (config.partition.mode) {
    case Mode::recursive_bisection:
      performRecursiveBisectionPartitioning(hypergraph, config);
      break;
    case Mode::direct_kway:
      performDirectKwayPartitioning(hypergraph, config);
      break;
  }

  start = std::chrono::high_resolution_clock::now();
  restoreLargeHyperedges(hypergraph, removed_hyperedges);
  end = std::chrono::high_resolution_clock::now();
  _timings[kInitialLargeHErestore] = end - start;

  if (config.partition.initial_parallel_he_removal) {
    restoreParallelHyperedges(hypergraph);
  }
}

inline void Partitioner::partition(Hypergraph& hypergraph, ICoarsener& coarsener,
                                   IRefiner& refiner, const Configuration& config,
                                   const PartitionID k1, const PartitionID k2) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(config.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  _timings[kCoarsening] += end - start;

  utils::gatherCoarseningStats(hypergraph, 0, k1, k2);

  start = std::chrono::high_resolution_clock::now();
  performInitialPartitioning(hypergraph, config);
  end = std::chrono::high_resolution_clock::now();
  _timings[kInitialPartitioning] = end - start;

  hypergraph.sortConnectivitySets();
  hypergraph.initializeNumCutHyperedges();

  start = std::chrono::high_resolution_clock::now();
  const bool found_improved_cut = coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  _timings[kUncoarseningRefinement] += end - start;
}

inline bool Partitioner::partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener,
                                         IRefiner& refiner, const Configuration& config,
                                         const int vcycle, const PartitionID k1,
                                         const PartitionID k2) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(config.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  _timings[kCoarsening] += end - start;

  utils::gatherCoarseningStats(hypergraph, vcycle, k1, k2);

  hypergraph.initializeNumCutHyperedges();

  start = std::chrono::high_resolution_clock::now();
  const bool found_improved_cut = coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  _timings[kUncoarseningRefinement] += end - start;
  return found_improved_cut;
}

inline HypernodeID Partitioner::originalHypernode(const HypernodeID hn,
                                                  const MappingStack& mapping_stack) const {
  HypernodeID node = hn;
  for (auto it = mapping_stack.crbegin(); it != mapping_stack.crend(); ++it) {
    node = (*it)[node];
  }
  return node;
}

inline double Partitioner::calculateRelaxedEpsilon(const HypernodeWeight original_hypergraph_weight,
                                                   const HypernodeWeight current_hypergraph_weight,
                                                   const PartitionID k,
                                                   const Configuration& original_config) const {
  double base = ceil(static_cast<double>(original_hypergraph_weight) / original_config.partition.k)
                / ceil(static_cast<double>(current_hypergraph_weight) / k)
                * (1.0 + original_config.partition.epsilon);
  return std::pow(base, 1.0 / ceil(log2(static_cast<double>(k)))) - 1.0;
}

inline Configuration Partitioner::createConfigurationForCurrentBisection(
    const Configuration& original_config, const Hypergraph& original_hypergraph,
    const Hypergraph& current_hypergraph, const PartitionID current_k,
    const PartitionID k0, const PartitionID k1) const {
  Configuration current_config(original_config);
  current_config.partition.k = 2;
  current_config.partition.epsilon = calculateRelaxedEpsilon(original_hypergraph.totalWeight(),
                                                             current_hypergraph.totalWeight(),
                                                             current_k,
                                                             original_config);
  LOG(V(current_config.partition.epsilon));
  current_config.partition.total_graph_weight = current_hypergraph.totalWeight();

  current_config.partition.max_part_weights[0] =
      (1 + current_config.partition.epsilon)
      * ceil( (k0 / static_cast<double>(current_k))
              *static_cast<double>(current_config.partition.total_graph_weight));

  current_config.partition.max_part_weights[1] =
      (1 + current_config.partition.epsilon)
      * ceil( (k1 / static_cast<double>(current_k))
              *static_cast<double>(current_config.partition.total_graph_weight));

  current_config.coarsening.contraction_limit =
    current_config.coarsening.contraction_limit_multiplier * current_config.partition.k;

  current_config.coarsening.hypernode_weight_fraction =
    current_config.coarsening.max_allowed_weight_multiplier
    / current_config.coarsening.contraction_limit;

  current_config.coarsening.max_allowed_node_weight =
    current_config.coarsening.hypernode_weight_fraction *
    current_config.partition.total_graph_weight;

  current_config.fm_local_search.beta = log(current_hypergraph.numNodes());

  current_config.partition.hmetis_ub_factor =
    100.0 * ((1 + current_config.partition.epsilon)
             * (ceil(static_cast<double>(current_config.partition.total_graph_weight)
                     / current_config.partition.k)
                / current_config.partition.total_graph_weight) - 0.5);

  current_config.partition.coarse_graph_partition_filename =
    current_config.partition.coarse_graph_filename + ".part."
    + std::to_string(current_config.partition.k);
  return current_config;
}


inline void Partitioner::performRecursiveBisectionPartitioning(Hypergraph& input_hypergraph,
                                                               const Configuration& original_config) {
  // Custom deleters for Hypergraphs stored in hypergraph_stack. The top-level
  // hypergraph is the input hypergraph, which is not supposed to be deleted.
  // All extracted hypergraphs however can be deleted as soon as they are not needed
  // anymore.
  auto no_delete = [](Hypergraph*) { };
  auto delete_hypergraph = [](Hypergraph* h) {
                             delete h;
                           };

  std::vector<RBState> hypergraph_stack;
  MappingStack mapping_stack;
  hypergraph_stack.emplace_back(HypergraphPtr(&input_hypergraph, no_delete),
                                RBHypergraphState::unpartitioned,
                                0, (original_config.partition.k - 1));

  while (!hypergraph_stack.empty()) {
    Hypergraph& current_hypergraph = *hypergraph_stack.back().hypergraph;

    if (hypergraph_stack.back().lower_k == hypergraph_stack.back().upper_k) {
      for (const HypernodeID hn : current_hypergraph.nodes()) {
        const HypernodeID original_hn = originalHypernode(hn, mapping_stack);
        const PartitionID current_part = input_hypergraph.partID(original_hn);
        ASSERT(current_part != Hypergraph::kInvalidPartition, V(current_part));
        if (current_part != hypergraph_stack.back().lower_k) {
          input_hypergraph.changeNodePart(original_hn, current_part, hypergraph_stack.back().lower_k);
        }
      }
      hypergraph_stack.pop_back();
      mapping_stack.pop_back();
      continue;
    }

    const PartitionID k1 = hypergraph_stack.back().lower_k;
    const PartitionID k2 = hypergraph_stack.back().upper_k;
    const RBHypergraphState state = hypergraph_stack.back().state;
    const PartitionID k = k2 - k1 + 1;
    const PartitionID km = k / 2;

    switch (state) {
      case RBHypergraphState::finished:
        hypergraph_stack.pop_back();
        if (!mapping_stack.empty()) {
          mapping_stack.pop_back();
        }
        break;
      case RBHypergraphState::unpartitioned: {
          Configuration current_config = createConfigurationForCurrentBisection(original_config,
                                                                                input_hypergraph,
                                                                                current_hypergraph,
                                                                                k, km, k -km);
          std::unique_ptr<ICoarsener> coarsener(CoarsenerFactory::getInstance().createObject(
                                                  current_config.partition.coarsening_algorithm,
                                                  current_hypergraph, current_config,
                                                  current_hypergraph.weightOfHeaviestNode()));
          std::unique_ptr<IRefiner> refiner(RefinerFactory::getInstance().createObject(
                                              current_config.partition.refinement_algorithm,
                                              current_hypergraph, current_config));

          // TODO(schlag): find better solution
          if (_internals.empty()) {
            _internals.append(coarsener->policyString() + " " + refiner->policyString());
          }

          io::printHypergraphInfo(current_hypergraph, "---");

          // TODO(schlag): we could integrate v-cycles in a similar fashion as is
          // performDirectKwayPartitioning
          partition(current_hypergraph, *coarsener, *refiner, current_config, k1, k2);

          std::cout << "-------------------------------------------------------------------------------------------" << std::endl;
          auto extractedHypergraph_1 =
            extractPartAsUnpartitionedHypergraphForBisection(current_hypergraph, 1);
          mapping_stack.emplace_back(std::move(extractedHypergraph_1.second));

          hypergraph_stack.back().state = RBHypergraphState::partitionedAndPart1Extracted;
          hypergraph_stack.emplace_back(
            HypergraphPtr(extractedHypergraph_1.first.release(), delete_hypergraph),
            RBHypergraphState::unpartitioned,
            k1 + km, k2);
        }
        break;
      case RBHypergraphState::partitionedAndPart1Extracted: {
          auto extractedHypergraph_0 =
            extractPartAsUnpartitionedHypergraphForBisection(current_hypergraph, 0);
          mapping_stack.emplace_back(std::move(extractedHypergraph_0.second));
          hypergraph_stack.back().state = RBHypergraphState::finished;
          hypergraph_stack.emplace_back(
            HypergraphPtr(extractedHypergraph_0.first.release(), delete_hypergraph),
            RBHypergraphState::unpartitioned,
            k1, k1 + km - 1);
        }
        break;
    }
  }
}

inline void Partitioner::performDirectKwayPartitioning(Hypergraph& hypergraph,
                                                       const Configuration& config) {
  std::unique_ptr<ICoarsener> coarsener(
    CoarsenerFactory::getInstance().createObject(
      config.partition.coarsening_algorithm, hypergraph, config,
      hypergraph.weightOfHeaviestNode()));

  std::unique_ptr<IRefiner> refiner(RefinerFactory::getInstance().createObject(
                                      config.partition.refinement_algorithm,
                                      hypergraph, config));

  // TODO(schlag): find better solution
  _internals.append(coarsener->policyString() + " " + refiner->policyString());

  partition(hypergraph, *coarsener, *refiner, config, 0, (config.partition.k - 1));

  DBG(dbg_partition_vcycles, "PartitioningResult: cut=" << metrics::hyperedgeCut(hypergraph));
#ifndef NDEBUG
  HyperedgeWeight initial_cut = std::numeric_limits<HyperedgeWeight>::max();
#endif

  for (int vcycle = 1; vcycle <= config.partition.global_search_iterations; ++vcycle) {
    const bool found_improved_cut = partitionVCycle(hypergraph, *coarsener, *refiner, config,
                                                    vcycle, 0, (config.partition.k - 1));

    DBG(dbg_partition_vcycles, "vcycle # " << vcycle << ": cut=" << metrics::hyperedgeCut(hypergraph));
    if (!found_improved_cut) {
      LOG("Cut could not be decreased in v-cycle " << vcycle << ". Stopping global search.");
      break;
    }

    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut, "Uncoarsening worsened cut:"
           << metrics::hyperedgeCut(hypergraph) << ">" << initial_cut);
#ifndef NDEBUG
    initial_cut = metrics::hyperedgeCut(hypergraph);
#endif
  }
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

inline void Partitioner::removeLargeHyperedges(Hypergraph& hg, Hyperedges& removed_hyperedges,
                                               const Configuration& config) {
  if (config.partition.hyperedge_size_threshold != -1) {
    for (const HyperedgeID he : hg.edges()) {
      if (hg.edgeSize(he) > config.partition.hyperedge_size_threshold) {
        DBG(dbg_partition_large_he_removal, "Hyperedge " << he << ": size ("
            << hg.edgeSize(he) << ")   exceeds threshold: "
            << config.partition.hyperedge_size_threshold);
        removed_hyperedges.push_back(he);
        hg.removeEdge(he, false);
      }
    }
  }
  Stats::instance().add("numInitiallyRemovedLargeHEs", config.partition.current_v_cycle,
                        removed_hyperedges.size());
  LOG("removed " << removed_hyperedges.size() << " HEs that had more than "
      << config.partition.hyperedge_size_threshold << " pins");
}

inline void Partitioner::restoreLargeHyperedges(Hypergraph& hg,
                                                const Hyperedges& removed_hyperedges) {
  for (auto && edge = removed_hyperedges.rbegin(); edge != removed_hyperedges.rend(); ++edge) {
    DBG(dbg_partition_large_he_removal, " restore Hyperedge " << *edge);
    hg.restoreEdge(*edge);
  }
}

inline void Partitioner::removeParallelHyperedges(Hypergraph&, const Configuration&) {
  // HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  throw std::runtime_error("Not Implemented");
  // HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  // _timings[kInitialParallelHEremoval] = end - start;
  // LOG("Initially removed parallel HEs:" << Stats::instance().toString());
}

inline void Partitioner::restoreParallelHyperedges(Hypergraph&) {
  // HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  // HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  // _timings[kInitialParallelHErestore] = end - start;
}
}  // namespace partition

#endif  // SRC_PARTITION_PARTITIONER_H_
