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
#include <chrono>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/io/partitioning_output.h"
#include "kahypar/partition/coarsening/hypergraph_pruner.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/configuration.h"
#include "kahypar/partition/factories.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/preprocessing/large_hyperedge_remover.h"
#include "kahypar/partition/preprocessing/min_hash_sparsifier.h"
#include "kahypar/partition/preprocessing/single_node_hyperedge_remover.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/utils/randomize.h"
#include "kahypar/utils/stats.h"

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


class Partitioner {
 private:
  static constexpr bool debug = false;

  using PartitionWeights = std::vector<HypernodeWeight>;
  using Hyperedges = std::vector<HyperedgeID>;
  using HypergraphPtr = std::unique_ptr<Hypergraph, void (*)(Hypergraph*)>;
  using MappingStack = std::vector<std::vector<HypernodeID> >;

  enum class RBHypergraphState : std::uint8_t {
    unpartitioned,
    partitionedAndPart1Extracted,
    finished
  };

  class RBState {
 public:
    RBState(HypergraphPtr h, RBHypergraphState s, const PartitionID lk,
            const PartitionID uk) :
      hypergraph(std::move(h)),
      state(s),
      lower_k(lk),
      upper_k(uk) { }

    HypergraphPtr hypergraph;
    RBHypergraphState state;
    const PartitionID lower_k;
    const PartitionID upper_k;
  };

 public:
  explicit Partitioner() :
    _single_node_he_remover(),
    _large_he_remover(),
    _pin_sparsifier(),
    _internals() { }

  Partitioner(const Partitioner&) = delete;
  Partitioner& operator= (const Partitioner&) = delete;

  Partitioner(Partitioner&&) = delete;
  Partitioner& operator= (Partitioner&&) = delete;

  ~Partitioner() = default;

  inline void partition(Hypergraph& hypergraph, Configuration& config);

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

  inline void setupConfig(const Hypergraph& hypergraph, Configuration& config) const;

  inline void configurePreprocessing(const Hypergraph& hypergraph, Configuration& config) const;

  inline void preprocess(Hypergraph& hypergraph, const Configuration& config);
  inline void preprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                         const Configuration& config);

  inline void partitionInternal(Hypergraph& hypergraph, Configuration& config);
  inline void performPartitioning(Hypergraph& hypergraph, const Configuration& config);

  inline void performDirectKwayPartitioning(Hypergraph& hypergraph,
                                            const Configuration& config);

  inline void performRecursiveBisectionPartitioning(Hypergraph& input_hypergraph,
                                                    const Configuration& original_config);
  inline HypernodeID originalHypernode(HypernodeID hn,
                                       const MappingStack& mapping_stack) const;

  inline double calculateRelaxedEpsilon(HypernodeWeight original_hypergraph_weight,
                                        HypernodeWeight current_hypergraph_weight,
                                        PartitionID k,
                                        const Configuration& original_config) const;

  inline Configuration createConfigurationForCurrentBisection(const Configuration& original_config,
                                                              const Hypergraph& original_hypergraph,
                                                              const Hypergraph& current_hypergraph,
                                                              PartitionID current_k,
                                                              PartitionID k0,
                                                              PartitionID k1) const;

  inline void performPartitioning(Hypergraph& hypergraph, ICoarsener& coarsener, IRefiner& refiner,
                                  const Configuration& config);

  inline void performInitialPartitioning(Hypergraph& hg, const Configuration& config);
  inline Configuration createConfigurationForInitialPartitioning(const Hypergraph& hg,
                                                                 const Configuration& original_config,
                                                                 double init_alpha) const;

  inline void postprocess(Hypergraph& hypergraph, const Configuration& config);
  inline void postprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                          const Configuration& config);


  inline bool partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener, IRefiner& refiner,
                              const Configuration& config);

  SingleNodeHyperedgeRemover _single_node_he_remover;
  LargeHyperedgeRemover _large_he_remover;
  MinHashSparsifier _pin_sparsifier;
  std::string _internals;
};

inline void Partitioner::configurePreprocessing(const Hypergraph& hypergraph,
                                                Configuration& config) const {
  if (config.preprocessing.enable_min_hash_sparsifier) {
    // determine whether or not to apply the sparsifier
    std::vector<HypernodeID> he_sizes;
    he_sizes.reserve(hypergraph.currentNumEdges());
    for (const auto& he : hypergraph.edges()) {
      he_sizes.push_back(hypergraph.edgeSize(he));
    }
    std::sort(he_sizes.begin(), he_sizes.end());
    if (kahypar::math::median(he_sizes) >=
        config.preprocessing.min_hash_sparsifier.min_median_he_size) {
      config.preprocessing.min_hash_sparsifier.is_active = true;
    }
  }

  if (config.preprocessing.enable_louvain_community_detection &&
      config.preprocessing.louvain_community_detection.edge_weight == LouvainEdgeWeight::hybrid) {
    const double density = static_cast<double>(hypergraph.initialNumEdges()) /
                           static_cast<double>(hypergraph.initialNumNodes());
    if (density < 0.75) {
      config.preprocessing.louvain_community_detection.edge_weight = LouvainEdgeWeight::degree;
    } else {
      config.preprocessing.louvain_community_detection.edge_weight = LouvainEdgeWeight::uniform;
    }
  }
}

inline void Partitioner::setupConfig(const Hypergraph& hypergraph, Configuration& config) const {
  config.partition.total_graph_weight = hypergraph.totalWeight();

  config.coarsening.contraction_limit =
    config.coarsening.contraction_limit_multiplier * config.partition.k;

  config.coarsening.hypernode_weight_fraction =
    config.coarsening.max_allowed_weight_multiplier
    / config.coarsening.contraction_limit;

  config.partition.perfect_balance_part_weights[0] = ceil(
    config.partition.total_graph_weight
    / static_cast<double>(config.partition.k));
  config.partition.perfect_balance_part_weights[1] =
    config.partition.perfect_balance_part_weights[0];

  config.partition.max_part_weights[0] = (1 + config.partition.epsilon)
                                         * config.partition.perfect_balance_part_weights[0];
  config.partition.max_part_weights[1] = config.partition.max_part_weights[0];

  config.coarsening.max_allowed_node_weight = ceil(config.coarsening.hypernode_weight_fraction
                                                   * config.partition.total_graph_weight);

  // the main partitioner should track stats
  config.partition.collect_stats = true;
}

inline void Partitioner::preprocess(Hypergraph& hypergraph, const Configuration& config) {
  if (config.partition.verbose_output) {
    LOG << "\n********************************************************************************";
    LOG << "*                               Preprocessing...                               *";
    LOG << "********************************************************************************";
  }
  const auto result = _single_node_he_remover.removeSingleNodeHyperedges(hypergraph);
  if (config.partition.verbose_output && result.num_removed_single_node_hes > 0) {
    LOG << "\033[1m\033[31m" << "Removed" << result.num_removed_single_node_hes
        << "hyperedges with |e|=1" << "\033[0m";
    LOG << "\033[1m\033[31m" << "===>" << result.num_unconnected_hns
        << "unconnected HNs could have been removed" << "\033[0m";
  }

  if (config.preprocessing.remove_always_cut_hes) {
    const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    _large_he_remover.removeLargeHyperedges(hypergraph, config);
    const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    Stats::instance().addToTotal(config, "InitialLargeHEremoval",
                                 std::chrono::duration<double>(end - start).count());
  }
}

inline void Partitioner::preprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                                    const Configuration& config) {
  preprocess(hypergraph, config);
  ASSERT(config.preprocessing.enable_min_hash_sparsifier);

  const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  sparse_hypergraph = _pin_sparsifier.buildSparsifiedHypergraph(hypergraph, config);
  const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "MinHashSparsifier",
                               std::chrono::duration<double>(end - start).count());

  if (config.partition.verbose_output) {
    LOG << "After sparsification:";
    kahypar::io::printHypergraphInfo(sparse_hypergraph, "sparsified hypergraph");
  }
}

inline void Partitioner::postprocess(Hypergraph& hypergraph, const Configuration& config) {
  if (config.preprocessing.remove_always_cut_hes) {
    const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    _large_he_remover.restoreLargeHyperedges(hypergraph);
    const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    Stats::instance().addToTotal(config, "InitialLargeHErestore",
                                 std::chrono::duration<double>(end - start).count());
  }
  _single_node_he_remover.restoreSingleNodeHyperedges(hypergraph);
}


inline void Partitioner::postprocess(Hypergraph& hypergraph, Hypergraph& sparse_hypergraph,
                                     const Configuration& config) {
  ASSERT(config.preprocessing.enable_min_hash_sparsifier);
  const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  _pin_sparsifier.applyPartition(sparse_hypergraph, hypergraph);
  const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "MinHashSparsifier",
                               std::chrono::duration<double>(end - start).count());
  postprocess(hypergraph, config);
}

inline void Partitioner::performInitialPartitioning(Hypergraph& hg, const Configuration& config) {
  auto extracted_init_hypergraph = ds::reindex(hg);
  std::vector<HypernodeID> mapping(std::move(extracted_init_hypergraph.second));

  double init_alpha = config.initial_partitioning.init_alpha;
  double best_imbalance = std::numeric_limits<double>::max();
  std::vector<PartitionID> best_imbalanced_partition(
    extracted_init_hypergraph.first->initialNumNodes(), 0);

  do {
    extracted_init_hypergraph.first->resetPartitioning();
    Configuration init_config = Partitioner::createConfigurationForInitialPartitioning(
      *extracted_init_hypergraph.first, config, init_alpha);


    if (config.initial_partitioning.verbose_output) {
      LOG << "Calling Initial Partitioner:" << toString(config.initial_partitioning.technique)
          << "" << toString(config.initial_partitioning.mode) << ""
          << toString(config.initial_partitioning.algo)
          << "(k=" << init_config.initial_partitioning.k << ", epsilon="
          << init_config.initial_partitioning.epsilon << ")";
    }
    if (config.initial_partitioning.technique == InitialPartitioningTechnique::flat &&
        config.initial_partitioning.mode == Mode::direct_kway) {
      // If the direct k-way flat initial partitioner is used we call the
      // corresponding initial partitioing algorithm, otherwise...
      std::unique_ptr<IInitialPartitioner> partitioner(
        InitialPartitioningFactory::getInstance().createObject(
          config.initial_partitioning.algo,
          *extracted_init_hypergraph.first, init_config));
      partitioner->partition(*extracted_init_hypergraph.first,
                             init_config);
    } else {
      // ... we call the partitioner again with the new configuration.
      performPartitioning(*extracted_init_hypergraph.first, init_config);
    }

    const double imbalance = metrics::imbalance(*extracted_init_hypergraph.first, config);
    if (imbalance < best_imbalance) {
      for (const HypernodeID& hn : extracted_init_hypergraph.first->nodes()) {
        best_imbalanced_partition[hn] = extracted_init_hypergraph.first->partID(hn);
      }
      best_imbalance = imbalance;
    }
    init_alpha -= 0.1;
  } while (metrics::imbalance(*extracted_init_hypergraph.first, config)
           > config.partition.epsilon && init_alpha > 0.0);

  ASSERT([&]() {
      for (const HypernodeID& hn : hg.nodes()) {
        if (hg.partID(hn) != -1) {
          return false;
        }
      }
      return true;
    } (), "The original hypergraph isn't unpartitioned!");

  // Apply the best balanced partition to the original hypergraph
  for (const HypernodeID& hn : extracted_init_hypergraph.first->nodes()) {
    PartitionID part = extracted_init_hypergraph.first->partID(hn);
    if (part != best_imbalanced_partition[hn]) {
      part = best_imbalanced_partition[hn];
    }
    hg.setNodePart(mapping[hn], part);
  }

  switch (config.partition.objective) {
    case Objective::cut:
      Stats::instance().addToTotal(config, "initialCut", metrics::hyperedgeCut(hg));
      break;
    case Objective::km1:
      Stats::instance().addToTotal(config, "initialKm1", metrics::km1(hg));
      break;
    default:
      // do nothing
      break;
  }
  Stats::instance().addToTotal(config, "initialImbalance", metrics::imbalance(hg, config));
}

inline Configuration Partitioner::createConfigurationForInitialPartitioning(const Hypergraph& hg,
                                                                            const Configuration&
                                                                            original_config,
                                                                            double init_alpha) const {
  Configuration config(original_config);

  config.type = ConfigType::initial_partitioning;

  if (!config.preprocessing.louvain_community_detection.enable_in_initial_partitioning) {
    config.preprocessing.enable_louvain_community_detection = false;
  }

  config.partition.epsilon = init_alpha * original_config.partition.epsilon;
  config.partition.collect_stats = false;
  config.partition.global_search_iterations = 0;

  config.initial_partitioning.k = config.partition.k;
  config.initial_partitioning.epsilon = init_alpha * original_config.partition.epsilon;

  config.initial_partitioning.perfect_balance_partition_weight.clear();
  config.initial_partitioning.upper_allowed_partition_weight.clear();
  for (int i = 0; i < config.initial_partitioning.k; ++i) {
    config.initial_partitioning.perfect_balance_partition_weight.push_back(
      config.partition.perfect_balance_part_weights[i % 2]);
    config.initial_partitioning.upper_allowed_partition_weight.push_back(
      config.initial_partitioning.perfect_balance_partition_weight[i]
      * (1.0 + config.partition.epsilon));
  }

  // Coarsening-Parameters
  config.coarsening = config.initial_partitioning.coarsening;

  // Refinement-Parameters
  config.local_search = config.initial_partitioning.local_search;

  // Hypergraph depending parameters
  config.partition.total_graph_weight = hg.totalWeight();
  config.coarsening.contraction_limit = config.coarsening.contraction_limit_multiplier
                                        * config.initial_partitioning.k;
  config.coarsening.hypernode_weight_fraction = config.coarsening.max_allowed_weight_multiplier
                                                / config.coarsening.contraction_limit;
  config.coarsening.max_allowed_node_weight = ceil(config.coarsening.hypernode_weight_fraction
                                                   * config.partition.total_graph_weight);

  // Reconfiguring the partitioner to act as an initial partitioner
  // on the next partition call using the new configuration
  // based on the initial partitioning settings provided by the
  // original_config.
  switch (original_config.initial_partitioning.technique) {
    case InitialPartitioningTechnique::multilevel:
      config.coarsening.algorithm = config.initial_partitioning.coarsening.algorithm;
      switch (original_config.initial_partitioning.mode) {
        case Mode::recursive_bisection:
          config.partition.mode = Mode::recursive_bisection;
          break;
        case Mode::direct_kway:
          // Currently a bad configuration (see KaHyPar.cc). The same behaviour as this
          // initial partitioning method is achieved, if we use a smaller contraction limit
          // in the main partitioner. But the currently used contraction limit is optimized in
          // several experiments => It makes no sense to further coarsen the hypergraph after
          // coarsening phase.
          config.partition.mode = Mode::direct_kway;
          break;
        default:
          LOG << "Invalid IP mode";
          std::exit(-1);
      }
      config.local_search.algorithm = config.initial_partitioning.local_search.algorithm;
      break;
    case InitialPartitioningTechnique::flat:
      // No more coarsening in this case. Since KaHyPar is designed to be an n-level partitioner,
      // we do not support flat partitioning explicitly. However we provide a coarsening
      // algorithm that doesn't do anything in order to "emulate" a flat partitioner
      // for initial partitioning. Since the initial partitioning uses a refinement
      // algorithm to improve the initial partition, we use the twoway_fm algorithm.
      // Since the coarsening algorithm does nothing, the twoway_fm algorithm in our "emulated"
      // flat partitioner do also nothing, since there is no uncoarsening phase in which
      // a local search algorithm could further improve the solution.
      config.coarsening.algorithm = CoarseningAlgorithm::do_nothing;
      config.local_search.algorithm = config.initial_partitioning.local_search.algorithm;
      switch (original_config.initial_partitioning.mode) {
        case Mode::recursive_bisection:
          config.partition.mode = Mode::recursive_bisection;
          break;
        case Mode::direct_kway:
          config.partition.mode = Mode::direct_kway;
          break;
        default:
          LOG << "Invalid IP mode";
          std::exit(-1);
      }
  }
  // We are now in initial partitioning mode, i.e. the next call to performInitialPartitioning
  // will actually trigger the computation of an initial partition of the hypergraph.
  // Computing an actual initial partition is always flat, since the graph has been coarsened
  // before in case of multilevel initial partitioning, or should not be coarsened in case
  // of flat initial partitioning. Furthermore we set the initial partitioning mode to
  // direct k-way by convention, since all initial partitioning algorithms work for arbitrary
  // values of k >=2. The only difference is whether or not we use 2-way FM refinement
  // or k-way FM refinement (this decision is based on the value of k).
  config.initial_partitioning.technique = InitialPartitioningTechnique::flat;
  config.initial_partitioning.mode = Mode::direct_kway;

  return config;
}

inline void Partitioner::partition(Hypergraph& hypergraph, Configuration& config) {
  configurePreprocessing(hypergraph, config);
  partitionInternal(hypergraph, config);
}


inline void Partitioner::partitionInternal(Hypergraph& hypergraph, Configuration& config) {
  setupConfig(hypergraph, config);
  if (config.type == ConfigType::main) {
    LOG << config;
    if (config.partition.verbose_output) {
      LOG << "\n********************************************************************************";
      LOG << "*                                    Input                                     *";
      LOG << "********************************************************************************";
      io::printHypergraphInfo(hypergraph, config.partition.graph_filename.substr(
                                config.partition.graph_filename.find_last_of('/') + 1));
    }
  }
  if (config.preprocessing.min_hash_sparsifier.is_active) {
    Hypergraph sparseHypergraph;
    preprocess(hypergraph, sparseHypergraph, config);
    performPartitioning(sparseHypergraph, config);
    postprocess(hypergraph, sparseHypergraph, config);
  } else {
    preprocess(hypergraph, config);
    performPartitioning(hypergraph, config);
    postprocess(hypergraph, config);
  }
}

inline void Partitioner::performPartitioning(Hypergraph& hypergraph, const Configuration& config) {
  switch (config.partition.mode) {
    case Mode::recursive_bisection:
      performRecursiveBisectionPartitioning(hypergraph, config);
      break;
    case Mode::direct_kway:
      performDirectKwayPartitioning(hypergraph, config);
      break;
  }
}


inline void Partitioner::performPartitioning(Hypergraph& hypergraph,
                                             ICoarsener& coarsener,
                                             IRefiner& refiner,
                                             const Configuration& config) {
  if (config.partition.verbose_output && config.type == ConfigType::main) {
    LOG << "********************************************************************************";
    LOG << "*                                Coarsening...                                 *";
    LOG << "********************************************************************************";
  }
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(config.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "Coarsening",
                               std::chrono::duration<double>(end - start).count());

  gatherCoarseningStats(config, hypergraph);

  if (config.partition.verbose_output && config.type == ConfigType::main) {
    io::printHypergraphInfo(hypergraph, "Coarsened Hypergraph");
  }
  if (config.type == ConfigType::main && (config.partition.verbose_output ||
                                          config.initial_partitioning.verbose_output)) {
    LOG << "\n********************************************************************************";
    LOG << "*                           Initial Partitioning...                            *";
    LOG << "********************************************************************************";
  } else if (config.type == ConfigType::initial_partitioning &&
             config.initial_partitioning.verbose_output) {
    LOG << "--------------------------------------------------------------------------------";
    LOG << "Computing bisection (" << config.partition.rb_lower_k << ".."
        << config.partition.rb_upper_k << ")";
    LOG << "--------------------------------------------------------------------------------";
  }
  start = std::chrono::high_resolution_clock::now();
  performInitialPartitioning(hypergraph, config);
  end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "InitialPartitioning",
                               std::chrono::duration<double>(end - start).count());

  hypergraph.initializeNumCutHyperedges();
  if (config.partition.verbose_output && config.type == ConfigType::main) {
    if (config.initial_partitioning.verbose_output) {
      LOG << "--------------------------------------------------------------------------------\n";
    }
    LOG << "Initial Partitioning Result:";
    LOG << "Initial" << toString(config.partition.objective) << "      ="
        << (config.partition.objective == Objective::cut ? metrics::hyperedgeCut(hypergraph) :
        metrics::km1(hypergraph));
    LOG << "Initial imbalance =" << metrics::imbalance(hypergraph, config);
    LOG << "Initial part sizes and weights:";
    io::printPartSizesAndWeights(hypergraph);
    LLOG << "Target weights:";
    if (config.partition.mode == Mode::direct_kway) {
      LLOG << "w(*) =" << config.partition.max_part_weights[0] << "\n";
    } else {
      LLOG << "(RB): w(0)=" << config.partition.max_part_weights[0]
           << "w(1)=" << config.partition.max_part_weights[1] << "\n";
    }
    LOG << "\n********************************************************************************";
    LOG << "*                               Local Search...                                *";
    LOG << "********************************************************************************";
  }
  start = std::chrono::high_resolution_clock::now();
  coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "UncoarseningRefinement",
                               std::chrono::duration<double>(end - start).count());
  if (config.partition.verbose_output && config.type == ConfigType::main) {
    LOG << "Local Search Result:";
    LOG << "Final" << toString(config.partition.objective) << "      ="
        << (config.partition.objective == Objective::cut ? metrics::hyperedgeCut(hypergraph) :
        metrics::km1(hypergraph));
    LOG << "Final imbalance =" << metrics::imbalance(hypergraph, config);
    LOG << "Final part sizes and weights:";
    io::printPartSizesAndWeights(hypergraph);
    LOG << "";
  }
}

inline bool Partitioner::partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener,
                                         IRefiner& refiner, const Configuration& config) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(config.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "VCycleCoarsening",
                               std::chrono::duration<double>(end - start).count());

  gatherCoarseningStats(config, hypergraph);

  hypergraph.initializeNumCutHyperedges();

  start = std::chrono::high_resolution_clock::now();
  const bool found_improved_cut = coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "VCycleUnCoarseningRefinement",
                               std::chrono::duration<double>(end - start).count());
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
  return std::min(std::pow(base, 1.0 / ceil(log2(static_cast<double>(k)))) - 1.0, 0.99);
}

inline Configuration Partitioner::createConfigurationForCurrentBisection(const Configuration& original_config,
                                                                         const Hypergraph& original_hypergraph,
                                                                         const Hypergraph& current_hypergraph,
                                                                         const PartitionID current_k,
                                                                         const PartitionID k0,
                                                                         const PartitionID k1) const {
  Configuration current_config(original_config);
  current_config.partition.k = 2;
  current_config.partition.epsilon = calculateRelaxedEpsilon(original_hypergraph.totalWeight(),
                                                             current_hypergraph.totalWeight(),
                                                             current_k, original_config);
  ASSERT(current_config.partition.epsilon > 0.0, "start partition already too imbalanced");
  current_config.partition.total_graph_weight =
    current_hypergraph.totalWeight();

  current_config.partition.perfect_balance_part_weights[0] =
    ceil((k0 / static_cast<double>(current_k))
         * static_cast<double>(current_config.partition.total_graph_weight));

  current_config.partition.perfect_balance_part_weights[1] =
    ceil((k1 / static_cast<double>(current_k))
         * static_cast<double>(current_config.partition.total_graph_weight));

  current_config.partition.max_part_weights[0] =
    (1 + current_config.partition.epsilon) * current_config.partition.perfect_balance_part_weights[0];

  current_config.partition.max_part_weights[1] =
    (1 + current_config.partition.epsilon) * current_config.partition.perfect_balance_part_weights[1];

  current_config.coarsening.contraction_limit =
    current_config.coarsening.contraction_limit_multiplier * current_config.partition.k;

  current_config.coarsening.hypernode_weight_fraction =
    current_config.coarsening.max_allowed_weight_multiplier
    / current_config.coarsening.contraction_limit;

  current_config.coarsening.max_allowed_node_weight = ceil(
    current_config.coarsening.hypernode_weight_fraction
    * current_config.partition.total_graph_weight);

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
                                RBHypergraphState::unpartitioned, 0,
                                (original_config.partition.k - 1));

  while (!hypergraph_stack.empty()) {
    Hypergraph& current_hypergraph = *hypergraph_stack.back().hypergraph;

    if (hypergraph_stack.back().lower_k == hypergraph_stack.back().upper_k) {
      for (const HypernodeID& hn : current_hypergraph.nodes()) {
        const HypernodeID original_hn = originalHypernode(hn, mapping_stack);
        const PartitionID current_part = input_hypergraph.partID(original_hn);
        ASSERT(current_part != Hypergraph::kInvalidPartition, V(current_part));
        if (current_part != hypergraph_stack.back().lower_k) {
          input_hypergraph.changeNodePart(original_hn, current_part,
                                          hypergraph_stack.back().lower_k);
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
          Configuration current_config =
            createConfigurationForCurrentBisection(original_config,
                                                   input_hypergraph, current_hypergraph, k, km,
                                                   k - km);
          current_config.partition.rb_lower_k = k1;
          current_config.partition.rb_upper_k = k2;

          std::unique_ptr<ICoarsener> coarsener(
            CoarsenerFactory::getInstance().createObject(
              current_config.coarsening.algorithm,
              current_hypergraph, current_config,
              current_hypergraph.weightOfHeaviestNode()));

          std::unique_ptr<IRefiner> refiner(
            RefinerFactory::getInstance().createObject(
              current_config.local_search.algorithm,
              current_hypergraph, current_config));

          ASSERT(coarsener.get() != nullptr, "coarsener not found");
          ASSERT(refiner.get() != nullptr, "refiner not found");

          // TODO(schlag): find better solution
          if (_internals.empty()) {
            _internals.append(coarsener->policyString() + " " + refiner->policyString());
          }

          // TODO(schlag): we could integrate v-cycles in a similar fashion as is
          // performDirectKwayPartitioning
          performPartitioning(current_hypergraph, *coarsener, *refiner, current_config);

          auto extractedHypergraph_1 = ds::extractPartAsUnpartitionedHypergraphForBisection(
            current_hypergraph, 1, current_config.partition.objective == Objective::km1 ? true : false);
          mapping_stack.emplace_back(std::move(extractedHypergraph_1.second));

          hypergraph_stack.back().state =
            RBHypergraphState::partitionedAndPart1Extracted;
          hypergraph_stack.emplace_back(HypergraphPtr(extractedHypergraph_1.first.release(),
                                                      delete_hypergraph),
                                        RBHypergraphState::unpartitioned, k1 + km, k2);
        }
        break;
      case RBHypergraphState::partitionedAndPart1Extracted: {
          auto extractedHypergraph_0 =
            ds::extractPartAsUnpartitionedHypergraphForBisection(
              current_hypergraph, 0, original_config.partition.objective == Objective::km1 ? true : false);
          mapping_stack.emplace_back(std::move(extractedHypergraph_0.second));
          hypergraph_stack.back().state = RBHypergraphState::finished;
          hypergraph_stack.emplace_back(HypergraphPtr(extractedHypergraph_0.first.release(),
                                                      delete_hypergraph),
                                        RBHypergraphState::unpartitioned, k1, k1 + km - 1);
        }
        break;
      default:
        LOG << "Illegal recursive bisection state";
        break;
    }
  }
}

inline void Partitioner::performDirectKwayPartitioning(Hypergraph& hypergraph,
                                                       const Configuration& config) {
  std::unique_ptr<ICoarsener> coarsener(
    CoarsenerFactory::getInstance().createObject(
      config.coarsening.algorithm, hypergraph, config,
      hypergraph.weightOfHeaviestNode()));

  std::unique_ptr<IRefiner> refiner(
    RefinerFactory::getInstance().createObject(
      config.local_search.algorithm, hypergraph, config));

  // TODO(schlag): find better solution
  _internals.append(coarsener->policyString() + " " + refiner->policyString());

  performPartitioning(hypergraph, *coarsener, *refiner, config);

  DBG << "PartitioningResult: cut=" << metrics::hyperedgeCut(hypergraph);
#ifndef NDEBUG
  HyperedgeWeight initial_cut = std::numeric_limits<HyperedgeWeight>::max();
#endif

  for (int vcycle = 1; vcycle <= config.partition.global_search_iterations; ++vcycle) {
    const bool found_improved_cut = partitionVCycle(hypergraph, *coarsener, *refiner, config);

    DBG << V(vcycle) << V(metrics::hyperedgeCut(hypergraph));
    if (!found_improved_cut) {
      LOG << "Cut could not be decreased in v-cycle" << vcycle << ". Stopping global search.";
      break;
    }

    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut,
           "Uncoarsening worsened cut:" << metrics::hyperedgeCut(hypergraph) << ">" << initial_cut);
#ifndef NDEBUG
    initial_cut = metrics::hyperedgeCut(hypergraph);
#endif
  }
}
}  // namespace kahypar
