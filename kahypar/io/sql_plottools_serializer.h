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

#include <array>
#include <chrono>
#include <fstream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/git_revision.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/partitioner.h"

namespace kahypar {
namespace io {
namespace serializer {
static inline void serialize(const Context& context, const Hypergraph& hypergraph,
                             const std::chrono::duration<double>& elapsed_seconds) {
  const auto& timings = Timer::instance().result();
  std::ostringstream oss;
  oss << "RESULT"
      << " graph=" << context.partition.graph_filename.substr(
    context.partition.graph_filename.find_last_of('/') + 1)
      << " numHNs=" << hypergraph.initialNumNodes()
      << " numHEs=" << hypergraph.initialNumEdges()
      << " " << hypergraph.typeAsString()
      << " mode=" << context.partition.mode
      << " objective=" << context.partition.objective
      << " k=" << context.partition.k
      << " epsilon=" << context.partition.epsilon
      << " seed=" << context.partition.seed
      << " num_v_cycles=" << context.partition.global_search_iterations
      << " he_size_threshold=" << context.partition.hyperedge_size_threshold
      << " total_graph_weight=" << context.partition.total_graph_weight
      << " L_opt0=" << context.partition.perfect_balance_part_weights[0]
      << " L_opt1=" << context.partition.perfect_balance_part_weights[1]
      << " L_max0=" << context.partition.max_part_weights[0]
      << " L_max1=" << context.partition.max_part_weights[1]
      << " pre_enable_min_hash_sparsifier=" << std::boolalpha
      << context.preprocessing.enable_min_hash_sparsifier
      << " pre_min_hash_max_hyperedge_size="
      << context.preprocessing.min_hash_sparsifier.max_hyperedge_size
      << " pre_min_hash_max_cluster_size="
      << context.preprocessing.min_hash_sparsifier.max_cluster_size
      << " pre_min_hash_min_cluster_size="
      << context.preprocessing.min_hash_sparsifier.min_cluster_size
      << " pre_min_hash_num_hash_functions="
      << context.preprocessing.min_hash_sparsifier.num_hash_functions
      << " pre_min_hash_combined_num_hash_functions="
      << context.preprocessing.min_hash_sparsifier.combined_num_hash_functions
      << " pre_min_sparsifier_is_active="
      << context.preprocessing.min_hash_sparsifier.is_active
      << " pre_min_sparsifier_activation_median_he_size="
      << context.preprocessing.min_hash_sparsifier.min_median_he_size
      << " enable_community_detection=" << std::boolalpha
      << context.preprocessing.enable_community_detection
      << " enable_louvain_in_initial_partitioning=" << std::boolalpha
      << context.preprocessing.community_detection.enable_in_initial_partitioning
      << " max_louvain_pass_iterations="
      << context.preprocessing.community_detection.max_pass_iterations
      << " min_louvain_eps_improvement="
      << context.preprocessing.community_detection.min_eps_improvement
      << " louvain_edge_weight=" << context.preprocessing.community_detection.edge_weight
      << " reuse_community_structure=" << std::boolalpha
      << context.preprocessing.community_detection.reuse_communities
      << " coarsening_algo=" << context.coarsening.algorithm
      << " coarsening_max_allowed_weight_multiplier="
      << context.coarsening.max_allowed_weight_multiplier
      << " coarsening_contraction_limit_multiplier="
      << context.coarsening.contraction_limit_multiplier
      << " coarsening_hypernode_weight_fraction=" << context.coarsening.hypernode_weight_fraction
      << " coarsening_max_allowed_node_weight=" << context.coarsening.max_allowed_node_weight
      << " coarsening_contraction_limit=" << context.coarsening.contraction_limit
      << " coarsening_rating_function=" << context.coarsening.rating.rating_function
      << " coarsening_rating_use_communities="
      << context.coarsening.rating.community_policy
      << " coarsening_rating_heavy_node_penalty="
      << context.coarsening.rating.heavy_node_penalty_policy
      << " coarsening_rating_acceptance_policy="
      << context.coarsening.rating.acceptance_policy
      << " IP_mode=" << context.initial_partitioning.mode
      << " IP_technique=" << context.initial_partitioning.technique
      << " IP_algorithm=" << context.initial_partitioning.algo
      << " IP_pool_type=" << context.initial_partitioning.pool_type
      << " IP_num_runs=" << context.initial_partitioning.nruns
      << " IP_coarsening_algo=" << context.initial_partitioning.coarsening.algorithm
      << " IP_coarsening_max_allowed_weight_multiplier="
      << context.initial_partitioning.coarsening.max_allowed_weight_multiplier
      << " IP_coarsening_contraction_limit_multiplier="
      << context.initial_partitioning.coarsening.contraction_limit_multiplier
      << " IP_coarsening_rating_function="
      << context.initial_partitioning.coarsening.rating.rating_function
      << " IP_coarsening_rating_use_communities="
      << context.initial_partitioning.coarsening.rating.community_policy
      << " IP_coarsening_rating_heavy_node_penalty="
      << context.initial_partitioning.coarsening.rating.heavy_node_penalty_policy
      << " IP_coarsening_rating_acceptance_policy="
      << context.initial_partitioning.coarsening.rating.acceptance_policy
      << " IP_local_search_algorithm="
      << context.initial_partitioning.local_search.algorithm
      << " IP_local_search_iterations_per_level="
      << context.initial_partitioning.local_search.iterations_per_level;
  if (context.initial_partitioning.local_search.algorithm == RefinementAlgorithm::twoway_fm ||
      context.initial_partitioning.local_search.algorithm == RefinementAlgorithm::kway_fm ||
      context.initial_partitioning.local_search.algorithm == RefinementAlgorithm::kway_fm_km1) {
    oss << " IP_local_search_fm_stopping_rule="
        << context.initial_partitioning.local_search.fm.stopping_rule
        << " IP_local_search_fm_max_number_of_fruitless_moves="
        << context.initial_partitioning.local_search.fm.max_number_of_fruitless_moves
        << " IP_local_search_fm_adaptive_stopping_alpha="
        << context.initial_partitioning.local_search.fm.adaptive_stopping_alpha;
  }
  if (context.initial_partitioning.local_search.algorithm ==
      RefinementAlgorithm::label_propagation) {
    oss << " IP_local_search_sclap_max_number_iterations="
        << context.initial_partitioning.local_search.sclap.max_number_iterations;
  }

  oss << " local_search_algorithm=" << context.local_search.algorithm
      << " local_search_iterations_per_level=" << context.local_search.iterations_per_level;
  if (context.local_search.algorithm == RefinementAlgorithm::twoway_fm ||
      context.local_search.algorithm == RefinementAlgorithm::kway_fm ||
      context.local_search.algorithm == RefinementAlgorithm::kway_fm_km1) {
    oss << " local_search_fm_stopping_rule=" << context.local_search.fm.stopping_rule
        << " local_search_fm_max_number_of_fruitless_moves="
        << context.local_search.fm.max_number_of_fruitless_moves
        << " local_search_fm_adaptive_stopping_alpha="
        << context.local_search.fm.adaptive_stopping_alpha;
  }
  if (context.local_search.algorithm == RefinementAlgorithm::label_propagation) {
    oss << " local_search_sclap_max_number_iterations="
        << context.local_search.sclap.max_number_iterations;
  }
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
      << " imbalance=" << metrics::imbalance(hypergraph, context)
      << " totalPartitionTime=" << elapsed_seconds.count()
      << " minHashSparsifierTime=" << timings.pre_sparsifier
      << " communityDetectionTime=" << timings.pre_community_detection
      << " coarseningTime=" << timings.total_coarsening
      << " initialPartitionTime=" << timings.total_initial_partitioning
      << " uncoarseningRefinementTime=" << timings.total_local_search
      << " postMinHashSparsifierTime=" << timings.post_sparsifier_restore;

  if (context.partition.global_search_iterations > 0) {
    int i = 1;
    for (const auto& timing : timings.v_cycle_coarsening) {
      oss << " vcycle" << i << "_coarsening=" << timing;
    }
    i = 1;
    for (const auto& timing : timings.v_cycle_local_search) {
      oss << " vcycle" << i << "_local_search=" << timing;
    }
  }

  oss << " " << context.stats.serialize().str()
      << " git=" << STR(KaHyPar_BUILD_VERSION)
      << std::endl;

  std::cout << oss.str() << std::endl;
}
}  // namespace serializer
}  // namespace io
}  // namespace kahypar
