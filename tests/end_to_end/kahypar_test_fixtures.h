/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <limits>

#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
class KaHyParCA : public ::testing::Test {
 public:
  KaHyParCA() :
    context() {
    context.partition.mode = Mode::direct_kway;
    context.partition.objective = Objective::cut;
    context.partition.seed = 2;
    context.partition.rb_lower_k = 0;
    context.partition.rb_upper_k = 0;
    context.preprocessing.enable_community_detection = true;
    context.preprocessing.enable_min_hash_sparsifier = true;
    context.preprocessing.community_detection.enable_in_initial_partitioning = true;
    context.coarsening.algorithm = CoarseningAlgorithm::ml_style;
    context.coarsening.max_allowed_weight_multiplier = 1;
    context.coarsening.contraction_limit_multiplier = 160;
    context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
    context.coarsening.rating.community_policy = CommunityPolicy::use_communities;
    context.coarsening.rating.heavy_node_penalty_policy =
      HeavyNodePenaltyPolicy::multiplicative_penalty;
    context.coarsening.rating.acceptance_policy = AcceptancePolicy::best;
    context.initial_partitioning.mode = Mode::recursive_bisection;
    context.initial_partitioning.technique = InitialPartitioningTechnique::multilevel;
    context.initial_partitioning.coarsening.algorithm = CoarseningAlgorithm::ml_style;
    context.initial_partitioning.coarsening.max_allowed_weight_multiplier = 1;
    context.initial_partitioning.coarsening.contraction_limit_multiplier = 150;
    context.initial_partitioning.algo = InitialPartitionerAlgorithm::pool;
    context.initial_partitioning.nruns = 20;
    context.initial_partitioning.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    context.initial_partitioning.local_search.fm.max_number_of_fruitless_moves = 50;
    context.initial_partitioning.local_search.fm.stopping_rule = RefinementStoppingRule::simple;
    context.initial_partitioning.local_search.iterations_per_level =
      std::numeric_limits<int>::max();
    context.local_search.iterations_per_level = std::numeric_limits<int>::max();
    context.local_search.fm.stopping_rule = RefinementStoppingRule::adaptive_opt;
    context.local_search.fm.adaptive_stopping_alpha = 1;
    context.partition.graph_filename = "test_instances/ISPD98_ibm01.hgr";

    kahypar::Randomize::instance().setSeed(context.partition.seed);
  }

  Context context;
};


class KaHyParK : public ::testing::Test {
 public:
  KaHyParK() :
    context() {
    context.partition.mode = Mode::direct_kway;
    context.partition.objective = Objective::cut;
    context.partition.seed = 2;
    context.partition.rb_lower_k = 0;
    context.partition.rb_upper_k = 0;
    context.coarsening.algorithm = CoarseningAlgorithm::ml_style;
    context.coarsening.max_allowed_weight_multiplier = 1;
    context.coarsening.contraction_limit_multiplier = 160;
    context.initial_partitioning.mode = Mode::recursive_bisection;
    context.initial_partitioning.technique = InitialPartitioningTechnique::multilevel;
    context.initial_partitioning.coarsening.algorithm = CoarseningAlgorithm::ml_style;
    context.initial_partitioning.coarsening.max_allowed_weight_multiplier = 1;
    context.initial_partitioning.algo = InitialPartitionerAlgorithm::pool;
    context.initial_partitioning.nruns = 20;
    context.initial_partitioning.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    context.initial_partitioning.local_search.iterations_per_level = std::numeric_limits<int>::max();
    context.local_search.iterations_per_level = std::numeric_limits<int>::max();
    context.local_search.fm.stopping_rule = RefinementStoppingRule::adaptive_opt;
    context.local_search.fm.adaptive_stopping_alpha = 1;
    context.partition.graph_filename = "test_instances/ISPD98_ibm01.hgr";

    kahypar::Randomize::instance().setSeed(context.partition.seed);
  }

  Context context;
};


class KaHyParR : public ::testing::Test {
 public:
  KaHyParR() :
    context() {
    context.partition.mode = Mode::recursive_bisection;
    context.partition.objective = Objective::cut;
    context.partition.seed = 2;
    context.partition.rb_lower_k = 0;
    context.partition.rb_upper_k = 0;
    context.coarsening.algorithm = CoarseningAlgorithm::heavy_lazy;
    context.coarsening.max_allowed_weight_multiplier = 3.25;
    context.coarsening.contraction_limit_multiplier = 160;
    context.initial_partitioning.mode = Mode::direct_kway;
    context.initial_partitioning.technique = InitialPartitioningTechnique::flat;
    context.initial_partitioning.algo = InitialPartitionerAlgorithm::pool;
    context.initial_partitioning.nruns = 20;
    context.initial_partitioning.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    context.initial_partitioning.local_search.iterations_per_level = std::numeric_limits<int>::max();
    context.local_search.iterations_per_level = std::numeric_limits<int>::max();
    context.local_search.fm.stopping_rule = RefinementStoppingRule::simple;
    context.local_search.fm.max_number_of_fruitless_moves = 50;
    context.partition.graph_filename = "test_instances/ISPD98_ibm01.hgr";

    kahypar::Randomize::instance().setSeed(context.partition.seed);
  }

  Context context;
};

class KaHyParE : public ::testing::Test {
 public:
  KaHyParE() :
    context() {
    parseIniToContext(context, "configs/test.ini");
    context.partition.seed = 2;
    context.partition.k = 3;
    context.partition.quiet_mode = true;
    context.partition.epsilon = 0.03;
    context.partition.objective = Objective::km1;
    context.partition.mode = Mode::direct_kway;
    context.local_search.algorithm = RefinementAlgorithm::kway_fm_km1;
    context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
    context.partition.quiet_mode = false;
    context.partition_evolutionary = true;
    context.partition.graph_filename = "../../../tests/partition/evolutionary/TestHypergraph";

    kahypar::Randomize::instance().setSeed(context.partition.seed);
  }
  Context context;
};
}  // namespace kahypar
