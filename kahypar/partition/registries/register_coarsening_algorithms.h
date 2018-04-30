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

#include "kahypar/meta/registrar.h"
#include "kahypar/partition/coarsening/do_nothing_coarsener.h"
#include "kahypar/partition/coarsening/full_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/lazy_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/ml_coarsener.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/factories.h"

#define REGISTER_COARSENER(id, coarsener)                               \
  static meta::Registrar<CoarsenerFactory> register_ ## coarsener(      \
    id,                                                                 \
    [](Hypergraph& hypergraph, const Context& context,                  \
       const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {  \
    return new coarsener(hypergraph, context, weight_of_heaviest_node); \
  })

#define REGISTER_DISPATCHED_COARSENER(id, dispatcher, ...)                 \
  static meta::Registrar<CoarsenerFactory> register_ ## dispatcher(        \
    id,                                                                    \
    [](Hypergraph& hypergraph, const Context& context,                     \
       const HypernodeWeight weight_of_heaviest_node) {                    \
    return dispatcher::create(                                             \
      std::forward_as_tuple(hypergraph, context, weight_of_heaviest_node), \
      __VA_ARGS__                                                          \
      );                                                                   \
  })

namespace kahypar {
REGISTER_COARSENER(CoarseningAlgorithm::do_nothing, DoNothingCoarsener);

REGISTER_DISPATCHED_COARSENER(CoarseningAlgorithm::heavy_lazy,
                              LazyCoarseningDispatcher,
                              meta::PolicyRegistry<RatingFunction>::getInstance().getPolicy(
                                context.coarsening.rating.rating_function),
                              meta::PolicyRegistry<HeavyNodePenaltyPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.heavy_node_penalty_policy),
                              meta::PolicyRegistry<CommunityPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.community_policy),
                              meta::PolicyRegistry<RatingPartitionPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.partition_policy),
                              meta::PolicyRegistry<AcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.acceptance_policy),
                              meta::PolicyRegistry<FixVertexContractionAcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.fixed_vertex_acceptance_policy));

REGISTER_DISPATCHED_COARSENER(CoarseningAlgorithm::heavy_full,
                              FullCoarseningDispatcher,
                              meta::PolicyRegistry<RatingFunction>::getInstance().getPolicy(
                                context.coarsening.rating.rating_function),
                              meta::PolicyRegistry<HeavyNodePenaltyPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.heavy_node_penalty_policy),
                              meta::PolicyRegistry<CommunityPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.community_policy),
                              meta::PolicyRegistry<RatingPartitionPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.partition_policy),
                              meta::PolicyRegistry<AcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.acceptance_policy),
                              meta::PolicyRegistry<FixVertexContractionAcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.fixed_vertex_acceptance_policy));

REGISTER_DISPATCHED_COARSENER(CoarseningAlgorithm::ml_style,
                              MLCoarseningDispatcher,
                              meta::PolicyRegistry<RatingFunction>::getInstance().getPolicy(
                                context.coarsening.rating.rating_function),
                              meta::PolicyRegistry<HeavyNodePenaltyPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.heavy_node_penalty_policy),
                              meta::PolicyRegistry<CommunityPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.community_policy),
                              meta::PolicyRegistry<RatingPartitionPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.partition_policy),
                              meta::PolicyRegistry<AcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.acceptance_policy),
                              meta::PolicyRegistry<FixVertexContractionAcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.fixed_vertex_acceptance_policy));
}  // namespace kahypar
