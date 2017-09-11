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
    Hypergraph sparseHypergraph;
    preprocess(hypergraph, sparseHypergraph, context);


    // Quick fix: In evolutionary partitioning mode, we have to map the original
    // partitions of the individuals to the corresponding partition of the sparse
    // hypergraph.
    Context temp_context(context);
    LOG << V(sparseHypergraph.initialNumNodes());
    std::vector<PartitionID> sparse_parent1(sparseHypergraph.initialNumNodes(), -1);
    std::vector<PartitionID> sparse_parent2(sparseHypergraph.initialNumNodes(), -1);
    const auto& hn_to_sparse_hn = _pin_sparsifier.hnToSparsifiedHnMapping();

    if (context.evolutionary.parent1 != nullptr) {
      for (const auto& hn : hypergraph.nodes()) {
        const HypernodeID sparse_hn = hn_to_sparse_hn[hn];
        ALWAYS_ASSERT(sparse_parent1[sparse_hn] == -1 ||
                      (sparse_parent1[sparse_hn] == (*context.evolutionary.parent1)[hn]), "");
        sparse_parent1[sparse_hn] = (*context.evolutionary.parent1)[hn];
      }
      ALWAYS_ASSERT(std::none_of(sparse_parent1.cbegin(), sparse_parent1.cend(),
                                 [](PartitionID part){ return part == -1;}),"");
      temp_context.evolutionary.parent1 = &sparse_parent1;
    }

    if (context.evolutionary.parent2 != nullptr) {
      for (const auto& hn : hypergraph.nodes()) {
        const HypernodeID sparse_hn = hn_to_sparse_hn[hn];
        ALWAYS_ASSERT(sparse_parent2[sparse_hn] == -1 ||
                      (sparse_parent2[sparse_hn] ==  (*context.evolutionary.parent2)[hn]),"");
        sparse_parent2[sparse_hn] = (*context.evolutionary.parent2)[hn];
      }
      ALWAYS_ASSERT(std::none_of(sparse_parent2.cbegin(), sparse_parent2.cend(),
                                 [](PartitionID part){ return part == -1;}),"");
      temp_context.evolutionary.parent2 = &sparse_parent2;
    }

    // Quick Fix: Weak mutations set the partition of the hypergraph.
    // Since we use the sparsified version internally, we have to adapt
    // the partition of the sparse graph accordingly.
    sparseHypergraph.reset();
    for (const auto& hn : hypergraph.nodes()) {
      const HypernodeID sparse_hn = hn_to_sparse_hn[hn];
      const PartitionID hn_part = hypergraph.partID(hn);
      if (sparseHypergraph.partID(sparse_hn) != hn_part){
        sparseHypergraph.setNodePart(sparse_hn, hn_part);
      }
    }

    partition::partition(sparseHypergraph, temp_context);

    // In evolutionary mode, this might be necessary
    hypergraph.reset();

    postprocess(hypergraph, sparseHypergraph, context);
  } else {
    preprocess(hypergraph, context);
    partition::partition(hypergraph, context);
    postprocess(hypergraph);
  }
}
}  // namespace kahypar
