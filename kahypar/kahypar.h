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

#include "kahypar/meta/policy_registry.h"
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
#include "kahypar/partition/coarsening/vertex_pair_rater.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/factories.h"
#include "kahypar/partition/initial_partitioning/initial_partitioning.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/partitioner.h"
#include "kahypar/partition/refinement/do_nothing_refiner.h"

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


#define REGISTER_INITIAL_PARTITIONER(id, ip)                               \
  static meta::Registrar<InitialPartitioningFactory> register_ ## ip(      \
    id,                                                                    \
    [](Hypergraph& hypergraph, Context& context) -> IInitialPartitioner* { \
    return new ip(hypergraph, context);                                    \
  })

#define REGISTER_DISPATCHED_REFINER(id, dispatcher, ...)          \
  static meta::Registrar<RefinerFactory> register_ ## dispatcher( \
    id,                                                           \
    [](Hypergraph& hypergraph, const Context& context) {          \
    return dispatcher::create(                                    \
      std::forward_as_tuple(hypergraph, context),                 \
      __VA_ARGS__                                                 \
      );                                                          \
  })

#define REGISTER_REFINER(id, refiner)                                 \
  static meta::Registrar<RefinerFactory> register_ ## refiner(        \
    id,                                                               \
    [](Hypergraph& hypergraph, const Context& context) -> IRefiner* { \
    return new refiner(hypergraph, context);                          \
  })

#define REGISTER_POLICY(policy, id, policy_class)                                  \
  static meta::Registrar<meta::PolicyRegistry<policy> > register_ ## policy_class( \
    id, new policy_class())

namespace kahypar {
////////////////////////////////////////////////////////////////////////////////
//                            Rating Functions
////////////////////////////////////////////////////////////////////////////////
using RandomWinsRaterHeavyEdgeRater = VertexPairRater<>;

////////////////////////////////////////////////////////////////////////////////
//                       Coarsening / Rating Policies
////////////////////////////////////////////////////////////////////////////////
REGISTER_POLICY(CommunityPolicy, CommunityPolicy::use_communities,
                UseCommunityStructure);
REGISTER_POLICY(CommunityPolicy, CommunityPolicy::ignore_communities,
                IgnoreCommunityStructure);

REGISTER_POLICY(HeavyNodePenaltyPolicy, HeavyNodePenaltyPolicy::no_penalty,
                NoWeightPenalty);
REGISTER_POLICY(HeavyNodePenaltyPolicy, HeavyNodePenaltyPolicy::multiplicative_penalty,
                MultiplicativePenalty);

REGISTER_POLICY(RatingFunction, RatingFunction::heavy_edge,
                HeavyEdgeScore);
REGISTER_POLICY(RatingFunction, RatingFunction::edge_frequency,
                EdgeFrequencyScore);

using BestWithTieBreaking = BestRatingWithTieBreaking<>;
using BestPreferringUnmatched = BestRatingPreferringUnmatched<>;

REGISTER_POLICY(AcceptancePolicy, AcceptancePolicy::best,
                BestWithTieBreaking);
REGISTER_POLICY(AcceptancePolicy, AcceptancePolicy::best_prefer_unmatched,
                BestPreferringUnmatched);

////////////////////////////////////////////////////////////////////////////////
//                          Coarsening Algorithms
////////////////////////////////////////////////////////////////////////////////
REGISTER_COARSENER(CoarseningAlgorithm::do_nothing, DoNothingCoarsener);

REGISTER_DISPATCHED_COARSENER(CoarseningAlgorithm::heavy_lazy,
                              LazyCoarseningDispatcher,
                              meta::PolicyRegistry<RatingFunction>::getInstance().getPolicy(
                                context.coarsening.rating.rating_function),
                              meta::PolicyRegistry<HeavyNodePenaltyPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.heavy_node_penalty_policy),
                              meta::PolicyRegistry<CommunityPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.community_policy),
                              meta::PolicyRegistry<AcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.acceptance_policy));

REGISTER_DISPATCHED_COARSENER(CoarseningAlgorithm::heavy_full,
                              FullCoarseningDispatcher,
                              meta::PolicyRegistry<RatingFunction>::getInstance().getPolicy(
                                context.coarsening.rating.rating_function),
                              meta::PolicyRegistry<HeavyNodePenaltyPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.heavy_node_penalty_policy),
                              meta::PolicyRegistry<CommunityPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.community_policy),
                              meta::PolicyRegistry<AcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.acceptance_policy));

REGISTER_DISPATCHED_COARSENER(CoarseningAlgorithm::ml_style,
                              MLCoarseningDispatcher,
                              meta::PolicyRegistry<RatingFunction>::getInstance().getPolicy(
                                context.coarsening.rating.rating_function),
                              meta::PolicyRegistry<HeavyNodePenaltyPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.heavy_node_penalty_policy),
                              meta::PolicyRegistry<CommunityPolicy>::getInstance().getPolicy(
                                context.coarsening.rating.community_policy),
                              meta::PolicyRegistry<AcceptancePolicy>::getInstance().getPolicy(
                                context.coarsening.rating.acceptance_policy));


////////////////////////////////////////////////////////////////////////////////
//                          Initial Partitioning Algorithms
////////////////////////////////////////////////////////////////////////////////
using BFSInitialPartitionerBFS = BFSInitialPartitioner<BFSStartNodeSelectionPolicy<> >;
using LPInitialPartitionerBFS_FM =
        LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                           FMGainComputationPolicy>;
using GHGInitialPartitionerBFS_FM_SEQ =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  FMGainComputationPolicy,
                                                  SequentialQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_FM_GLO =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  FMGainComputationPolicy,
                                                  GlobalQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_FM_RND =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  FMGainComputationPolicy,
                                                  RoundRobinQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXP_SEQ =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  MaxPinGainComputationPolicy,
                                                  SequentialQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXP_GLO =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  MaxPinGainComputationPolicy,
                                                  GlobalQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXP_RND =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  MaxPinGainComputationPolicy,
                                                  RoundRobinQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXN_SEQ =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  MaxNetGainComputationPolicy,
                                                  SequentialQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXN_GLO =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  MaxNetGainComputationPolicy,
                                                  GlobalQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXN_RND =
        GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                                  MaxNetGainComputationPolicy,
                                                  RoundRobinQueueSelectionPolicy>;
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::random,
                             RandomInitialPartitioner);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::bfs, BFSInitialPartitionerBFS);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::lp, LPInitialPartitionerBFS_FM);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_sequential,
                             GHGInitialPartitionerBFS_FM_SEQ);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_global,
                             GHGInitialPartitionerBFS_FM_GLO);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_round,
                             GHGInitialPartitionerBFS_FM_RND);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_sequential_maxpin,
                             GHGInitialPartitionerBFS_MAXP_SEQ);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_global_maxpin,
                             GHGInitialPartitionerBFS_MAXP_GLO);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_round_maxpin,
                             GHGInitialPartitionerBFS_MAXP_RND);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_sequential_maxnet,
                             GHGInitialPartitionerBFS_MAXN_SEQ);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_global_maxnet,
                             GHGInitialPartitionerBFS_MAXN_GLO);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_round_maxnet,
                             GHGInitialPartitionerBFS_MAXN_RND);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::pool, PoolInitialPartitioner);

////////////////////////////////////////////////////////////////////////////////
//                       Local Search Algorithm Policies
////////////////////////////////////////////////////////////////////////////////
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::simple,
                NumberOfFruitlessMovesStopsSearch);
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::adaptive_opt,
                AdvancedRandomWalkModelStopsSearch);

////////////////////////////////////////////////////////////////////////////////
//                           Local Search Algorithms
////////////////////////////////////////////////////////////////////////////////
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::twoway_fm,
                            TwoWayFMFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_fm,
                            KWayFMFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));
/*REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_fm_km1,
                            KWayKMinusOneFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));*/
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_flow,
                            KWayKMinusOneFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));
REGISTER_REFINER(RefinementAlgorithm::label_propagation, LPRefiner);
REGISTER_REFINER(RefinementAlgorithm::do_nothing, DoNothingRefiner);
}  // namespace kahypar
