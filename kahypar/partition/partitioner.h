/***************************************************************************
 *  Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

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

#include "definitions.h"
#include "io/hypergraph_io.h"
#include "io/partitioning_output.h"
#include "partition/coarsening/hypergraph_pruner.h"
#include "partition/coarsening/i_coarsener.h"
#include "partition/configuration.h"
#include "partition/factories.h"
#include "partition/metrics.h"
#include "partition/refinement/2way_fm_refiner.h"
#include "preprocessing/large_hyperedge_remover.h"
#include "preprocessing/single_node_hyperedge_remover.h"
#include "utils/randomize.h"
#include "utils/stats.h"

using datastructure::extractPartAsUnpartitionedHypergraphForBisection;
using datastructure::reindex;
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
static const bool dbg_partition_initial_partitioning = true;
static const bool dbg_partition_vcycles = true;

class Partitioner {
  using CoarsenedToHmetisMapping = std::unordered_map<HypernodeID, HypernodeID>;
  using HmetisToCoarsenedMapping = std::vector<HypernodeID>;
  using PartitionWeights = std::vector<HypernodeWeight>;
  using Hyperedges = std::vector<HyperedgeID>;
  using HypergraphPtr = std::unique_ptr<Hypergraph, void (*)(Hypergraph*)>;
  using MappingStack = std::vector<std::vector<HypernodeID> >;

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
    RBState(HypergraphPtr h, RBHypergraphState s, const PartitionID lk,
            const PartitionID uk) :
      hypergraph(std::move(h)),
      state(s),
      lower_k(lk),
      upper_k(uk) { }
  };

 public:
  explicit Partitioner() :
    _single_node_he_remover(),
    _large_he_remover(),
    _internals() { }

  Partitioner(const Partitioner&) = delete;
  Partitioner& operator= (const Partitioner&) = delete;

  Partitioner(Partitioner&&) = delete;
  Partitioner& operator= (Partitioner&&) = delete;

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
  FRIEND_TEST(APartitioner, UseshMetisPartitioningOnCoarsestHypergraph);
  FRIEND_TEST(APartitioner, UncoarsensTheInitiallyPartitionedHypergraph);
  FRIEND_TEST(APartitioner, CalculatesPinCountsOfAHyperedgesAfterInitialPartitioning);
  FRIEND_TEST(APartitioner, CanUseVcyclesAsGlobalSearchStrategy);
  friend class io::APartitionOfAHypergraph_IsCorrectlyWrittenToFile_Test;
  friend class metrics::APartitionedHypergraph;

  inline void performDirectKwayPartitioning(Hypergraph& hypergraph,
                                            const Configuration& config);

  inline void performRecursiveBisectionPartitioning(Hypergraph& hypergraph,
                                                    const Configuration& config);

  inline void createMappingsForInitialPartitioning(HmetisToCoarsenedMapping& hmetis_to_hg,
                                                   CoarsenedToHmetisMapping& hg_to_hmetis,
                                                   const Hypergraph& hg);
  void performInitialPartitioning(Hypergraph& hg, const Configuration& config);

  inline void initialPartitioningViaExternalTools(Hypergraph& hg, const Configuration& config);
  inline void initialPartitioningViaKaHyPar(Hypergraph& hg, const Configuration& config);

  inline void partition(Hypergraph& hypergraph, ICoarsener& coarsener, IRefiner& refiner,
                        const Configuration& config, const PartitionID k1, const PartitionID k2);

  inline bool partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener, IRefiner& refiner,
                              const Configuration& config, const int vcycle, const PartitionID k1,
                              const PartitionID k2);
  inline HypernodeID originalHypernode(const HypernodeID hn,
                                       const MappingStack& mapping_stack) const;
  inline double calculateRelaxedEpsilon(const HypernodeWeight original_hypergraph_weight,
                                        const HypernodeWeight current_hypergraph_weight,
                                        const PartitionID k,
                                        const Configuration& original_config) const;

  inline Configuration createConfigurationForCurrentBisection(const Configuration& original_config,
                                                              const Hypergraph& original_hypergraph,
                                                              const Hypergraph& current_hypergraph,
                                                              const PartitionID current_k,
                                                              const PartitionID k0,
                                                              const PartitionID k1) const;


  inline Configuration createConfigurationForInitialPartitioning(const Hypergraph& hg,
                                                                 const Configuration& original_config,
                                                                 double init_alpha) const;

  inline void partitionInternal(Hypergraph& hypergraph, const Configuration& config);

  inline void setupConfig(const Hypergraph& hypergraph, Configuration& config) const;

  inline void preprocess(Hypergraph& hypergraph, const Configuration& config);

  inline void postprocess(Hypergraph& hypergraph, const Configuration& config);

  SingleNodeHyperedgeRemover _single_node_he_remover;
  LargeHyperedgeRemover _large_he_remover;
  std::string _internals;
};

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
  config.local_search.fm.adaptive_stopping_beta = log(hypergraph.initialNumNodes());

// We use hMetis-RB as initial partitioner. If called to partition a graph into k parts
// with an UBfactor of b, the maximal allowed partition size will be 0.5+(b/100)^(log2(k)) n.
// In order to provide a balanced initial partitioning, we determine the UBfactor such that
// the maximal allowed partiton size corresponds to our upper bound i.e.
// (1+epsilon) * ceil(total_weight / k).
  double exp = 1.0 / log2(config.partition.k);
  config.initial_partitioning.hmetis_ub_factor =
    50.0
    * (2 * pow((1 + config.partition.epsilon), exp)
       * pow(
         ceil(
           static_cast<double>(config.partition.total_graph_weight)
           / config.partition.k)
         / config.partition.total_graph_weight,
         exp) - 1);

  // the main partitioner should track stats
  config.partition.collect_stats = true;


  io::printPartitionerConfiguration(config);
}

inline void Partitioner::preprocess(Hypergraph& hypergraph, const Configuration& config) {
  const auto result = _single_node_he_remover.removeSingleNodeHyperedges(hypergraph);
  if (config.partition.verbose_output && result.num_removed_single_node_hes > 0) {
    LOG("\033[1m\033[31m" << "Removed " << result.num_removed_single_node_hes
        << " hyperedges with |e|=1" << "\033[0m");
    LOG("\033[1m\033[31m" << "===> " << result.num_unconnected_hns
        << " unconnected HNs could have been removed" << "\033[0m");
  }

  if (config.preprocessing.remove_always_cut_hes) {
    const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    _large_he_remover.removeLargeHyperedges(hypergraph, config);
    const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    Stats::instance().addToTotal(config, "InitialLargeHEremoval",
                                 std::chrono::duration<double>(end - start).count());
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

inline void Partitioner::performInitialPartitioning(Hypergraph& hg, const Configuration& config) {
  if (config.partition.verbose_output) {
    io::printHypergraphInfo(hg,
                            config.partition.coarse_graph_filename.substr(
                              config.partition.coarse_graph_filename.find_last_of("/")
                              + 1));
  }
  if (config.initial_partitioning.tool == InitialPartitioner::hMetis ||
      config.initial_partitioning.tool == InitialPartitioner::PaToH) {
    initialPartitioningViaExternalTools(hg, config);
  } else if (config.initial_partitioning.tool == InitialPartitioner::KaHyPar) {
    initialPartitioningViaKaHyPar(hg, config);
  }
  Stats::instance().addToTotal(config, "InitialCut", metrics::hyperedgeCut(hg));
}

inline Configuration Partitioner::createConfigurationForInitialPartitioning(const Hypergraph& hg,
                                                                            const Configuration&
                                                                            original_config,
                                                                            double init_alpha) const {
  Configuration config(original_config);

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
  config.local_search.fm.adaptive_stopping_beta = log(hg.currentNumNodes());

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
  setupConfig(hypergraph, config);
  preprocess(hypergraph, config);
  partitionInternal(hypergraph, config);
  postprocess(hypergraph, config);
}

inline void Partitioner::partitionInternal(Hypergraph& hypergraph, const Configuration& config) {
  switch (config.partition.mode) {
    case Mode::recursive_bisection:
      performRecursiveBisectionPartitioning(hypergraph, config);
      break;
    case Mode::direct_kway:
      performDirectKwayPartitioning(hypergraph, config);
      break;
  }
}


inline void Partitioner::partition(Hypergraph& hypergraph, ICoarsener& coarsener, IRefiner& refiner,
                                   const Configuration& config, const PartitionID k1,
                                   const PartitionID k2) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(config.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "Coarsening",
                               std::chrono::duration<double>(end - start).count());

  utils::gatherCoarseningStats(hypergraph, 0, k1, k2);

  // hypergraph.printGraphState();

  start = std::chrono::high_resolution_clock::now();
  performInitialPartitioning(hypergraph, config);
  end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "InitialPartitioning",
                               std::chrono::duration<double>(end - start).count());

  hypergraph.initializeNumCutHyperedges();

  start = std::chrono::high_resolution_clock::now();
  coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "UncoarseningRefinement",
                               std::chrono::duration<double>(end - start).count());
}

inline bool Partitioner::partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener,
                                         IRefiner& refiner, const Configuration& config,
                                         const int vcycle, const PartitionID k1,
                                         const PartitionID k2) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(config.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  Stats::instance().addToTotal(config, "VCycleCoarsening",
                               std::chrono::duration<double>(end - start).count());

  utils::gatherCoarseningStats(hypergraph, vcycle, k1, k2);

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
  return std::pow(base, 1.0 / ceil(log2(static_cast<double>(k)))) - 1.0;
}

void Partitioner::initialPartitioningViaExternalTools(Hypergraph& hg, const Configuration& config) {
  std::uniform_int_distribution<int> int_dist;
  std::mt19937 generator(config.partition.seed);

  DBG(dbg_partition_initial_partitioning, "# unconnected hypernodes = " << std::to_string([&]() {
      HypernodeID count = 0;
      for (const HypernodeID hn : hg.nodes()) {
        if (hg.nodeDegree(hn) == 0) {
          ++count;
        }
      }
      return count;
    } ()));

  HmetisToCoarsenedMapping hmetis_to_hg(hg.currentNumNodes(), 0);
  CoarsenedToHmetisMapping hg_to_hmetis;
  createMappingsForInitialPartitioning(hmetis_to_hg, hg_to_hmetis, hg);

  switch (config.initial_partitioning.tool) {
    case InitialPartitioner::hMetis:
      io::writeHypergraphForhMetisPartitioning(hg,
                                               config.partition.coarse_graph_filename, hg_to_hmetis);
      break;
    case InitialPartitioner::PaToH:
      io::writeHypergraphForPaToHPartitioning(hg,
                                              config.partition.coarse_graph_filename, hg_to_hmetis);
      break;
    case InitialPartitioner::KaHyPar:
      break;
  }

  std::vector<PartitionID> partitioning;
  std::vector<PartitionID> best_partitioning;
  partitioning.reserve(hg.currentNumNodes());
  best_partitioning.reserve(hg.currentNumNodes());

  HyperedgeWeight best_cut = std::numeric_limits<HyperedgeWeight>::max();
  HyperedgeWeight current_cut =
    std::numeric_limits<HyperedgeWeight>::max();

  for (int attempt = 0; attempt < config.initial_partitioning.nruns; ++attempt) {
    int seed = int_dist(generator);
    std::string initial_partitioner_call;
    switch (config.initial_partitioning.tool) {
      case InitialPartitioner::hMetis:
        initial_partitioner_call = config.initial_partitioning.tool_path + " "
                                   + config.partition.coarse_graph_filename + " "
                                   + std::to_string(config.partition.k) + " -seed="
                                   + std::to_string(seed) + " -ufactor="
                                   + std::to_string(
          config.initial_partitioning.hmetis_ub_factor
          < 0.1 ?
          0.1 :
          config.initial_partitioning.hmetis_ub_factor)
                                   + (config.partition.verbose_output ?
                                      "" : " > /dev/null");
        break;
      case InitialPartitioner::PaToH:
        initial_partitioner_call = config.initial_partitioning.tool_path + " "
                                   + config.partition.coarse_graph_filename + " "
                                   + std::to_string(config.partition.k) + " SD="
                                   + std::to_string(seed) + " FI="
                                   + std::to_string(config.partition.epsilon)
                                   + " PQ=Q"    // quality preset
                                   + " UM=U"    // net-cut metric
                                   + " WI=1"    // write partition info
                                   + " BO=C"    // balance on cell weights
                                   + (config.partition.verbose_output ?
                                      " OD=2" : " > /dev/null");
        break;
      case InitialPartitioner::KaHyPar:
        break;
    }

    LOG(initial_partitioner_call);
    LOGVAR(config.initial_partitioning.hmetis_ub_factor);
    std::system(initial_partitioner_call.c_str());

    io::readPartitionFile(config.partition.coarse_graph_partition_filename, partitioning);
    ASSERT(partitioning.size() == hg.currentNumNodes(), "Partition file has incorrect size");

    current_cut = metrics::hyperedgeCut(hg, hg_to_hmetis, partitioning);
    DBG(dbg_partition_initial_partitioning,
        "attempt " << attempt << " seed(" << seed << "):" << current_cut
        << " - balance=" << metrics::imbalance(hg, hg_to_hmetis, partitioning, config));
    Stats::instance().add(config, "initialCut_" + std::to_string(attempt), current_cut);

    if (current_cut < best_cut) {
      DBG(dbg_partition_initial_partitioning,
          "Attempt " << attempt << " improved initial cut from " << best_cut << " to " << current_cut);
      best_partitioning.swap(partitioning);
      best_cut = current_cut;
    }
    partitioning.clear();
  }

  ASSERT(best_cut != std::numeric_limits<HyperedgeWeight>::max(),
         "No min cut calculated");
  for (size_t i = 0; i < best_partitioning.size(); ++i) {
    hg.setNodePart(hmetis_to_hg[i], best_partitioning[i]);
  }
  ASSERT(metrics::hyperedgeCut(hg) == best_cut,
         "Cut induced by hypergraph does not equal " << "best initial cut");
}

inline void Partitioner::initialPartitioningViaKaHyPar(Hypergraph& hg,
                                                       const Configuration& config) {
  std::uniform_int_distribution<int> int_dist;
  auto extracted_init_hypergraph = reindex(hg);
  std::vector<HypernodeID> mapping(std::move(extracted_init_hypergraph.second));

  double init_alpha = config.initial_partitioning.init_alpha;
  double best_imbalance = std::numeric_limits<double>::max();
  std::vector<PartitionID> best_imbalanced_partition(
    extracted_init_hypergraph.first->initialNumNodes(), 0);

  do {
    extracted_init_hypergraph.first->resetPartitioning();
    Configuration init_config = Partitioner::createConfigurationForInitialPartitioning(
      *extracted_init_hypergraph.first, config, init_alpha);


    if (config.partition.verbose_output) {
      LOG("Calling Initial Partitioner: " << toString(config.initial_partitioning.technique)
          << " " << toString(config.initial_partitioning.mode) << " "
          << toString(config.initial_partitioning.algo)
          << " (k=" << init_config.initial_partitioning.k << ", epsilon="
          << init_config.initial_partitioning.epsilon << ")");
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
      partitionInternal(*extracted_init_hypergraph.first, init_config);
    }

    const double imbalance = metrics::imbalance(*extracted_init_hypergraph.first, config);
    if (imbalance < best_imbalance) {
      for (const HypernodeID hn : extracted_init_hypergraph.first->nodes()) {
        best_imbalanced_partition[hn] = extracted_init_hypergraph.first->partID(hn);
      }
    }
    init_alpha -= 0.1;
  } while (metrics::imbalance(*extracted_init_hypergraph.first, config)
           > config.partition.epsilon && init_alpha > 0.0);

  ASSERT([&]() {
      for (const HypernodeID hn : hg.nodes()) {
        if (hg.partID(hn) != -1) {
          return false;
        }
      }
      return true;
    } (), "The original hypergraph isn't unpartitioned!");

  // Apply the best balanced partition to the original hypergraph
  for (const HypernodeID hn : extracted_init_hypergraph.first->nodes()) {
    PartitionID part = extracted_init_hypergraph.first->partID(hn);
    if (part != best_imbalanced_partition[hn]) {
      part = best_imbalanced_partition[hn];
    }
    hg.setNodePart(mapping[hn], part);
  }
}

inline Configuration Partitioner::createConfigurationForCurrentBisection(const Configuration& original_config, const Hypergraph& original_hypergraph,
                                                                         const Hypergraph& current_hypergraph, const PartitionID current_k,
                                                                         const PartitionID k0, const PartitionID k1) const {
  Configuration current_config(original_config);
  current_config.partition.k = 2;
  current_config.partition.epsilon = calculateRelaxedEpsilon(
    original_hypergraph.totalWeight(), current_hypergraph.totalWeight(),
    current_k, original_config);
  ASSERT(current_config.partition.epsilon > 0.0, "start partition already too imbalanced");
  if (current_config.partition.verbose_output) {
    LOG(V(current_config.partition.epsilon));
  }
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

  current_config.local_search.fm.adaptive_stopping_beta = log(current_hypergraph.currentNumNodes());

  current_config.initial_partitioning.hmetis_ub_factor =
    100.0
    * ((1 + current_config.partition.epsilon)
       * (ceil(
            static_cast<double>(current_config.partition.total_graph_weight)
            / current_config.partition.k)
          / current_config.partition.total_graph_weight)
       - 0.5);

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
                                RBHypergraphState::unpartitioned, 0,
                                (original_config.partition.k - 1));

  while (!hypergraph_stack.empty()) {
    Hypergraph& current_hypergraph = *hypergraph_stack.back().hypergraph;

    if (hypergraph_stack.back().lower_k == hypergraph_stack.back().upper_k) {
      for (const HypernodeID hn : current_hypergraph.nodes()) {
        const HypernodeID original_hn = originalHypernode(hn,
                                                          mapping_stack);
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
            _internals.append(
              coarsener->policyString() + " "
              + refiner->policyString());
          }

          if (current_config.partition.verbose_output) {
            io::printHypergraphInfo(current_hypergraph, "---");
          }

          // TODO(schlag): we could integrate v-cycles in a similar fashion as is
          // performDirectKwayPartitioning
          partition(current_hypergraph, *coarsener, *refiner, current_config, k1, k2);

          if (current_config.partition.verbose_output) {
            LOG("-------------------------------------------------------------");
          }

          auto extractedHypergraph_1 = extractPartAsUnpartitionedHypergraphForBisection(
            current_hypergraph, 1, current_config.partition.objective == Objective::km1 ? true : false);
          mapping_stack.emplace_back(std::move(extractedHypergraph_1.second));

          hypergraph_stack.back().state =
            RBHypergraphState::partitionedAndPart1Extracted;
          hypergraph_stack.emplace_back(
            HypergraphPtr(extractedHypergraph_1.first.release(),
                          delete_hypergraph),
            RBHypergraphState::unpartitioned, k1 + km, k2);
        }
        break;
      case RBHypergraphState::partitionedAndPart1Extracted: {
          auto extractedHypergraph_0 =
            extractPartAsUnpartitionedHypergraphForBisection(
              current_hypergraph, 0, original_config.partition.objective == Objective::km1 ? true : false);
          mapping_stack.emplace_back(std::move(extractedHypergraph_0.second));
          hypergraph_stack.back().state = RBHypergraphState::finished;
          hypergraph_stack.emplace_back(
            HypergraphPtr(extractedHypergraph_0.first.release(),
                          delete_hypergraph),
            RBHypergraphState::unpartitioned, k1, k1 + km - 1);
        }
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

  partition(hypergraph, *coarsener, *refiner, config, 0, (config.partition.k - 1));

  DBG(dbg_partition_vcycles,
      "PartitioningResult: cut=" << metrics::hyperedgeCut(hypergraph));
#ifndef NDEBUG
  HyperedgeWeight initial_cut = std::numeric_limits<HyperedgeWeight>::max();
#endif

  for (int vcycle = 1; vcycle <= config.partition.global_search_iterations;
       ++vcycle) {
    const bool found_improved_cut = partitionVCycle(hypergraph, *coarsener,
                                                    *refiner, config, vcycle, 0, (config.partition.k - 1));

    DBG(dbg_partition_vcycles,
        "vcycle # " << vcycle << ": cut=" << metrics::hyperedgeCut(hypergraph));
    if (!found_improved_cut) {
      LOG("Cut could not be decreased in v-cycle " << vcycle << ". Stopping global search.");
      break;
    }

    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut,
           "Uncoarsening worsened cut:" << metrics::hyperedgeCut(hypergraph) << ">" << initial_cut);
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
}  // namespace partition
