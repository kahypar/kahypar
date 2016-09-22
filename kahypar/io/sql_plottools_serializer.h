/***************************************************************************
 *  Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <array>
#include <chrono>
#include <fstream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/git_revision.h"
#include "kahypar/partition/configuration.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/partitioner.h"

using partition::Configuration;
using partition::Partitioner;
using partition::toString;
using partition::RefinementAlgorithm;

namespace io {
namespace serializer {
static inline void serialize(const Configuration& config, const Hypergraph& hypergraph,
                             const Partitioner& partitioner,
                             const std::chrono::duration<double>& elapsed_seconds) {
  std::ostringstream oss;
  oss << "RESULT"
  << " graph=" << config.partition.graph_filename.substr(
    config.partition.graph_filename.find_last_of("/") + 1)
  << " numHNs=" << hypergraph.initialNumNodes()
  << " numHEs=" << hypergraph.initialNumEdges()
  << " " << hypergraph.typeAsString()
  << " mode=" << toString(config.partition.mode)
  << " objective=" << toString(config.partition.objective)
  << " k=" << config.partition.k
  << " epsilon=" << config.partition.epsilon
  << " seed=" << config.partition.seed
  << " num_v_cycles=" << config.partition.global_search_iterations
  << " he_size_threshold=" << config.partition.hyperedge_size_threshold
  << " total_graph_weight=" << config.partition.total_graph_weight
  << " L_opt0=" << config.partition.perfect_balance_part_weights[0]
  << " L_opt1=" << config.partition.perfect_balance_part_weights[1]
  << " L_max0=" << config.partition.max_part_weights[0]
  << " L_max1=" << config.partition.max_part_weights[1]
  << " pre_use_min_hash_sparsifier=" << std::boolalpha
  << config.preprocessing.use_min_hash_sparsifier
  << " pre_remove_parallel_hes=" << std::boolalpha
  << config.preprocessing.remove_parallel_hes
  << " pre_remove_always_cut_hes=" << std::boolalpha
  << config.preprocessing.remove_always_cut_hes
  << " coarsening_algo=" << toString(config.coarsening.algorithm)
  << " coarsening_max_allowed_weight_multiplier=" << config.coarsening.max_allowed_weight_multiplier
  << " coarsening_contraction_limit_multiplier=" << config.coarsening.contraction_limit_multiplier
  << " coarsening_hypernode_weight_fraction=" << config.coarsening.hypernode_weight_fraction
  << " coarsening_max_allowed_node_weight=" << config.coarsening.max_allowed_node_weight
  << " coarsening_contraction_limit=" << config.coarsening.contraction_limit
  << " IP_tool=" << toString(config.initial_partitioning.tool)
  << " IP_tool_path=" << config.initial_partitioning.tool_path
  << " IP_hmetis_ub_factor=" << config.initial_partitioning.hmetis_ub_factor
  << " IP_mode=" << toString(config.initial_partitioning.mode)
  << " IP_technique=" << toString(config.initial_partitioning.technique)
  << " IP_algorithm=" << toString(config.initial_partitioning.algo)
  << " IP_pool_type=" << config.initial_partitioning.pool_type
  << " IP_num_runs=" << config.initial_partitioning.nruns
  << " IP_coarsening_algo=" << toString(config.initial_partitioning.coarsening.algorithm)
  << " IP_coarsening_max_allowed_weight_multiplier="
  << config.initial_partitioning.coarsening.max_allowed_weight_multiplier
  << " IP_coarsening_contraction_limit_multiplier="
  << config.initial_partitioning.coarsening.contraction_limit_multiplier
  << " IP_local_search_algorithm=" << toString(config.initial_partitioning.local_search.algorithm)
  << " IP_local_search_iterations_per_level="
  << config.initial_partitioning.local_search.iterations_per_level;
  if (config.initial_partitioning.local_search.algorithm == RefinementAlgorithm::twoway_fm ||
      config.initial_partitioning.local_search.algorithm == RefinementAlgorithm::kway_fm ||
      config.initial_partitioning.local_search.algorithm == RefinementAlgorithm::kway_fm_km1) {
    oss << " IP_local_search_fm_stopping_rule="
    << toString(config.initial_partitioning.local_search.fm.stopping_rule)
    << " IP_local_search_fm_max_number_of_fruitless_moves="
    << config.initial_partitioning.local_search.fm.max_number_of_fruitless_moves
    << " IP_local_search_fm_global_rebalancing="
    << toString(config.initial_partitioning.local_search.fm.global_rebalancing)
    << " IP_local_search_fm_adaptive_stopping_alpha="
    << config.initial_partitioning.local_search.fm.adaptive_stopping_alpha;
  }
  if (config.initial_partitioning.local_search.algorithm == RefinementAlgorithm::label_propagation) {
    oss << " IP_local_search_sclap_max_number_iterations="
    << config.initial_partitioning.local_search.sclap.max_number_iterations;
  }

  oss << " local_search_algorithm=" << toString(config.local_search.algorithm)
  << " local_search_iterations_per_level=" << config.local_search.iterations_per_level;
  if (config.local_search.algorithm == RefinementAlgorithm::twoway_fm ||
      config.local_search.algorithm == RefinementAlgorithm::kway_fm ||
      config.local_search.algorithm == RefinementAlgorithm::kway_fm_km1) {
    oss << " local_search_fm_stopping_rule=" << toString(config.local_search.fm.stopping_rule)
    << " local_search_fm_max_number_of_fruitless_moves="
    << config.local_search.fm.max_number_of_fruitless_moves
    << " local_search_fm_global_rebalancing=" << toString(config.local_search.fm.global_rebalancing)
    << " local_search_fm_adaptive_stopping_alpha=" << config.local_search.fm.adaptive_stopping_alpha;
  }
  if (config.local_search.algorithm == RefinementAlgorithm::label_propagation) {
    oss << " local_search_sclap_max_number_iterations="
    << config.local_search.sclap.max_number_iterations;
  }
  oss << partitioner.internals();
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    oss << " partSize" << i << "=" << hypergraph.partSize(i);
  }
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    oss << " partWeight" << i << "=" << hypergraph.partWeight(i);
  }
  oss << " cut=" << metrics::hyperedgeCut(hypergraph)
  << " soed=" << metrics::soed(hypergraph)
  << " km1=" << metrics::km1(hypergraph)
  << " absorption=" << metrics::absorption(hypergraph)
  << " imbalance=" << metrics::imbalance(hypergraph, config)
  << " totalPartitionTime=" << elapsed_seconds.count()
  << " initialParallelHEremovalTime=" << Stats::instance().get("InitialParallelHEremoval")
  << " initialLargeHEremovalTime=" << Stats::instance().get("InitialLargeHEremoval")
  << " coarseningTime=" << Stats::instance().get("Coarsening")
  << " initialPartitionTime=" << Stats::instance().get("InitialPartitioning")
  << " uncoarseningRefinementTime=" << Stats::instance().get("UncoarseningRefinement")
  << " initialParallelHErestoreTime=" << Stats::instance().get("InitialParallelHErestore")
  << " initialLargeHErestoreTime=" << Stats::instance().get("InitialLargeHErestore")
  << Stats::instance().toString()
  << " git=" << STR(KaHyPar_BUILD_VERSION)
  << std::endl;

  std::cout << oss.str() << std::endl;
}
}  // namespace serializer
}  // namespace io
