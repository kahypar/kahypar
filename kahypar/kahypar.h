/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "meta/policy_registry.h"
#include "meta/registrar.h"
#include "partition/coarsening/do_nothing_coarsener.h"
#include "partition/coarsening/full_vertex_pair_coarsener.h"
#include "partition/coarsening/heavy_edge_rater.h"
#include "partition/coarsening/lazy_vertex_pair_coarsener.h"
#include "partition/coarsening/ml_coarsener.h"
#include "partition/configuration.h"
#include "partition/factories.h"
#include "partition/initial_partitioning/initial_partitioning.h"
#include "partition/metrics.h"
#include "partition/partitioner.h"
#include "partition/refinement/do_nothing_refiner.h"

#define REGISTER_COARSENER(id, coarsener)                              \
  static meta::Registrar<CoarsenerFactory> register_ ## coarsener(     \
    id,                                                                \
    [](Hypergraph& hypergraph, const Configuration& config,            \
       const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* { \
    return new coarsener(hypergraph, config, weight_of_heaviest_node); \
  })

#define REGISTER_INITIAL_PARTITIONER(id, ip)                                    \
  static meta::Registrar<InitialPartitioningFactory> register_ ## ip(           \
    id,                                                                         \
    [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* { \
    return new ip(hypergraph, config);                                          \
  })

#define REGISTER_DISPATCHED_REFINER(id, dispatcher, ...)          \
  static meta::Registrar<RefinerFactory> register_ ## dispatcher( \
    id,                                                           \
    [](Hypergraph& hypergraph, const Configuration& config) {     \
    return dispatcher::create(                                    \
      std::forward_as_tuple(hypergraph, config),                  \
      __VA_ARGS__                                                 \
      );                                                          \
  })

#define REGISTER_REFINER(id, refiner)                                      \
  static meta::Registrar<RefinerFactory> register_ ## refiner(             \
    id,                                                                    \
    [](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* { \
    return new refiner(hypergraph, config);                                \
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
using RandomWinsMLCoarsener = MLCoarsener<RandomWinsRaterHeavyEdgeRater>;
REGISTER_COARSENER(CoarseningAlgorithm::heavy_lazy, RandomWinsLazyUpdateCoarsener);
REGISTER_COARSENER(CoarseningAlgorithm::heavy_full, RandomWinsFullCoarsener);
REGISTER_COARSENER(CoarseningAlgorithm::ml_style, RandomWinsMLCoarsener);
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
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::adaptive1,
                RandomWalkModelStopsSearch);
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::adaptive2,
                nGPRandomWalkStopsSearch);
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
                              config.local_search.fm.stopping_rule),
                            meta::PolicyRegistry<GlobalRebalancingMode>::getInstance().getPolicy(
                              config.local_search.fm.global_rebalancing));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_fm,
                            KWayFMFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              config.local_search.fm.stopping_rule));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_fm_km1,
                            KWayKMinusOneFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              config.local_search.fm.stopping_rule));
REGISTER_REFINER(RefinementAlgorithm::label_propagation, LPRefiner);
REGISTER_REFINER(RefinementAlgorithm::do_nothing, DoNothingRefiner);
}  // namespace kahypar
