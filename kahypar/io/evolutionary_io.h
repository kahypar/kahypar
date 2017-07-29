/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
namespace kahypar {
  namespace io {

    void readInBisectionContext(Context& context) {
      // general
      context.partition.mode=kahypar::modeFromString("recursive");
      context.partition.objective=Objective::cut;
      context.partition.hyperedge_size_threshold = std::numeric_limits<HyperedgeID>::max();
      context.partition.global_search_iterations = 0;
      // main -> coarsening
      context.coarsening.algorithm = kahypar::coarseningAlgorithmFromString("heavy_lazy");
      context.coarsening.max_allowed_weight_multiplier = 3.25;
      context.coarsening.contraction_limit_multiplier = 160;
      // main -> coarsening -> rating
      context.coarsening.rating.rating_function =
        kahypar::ratingFunctionFromString("heavy_edge");
      context.coarsening.rating.community_policy = CommunityPolicy::use_communities;
      context.coarsening.rating.heavy_node_penalty_policy =
        kahypar::heavyNodePenaltyFromString("multiplicative");
context.coarsening.rating.acceptance_policy =
        kahypar::acceptanceCriterionFromString("best");
      // main -> initial partitioning
      //i-type=KaHyPar
      context.initial_partitioning.mode = kahypar::modeFromString("direct");
      context.initial_partitioning.technique =
        kahypar::inititalPartitioningTechniqueFromString("flat");
      // initial partitioning -> initial partitioning
      context.initial_partitioning.algo =
        kahypar::initialPartitioningAlgorithmFromString("pool");
      context.initial_partitioning.nruns = 20;
      // main -> local search
      context.local_search.algorithm = kahypar::refinementAlgorithmFromString("twoway_fm");
      context.local_search.iterations_per_level = std::numeric_limits<int>::max();
      context.local_search.fm.stopping_rule = kahypar::stoppingRuleFromString("simple");
      context.local_search.fm.max_number_of_fruitless_moves = 350;
    }
    void readInDirectKwayContext(Context& context) {
      context.partition.mode=kahypar::modeFromString("direct");
      context.partition.objective=Objective::km1;
      context.partition.hyperedge_size_threshold = 1000;
      context.partition.global_search_iterations = 0;
      // main -> preprocessing -> min hash sparsifier
      context.preprocessing.enable_min_hash_sparsifier = true;
      context.preprocessing.min_hash_sparsifier.min_median_he_size=28;
      context.preprocessing.min_hash_sparsifier.max_hyperedge_size=1200;
      context.preprocessing.min_hash_sparsifier.max_cluster_size=10;
      context.preprocessing.min_hash_sparsifier.min_cluster_size=2;
      context.preprocessing.min_hash_sparsifier.combined_num_hash_functions=5;
      context.preprocessing.min_hash_sparsifier.combined_num_hash_functions=100;
      //p-parallel-net-removal=false
      //p-large-net-removal=false
      // main -> preprocessing -> community detection
      context.preprocessing.enable_community_detection=false;
      context.preprocessing.community_detection.enable_in_initial_partitioning=true;
      context.preprocessing.community_detection.reuse_communities=false;
      context.preprocessing.community_detection.max_pass_iterations=100;
      context.preprocessing.community_detection.min_eps_improvement=0.0001;
      context.preprocessing.community_detection.edge_weight = kahypar::edgeWeightFromString("hybrid");
      // main -> coarsening
      context.coarsening.algorithm = kahypar::coarseningAlgorithmFromString("ml_style");
      context.coarsening.max_allowed_weight_multiplier = 1;
      context.coarsening.contraction_limit_multiplier = 160;
      // main -> coarsening -> rating
      context.coarsening.algorithm = kahypar::coarseningAlgorithmFromString("heavy_edge");
      context.coarsening.rating.community_policy = CommunityPolicy::use_communities;
      context.coarsening.rating.heavy_node_penalty_policy =
        kahypar::heavyNodePenaltyFromString("no_penalty");
      context.coarsening.rating.acceptance_policy =
        kahypar::acceptanceCriterionFromString("best_prefer_unmatched");
      // main -> initial partitioning
      //i-type=KaHyPar
      context.initial_partitioning.mode = kahypar::modeFromString("recursive");
      context.initial_partitioning.technique =
        kahypar::inititalPartitioningTechniqueFromString("multi");
      // initial partitioning -> coarsening
      context.initial_partitioning.coarsening.algorithm =
        kahypar::coarseningAlgorithmFromString("ml_style");
      context.initial_partitioning.coarsening.max_allowed_weight_multiplier = 1;
      // initial partitioning -> coarsening -> rating
      context.initial_partitioning.coarsening.rating.rating_function =
        kahypar::ratingFunctionFromString("heavy_edge");
      context.initial_partitioning.coarsening.rating.community_policy =
      CommunityPolicy::use_communities;
            context.initial_partitioning.coarsening.rating.heavy_node_penalty_policy =
      kahypar::heavyNodePenaltyFromString("no_penalty");
      context.initial_partitioning.coarsening.rating.acceptance_policy =
      kahypar::acceptanceCriterionFromString("best_prefer_unmatched");
      // initial partitioning -> initial partitioning
            context.initial_partitioning.algo =
        kahypar::initialPartitioningAlgorithmFromString("pool");
      context.initial_partitioning.nruns=20,
      // initial partitioning -> local search
      context.initial_partitioning.local_search.algorithm =
        kahypar::refinementAlgorithmFromString("twoway_fm");
      context.initial_partitioning.local_search.iterations_per_level =
          std::numeric_limits<int>::max();
      // main -> local search
      context.local_search.algorithm = kahypar::refinementAlgorithmFromString("kway_fm_km1");
      context.local_search.iterations_per_level = std::numeric_limits<int>::max();
      context.local_search.fm.stopping_rule = kahypar::stoppingRuleFromString("adaptive_opt");
      context.local_search.fm.adaptive_stopping_alpha = 1;
    }
    
  }
}
