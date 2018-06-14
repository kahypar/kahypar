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
#include "kahypar/partition/preprocessing/louvain.h"
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
  ASSERT([&]() {
        if (context.partition.mode != Mode::recursive_bisection &&
            context.preprocessing.enable_community_detection) {
          return !std::all_of(hypergraph.communities().cbegin(),
                              hypergraph.communities().cend(),
                              [](auto i) {
            return i == 0;
          });
        }
        return true;
      } ());
  switch (context.partition.mode) {
    case Mode::recursive_bisection:

      recursive_bisection::partition(hypergraph, context);

      break;
    case Mode::direct_kway:
      direct_kway::partition(hypergraph, context);
      break;
    case Mode::UNDEFINED:
      LOG << "Partitioning Mode undefined!";
      std::exit(-1);
  }
}
}  // namespace partition

class Partitioner {
 private:
  static constexpr bool debug = false;

 public:
  Partitioner() :
    _single_node_he_remover(),
    _pin_sparsifier() { }

  Partitioner(const Partitioner&) = delete;
  Partitioner& operator= (const Partitioner&) = delete;

  Partitioner(Partitioner&&) = delete;
  Partitioner& operator= (Partitioner&&) = delete;

  ~Partitioner() = default;

  inline void partition(Hypergraph& hypergraph, Context& context);

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

  inline void sanitize(Hypergraph& hypergraph, const Context& context);

  inline void preprocess(Hypergraph& hypergraph, const Context& context);
  inline void preprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                         const Context& context);

  inline void postprocess(Hypergraph& hypergraph);
  inline void postprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                          const Context& context);

  SingleNodeHyperedgeRemover _single_node_he_remover;
  MinHashSparsifier _pin_sparsifier;
};

inline void Partitioner::configurePreprocessing(const Hypergraph& hypergraph,
                                                Context& context) {
  // Don't use sparsification by default
  context.preprocessing.min_hash_sparsifier.is_active = false;

  if (!context.partition_evolutionary ||
      context.evolutionary.action.decision() == EvoDecision::normal) {
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
  }

  if (context.preprocessing.enable_community_detection &&
      context.preprocessing.community_detection.edge_weight == LouvainEdgeWeight::hybrid) {
    const double density = static_cast<double>(hypergraph.initialNumEdges()) /
                           static_cast<double>(hypergraph.initialNumNodes());
    if (density < 0.75) {
      context.preprocessing.community_detection.edge_weight = LouvainEdgeWeight::degree;
    } else {
      context.preprocessing.community_detection.edge_weight = LouvainEdgeWeight::uniform;
    }
  }
}

inline void Partitioner::setupContext(const Hypergraph& hypergraph, Context& context) {
  context.coarsening.contraction_limit =
    context.coarsening.contraction_limit_multiplier * context.partition.k;

  context.coarsening.hypernode_weight_fraction =
    context.coarsening.max_allowed_weight_multiplier
    / context.coarsening.contraction_limit;

  context.coarsening.max_allowed_node_weight = ceil(context.coarsening.hypernode_weight_fraction
                                                    * hypergraph.totalWeight());
  context.setupPartWeights(hypergraph.totalWeight());

  ASSERT(context.partition.perfect_balance_part_weights.size() ==
         static_cast<size_t>(context.partition.k));
  ASSERT(context.partition.max_part_weights.size() ==
         static_cast<size_t>(context.partition.k));
}

inline void Partitioner::sanitize(Hypergraph& hypergraph, const Context& context) {
  io::printTopLevelPreprocessingBanner(context);
  const auto result = _single_node_he_remover.removeSingleNodeHyperedges(hypergraph);
  if (context.partition.verbose_output && result.num_removed_single_node_hes > 0) {
    LOG << "\033[1m\033[31m" << "Removed" << result.num_removed_single_node_hes
        << "hyperedges with |e|=1" << "\033[0m";
    LOG << "\033[1m\033[31m" << "===>" << result.num_unconnected_hns
        << "unconnected HNs could have been removed" << "\033[0m";
  }
}

inline void Partitioner::preprocess(Hypergraph& hypergraph, const Context& context) {
  // In recursive bisection mode, we perform community detection before each
  // bisection. Therefore the 'top-level' preprocessing is disabled in this case.
  if (context.partition.mode != Mode::recursive_bisection &&
      context.preprocessing.enable_community_detection) {
    // Repeated executions of non-evolutionary KaHyPar also re-use the community structure.
    if (context.evolutionary.communities.empty() ||
        context.preprocessing.min_hash_sparsifier.is_active) {
      // If sparsification is enabled, we can't reuse the community structure
      // since each sparsification call might return a different hypergraph.
      detectCommunities(hypergraph, context);
      context.evolutionary.communities = hypergraph.communities();
    } else {
      ASSERT(hypergraph.currentNumNodes() == context.getCommunities().size());
      hypergraph.setCommunities(context.getCommunities());
    }
  }
}

inline void Partitioner::preprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                                    const Context& context) {
  ASSERT(context.preprocessing.enable_min_hash_sparsifier);

  const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  sparse_hypergraph = _pin_sparsifier.buildSparsifiedHypergraph(hypergraph, context);
  const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();

  context.stats.set(StatTag::Preprocessing, "MinHashSparsifierTime",
                    std::chrono::duration<double>(end - start).count());
  Timer::instance().add(context, Timepoint::pre_sparsifier,
                        std::chrono::duration<double>(end - start).count());

  if (context.partition.verbose_output) {
    LOG << "After sparsification:";
    kahypar::io::printHypergraphInfo(sparse_hypergraph, "sparsified hypergraph");
  }
  preprocess(sparse_hypergraph, context);
}

inline void Partitioner::postprocess(Hypergraph& hypergraph) {
  _single_node_he_remover.restoreSingleNodeHyperedges(hypergraph);
}


inline void Partitioner::postprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                                     const Context& context) {
  ASSERT(context.preprocessing.enable_min_hash_sparsifier);
  const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  _pin_sparsifier.applyPartition(sparse_hypergraph, hypergraph);
  const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  context.stats.set(StatTag::Postprocessing, "MinHashSparsifierTime",
                    std::chrono::duration<double>(end - start).count());
  Timer::instance().add(context, Timepoint::post_sparsifier_restore,
                        std::chrono::duration<double>(end - start).count());
  postprocess(hypergraph);
}

inline void Partitioner::partition(Hypergraph& hypergraph, Context& context) {
  configurePreprocessing(hypergraph, context);

  setupContext(hypergraph, context);
  io::printInputInformation(context, hypergraph);

  sanitize(hypergraph, context);

  if (context.preprocessing.min_hash_sparsifier.is_active) {
    ALWAYS_ASSERT(!context.partition_evolutionary ||
                  context.evolutionary.action.decision() == EvoDecision::normal,
                  "Sparsification is only allowed for non-evolutionary partitioning "
                  "and while filling the initial population of KaHyParE.");
    Hypergraph sparseHypergraph;
    preprocess(hypergraph, sparseHypergraph, context);
    ASSERT(sparseHypergraph.numFixedVertices() == hypergraph.numFixedVertices());
    partition::partition(sparseHypergraph, context);
    hypergraph.reset();
    postprocess(hypergraph, sparseHypergraph, context);

    // If the sparsifier is active, we can't reuse the community structure, because
    // each sparsification call might return a different hypergraph due to randomization.
    // Therefore we have to make sure that the community structure is not used somewhere
    // outside of the partitioner.
    context.evolutionary.communities.clear();
  } else {
    preprocess(hypergraph, context);
    partition::partition(hypergraph, context);
    postprocess(hypergraph);
  }

  ASSERT([&]() {
      for (const HypernodeID& hn : hypergraph.fixedVertices()) {
        if (hypergraph.partID(hn) != hypergraph.fixedVertexPartID(hn)) {
          LOG << "Hypernode" << hn << "should be in part" << hypergraph.fixedVertexPartID(hn)
              << "but actually is in" << hypergraph.partID(hn);
          return false;
        }
      }
      return true;
    } (), "Fixed Vertices are assigned incorrectly!");
}
}  // namespace kahypar
