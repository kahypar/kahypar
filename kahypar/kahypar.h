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
#include "kahypar/partition/coarsening/heavy_edge_rater.h"
#include "kahypar/partition/coarsening/lazy_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/ml_coarsener.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
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
using RandomWinsRaterHeavyEdgeRater = HeavyEdgeRater<RatingType, RandomRatingWins>;

////////////////////////////////////////////////////////////////////////////////
//                          Coarsening Algorithms
////////////////////////////////////////////////////////////////////////////////
using RandomWinsFullCoarsener = FullVertexPairCoarsener<RandomWinsRaterHeavyEdgeRater>;
using RandomWinsLazyUpdateCoarsener = LazyVertexPairCoarsener<RandomWinsRaterHeavyEdgeRater>;
REGISTER_COARSENER(CoarseningAlgorithm::heavy_lazy, RandomWinsLazyUpdateCoarsener);
REGISTER_COARSENER(CoarseningAlgorithm::heavy_full, RandomWinsFullCoarsener);
REGISTER_COARSENER(CoarseningAlgorithm::ml_style, MLCoarsener);
REGISTER_COARSENER(CoarseningAlgorithm::do_nothing, DoNothingCoarsener);

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
REGISTER_POLICY(GlobalRebalancingMode, GlobalRebalancingMode::on,
                GlobalRebalancing);
REGISTER_POLICY(GlobalRebalancingMode, GlobalRebalancingMode::off,
                NoGlobalRebalancing);

////////////////////////////////////////////////////////////////////////////////
//                           Local Search Algorithms
////////////////////////////////////////////////////////////////////////////////
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::twoway_fm,
                            TwoWayFMFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule),
                            meta::PolicyRegistry<GlobalRebalancingMode>::getInstance().getPolicy(
                              context.local_search.fm.global_rebalancing));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_fm,
                            KWayFMFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_fm_km1,
                            KWayKMinusOneFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));
REGISTER_REFINER(RefinementAlgorithm::label_propagation, LPRefiner);
REGISTER_REFINER(RefinementAlgorithm::do_nothing, DoNothingRefiner);
}  // namespace kahypar
