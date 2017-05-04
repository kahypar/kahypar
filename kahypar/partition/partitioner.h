/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "gtest/gtest_prod.h"

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/io/partitioning_output.h"
#include "kahypar/partition/coarsening/hypergraph_pruner.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/direct_kway.h"
#include "kahypar/partition/factories.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/preprocessing/large_hyperedge_remover.h"
#include "kahypar/partition/preprocessing/min_hash_sparsifier.h"
#include "kahypar/partition/preprocessing/single_node_hyperedge_remover.h"
#include "kahypar/partition/recursive_bisection.h"

namespace kahypar {
// Workaround for bug in gtest
// Because of different namespaces it is not possible to use
// FRIEND_TEST macro.
namespace io {
class APartitionOfAHypergraph_IsCorrectlyWrittenToFile_Test;
}  // namespace io

namespace metrics {
class APartitionedHypergraph;
}  // namespace metrics


namespace partition {
static inline void partition(Hypergraph& hypergraph, const Context& context) {
  switch (context.partition.mode) {
    case Mode::recursive_bisection:
      recursive_bisection::partition(hypergraph, context);
      break;
    case Mode::direct_kway:
      direct_kway::partition(hypergraph, context);
      break;
  }
}
}  // namespace partition

class Partitioner {
 private:
  static constexpr bool debug = false;

 public:
  Partitioner() :
    _single_node_he_remover(),
    _large_he_remover(),
    _pin_sparsifier(),
    _internals() { }

  Partitioner(const Partitioner&) = delete;
  Partitioner& operator= (const Partitioner&) = delete;

  Partitioner(Partitioner&&) = delete;
  Partitioner& operator= (Partitioner&&) = delete;

  ~Partitioner() = default;

  inline void partition(Hypergraph& hypergraph, Context& context);

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
  FRIEND_TEST(APartitioner, UsesKaHyParPartitioningOnCoarsestHypergraph);
  FRIEND_TEST(APartitioner, UncoarsensTheInitiallyPartitionedHypergraph);
  FRIEND_TEST(APartitioner, CalculatesPinCountsOfAHyperedgesAfterInitialPartitioning);
  FRIEND_TEST(APartitioner, CanUseVcyclesAsGlobalSearchStrategy);
  friend class io::APartitionOfAHypergraph_IsCorrectlyWrittenToFile_Test;
  friend class metrics::APartitionedHypergraph;

  static inline void setupContext(const Hypergraph& hypergraph, Context& context);

  static inline void configurePreprocessing(const Hypergraph& hypergraph, Context& context);

  inline void preprocess(Hypergraph& hypergraph, const Context& context);
  inline void preprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                         const Context& context);

  inline void postprocess(Hypergraph& hypergraph, const Context& context);
  inline void postprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                          const Context& context);

  SingleNodeHyperedgeRemover _single_node_he_remover;
  LargeHyperedgeRemover _large_he_remover;
  MinHashSparsifier _pin_sparsifier;
  std::string _internals;
};

inline void Partitioner::configurePreprocessing(const Hypergraph& hypergraph,
                                                Context& context) {
  if (context.preprocessing.enable_min_hash_sparsifier) {
    // determine whether or not to apply the sparsifier
    std::vector<HypernodeID> he_sizes;
    he_sizes.reserve(hypergraph.currentNumEdges());
    for (const auto& he : hypergraph.edges()) {
      he_sizes.push_back(hypergraph.edgeSize(he));
    }
    std::sort(he_sizes.begin(), he_sizes.end());
    if (kahypar::math::median(he_sizes) >=
        context.preprocessing.min_hash_sparsifier.min_median_he_size) {
      context.preprocessing.min_hash_sparsifier.is_active = true;
    }
  }

  if (context.preprocessing.enable_louvain_community_detection &&
      context.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::hybrid) {
    const double density = static_cast<double>(hypergraph.initialNumEdges()) /
                           static_cast<double>(hypergraph.initialNumNodes());
    if (density < 0.75) {
      context.preprocessing.louvain_community_detection.edge_weight = LouvainEdgeWeight::degree;
    } else {
      context.preprocessing.louvain_community_detection.edge_weight = LouvainEdgeWeight::uniform;
    }
  }
}

inline void Partitioner::setupContext(const Hypergraph& hypergraph, Context& context) {
  context.partition.total_graph_weight = hypergraph.totalWeight();

  context.coarsening.contraction_limit =
    context.coarsening.contraction_limit_multiplier * context.partition.k;

  context.coarsening.hypernode_weight_fraction =
    context.coarsening.max_allowed_weight_multiplier
    / context.coarsening.contraction_limit;

  context.partition.perfect_balance_part_weights[0] = ceil(
    context.partition.total_graph_weight
    / static_cast<double>(context.partition.k));
  context.partition.perfect_balance_part_weights[1] =
    context.partition.perfect_balance_part_weights[0];

  context.partition.max_part_weights[0] = (1 + context.partition.epsilon)
                                          * context.partition.perfect_balance_part_weights[0];
  context.partition.max_part_weights[1] = context.partition.max_part_weights[0];

  context.coarsening.max_allowed_node_weight = ceil(context.coarsening.hypernode_weight_fraction
                                                    * context.partition.total_graph_weight);

  // the main partitioner should track stats
  context.partition.collect_stats = true;
}

inline void Partitioner::preprocess(Hypergraph& hypergraph, const Context& context) {
  if (context.partition.verbose_output) {
    LOG << "\n********************************************************************************";
    LOG << "*                               Preprocessing...                               *";
    LOG << "********************************************************************************";
  }
  const auto result = _single_node_he_remover.removeSingleNodeHyperedges(hypergraph);
  if (context.partition.verbose_output && result.num_removed_single_node_hes > 0) {
    LOG << "\033[1m\033[31m" << "Removed" << result.num_removed_single_node_hes
        << "hyperedges with |e|=1" << "\033[0m";
    LOG << "\033[1m\033[31m" << "===>" << result.num_unconnected_hns
        << "unconnected HNs could have been removed" << "\033[0m";
  }

  if (context.preprocessing.remove_always_cut_hes) {
    const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    _large_he_remover.removeLargeHyperedges(hypergraph, context);
    const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    context.stats.preprocessing("LargeHEremovalTime") +=
      std::chrono::duration<double>(end - start).count();
    if (context.isMainRecursiveBisection()) {
      context.stats.topLevel().preprocessing("LargeHEremovalTime") +=
        std::chrono::duration<double>(end - start).count();
    }
  }
}

inline void Partitioner::preprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                                    const Context& context) {
  preprocess(hypergraph, context);
  ASSERT(context.preprocessing.enable_min_hash_sparsifier);

  const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  sparse_hypergraph = _pin_sparsifier.buildSparsifiedHypergraph(hypergraph, context);
  const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();

  context.stats.preprocessing("MinHashSparsifierTime") +=
    std::chrono::duration<double>(end - start).count();
  if (context.isMainRecursiveBisection()) {
    context.stats.topLevel().preprocessing("MinHashSparsifierTime") +=
      std::chrono::duration<double>(end - start).count();
  }

  if (context.partition.verbose_output) {
    LOG << "After sparsification:";
    kahypar::io::printHypergraphInfo(sparse_hypergraph, "sparsified hypergraph");
  }
}

inline void Partitioner::postprocess(Hypergraph& hypergraph, const Context& context) {
  if (context.preprocessing.remove_always_cut_hes) {
    const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    _large_he_remover.restoreLargeHyperedges(hypergraph);
    const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    context.stats.postprocessing("LargeHErestoreTime") +=
      std::chrono::duration<double>(end - start).count();
    if (context.isMainRecursiveBisection()) {
      context.stats.topLevel().postprocessing("LargeHErestoreTime") +=
        std::chrono::duration<double>(end - start).count();
    }
  }
  _single_node_he_remover.restoreSingleNodeHyperedges(hypergraph);
}


inline void Partitioner::postprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                                     const Context& context) {
  ASSERT(context.preprocessing.enable_min_hash_sparsifier);
  const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  _pin_sparsifier.applyPartition(sparse_hypergraph, hypergraph);
  const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  context.stats.postprocessing("MinHashSparsifierTime") +=
    std::chrono::duration<double>(end - start).count();
  if (context.isMainRecursiveBisection()) {
    context.stats.topLevel().postprocessing("MinHashSparsifierTime") +=
      std::chrono::duration<double>(end - start).count();
  }
  postprocess(hypergraph, context);
}

inline void Partitioner::partition(Hypergraph& hypergraph, Context& context) {
  configurePreprocessing(hypergraph, context);

  setupContext(hypergraph, context);
  if (context.type == ContextType::main && !context.partition.quiet_mode) {
    LOG << context;
    if (context.partition.verbose_output) {
      LOG << "\n********************************************************************************";
      LOG << "*                                    Input                                     *";
      LOG << "********************************************************************************";
      io::printHypergraphInfo(hypergraph, context.partition.graph_filename.substr(
                                context.partition.graph_filename.find_last_of('/') + 1));
    }
  }
  if (context.preprocessing.min_hash_sparsifier.is_active) {
    Hypergraph sparseHypergraph;
    preprocess(hypergraph, sparseHypergraph, context);
    partition::partition(sparseHypergraph, context);
    postprocess(hypergraph, sparseHypergraph, context);
  } else {
    preprocess(hypergraph, context);
    partition::partition(hypergraph, context);
    postprocess(hypergraph, context);
  }
}
}  // namespace kahypar
