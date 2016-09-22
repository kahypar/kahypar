/***************************************************************************
 *  Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "meta/abstract_factory.h"
#include "meta/policy_registry.h"
#include "meta/registrar.h"
#include "meta/static_double_dispatch_factory.h"
#include "meta/static_multi_dispatch_factory.h"
#include "meta/typelist.h"
#include "partition/coarsening/do_nothing_coarsener.h"
#include "partition/coarsening/full_vertex_pair_coarsener.h"
#include "partition/coarsening/heavy_edge_rater.h"
#include "partition/coarsening/i_coarsener.h"
#include "partition/coarsening/lazy_vertex_pair_coarsener.h"
#include "partition/coarsening/ml_coarsener.h"
#include "partition/initial_partitioning/i_initial_partitioner.h"
#include "partition/refinement/2way_fm_refiner.h"
#include "partition/refinement/do_nothing_refiner.h"
#include "partition/refinement/i_refiner.h"
#include "partition/refinement/kway_fm_cut_refiner.h"
#include "partition/refinement/kway_fm_km1_refiner.h"
#include "partition/refinement/lp_refiner.h"
#include "partition/refinement/policies/2fm_rebalancing_policy.h"
#include "partition/refinement/policies/fm_stop_policy.h"

using meta::Factory;
using meta::PolicyRegistry;
using meta::StaticDoubleDispatchFactory;
using meta::StaticMultiDispatchFactory;
using meta::Typelist;
using partition::AdvancedRandomWalkModelStopsSearch;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;
using partition::GlobalRebalancing;
using partition::NoGlobalRebalancing;

#define REGISTER_COARSENER(id, coarsener)                               \
  static Registrar<partition::CoarsenerFactory> register_ ## coarsener( \
    id,                                                                 \
    [](Hypergraph& hypergraph, const Configuration& config,             \
       const HypernodeWeight weight_of_heaviest_node) -> ICoarsener* {  \
    return new coarsener(hypergraph, config, weight_of_heaviest_node);  \
  })

#define REGISTER_INITIAL_PARTITIONER(id, ip)                                    \
  static Registrar<partition::InitialPartitioningFactory> register_ ## ip(      \
    id,                                                                         \
    [](Hypergraph& hypergraph, Configuration& config) -> IInitialPartitioner* { \
    return new ip(hypergraph, config);                                          \
  })

#define REGISTER_DISPATCHED_REFINER(id, dispatcher, ...)               \
  static Registrar<partition::RefinerFactory> register_ ## dispatcher( \
    id,                                                                \
    [](Hypergraph& hypergraph, const Configuration& config) {          \
    return dispatcher::create(                                         \
      std::forward_as_tuple(hypergraph, config),                       \
      __VA_ARGS__                                                      \
      );                                                               \
  })

#define REGISTER_REFINER(id, refiner)                                      \
  static Registrar<partition::RefinerFactory> register_ ## refiner(        \
    id,                                                                    \
    [](Hypergraph& hypergraph, const Configuration& config) -> IRefiner* { \
    return new refiner(hypergraph, config);                                \
  })

namespace partition {
using CoarsenerFactory = Factory<CoarseningAlgorithm,
                                 ICoarsener* (*)(Hypergraph&, const Configuration&,
                                                 const HypernodeWeight)>;


using RefinerFactory = Factory<RefinementAlgorithm,
                               IRefiner* (*)(Hypergraph&, const Configuration&)>;

using InitialPartitioningFactory = Factory<InitialPartitionerAlgorithm,
                                           IInitialPartitioner* (*)(Hypergraph&, Configuration&)>;

using StoppingPolicyClasses = Typelist<NumberOfFruitlessMovesStopsSearch,
                                       AdvancedRandomWalkModelStopsSearch,
                                       RandomWalkModelStopsSearch,
                                       nGPRandomWalkStopsSearch>;
using RebalancingPolicyClasses = Typelist<GlobalRebalancing,
                                          NoGlobalRebalancing>;

using TwoWayFMFactoryDispatcher = StaticMultiDispatchFactory<TwoWayFMRefiner,
                                                             IRefiner,
                                                             Typelist<StoppingPolicyClasses,
                                                                      RebalancingPolicyClasses> >;


using KWayFMFactoryDispatcher = StaticMultiDispatchFactory<KWayFMRefiner,
                                                           IRefiner,
                                                           Typelist<StoppingPolicyClasses> >;

using KWayKMinusOneFactoryDispatcher = StaticMultiDispatchFactory<KWayKMinusOneRefiner,
                                                                  IRefiner,
                                                                  Typelist<StoppingPolicyClasses> >;

using RandomWinsRater = Rater<RatingType, RandomRatingWins>;
using RandomWinsFullCoarsener = FullVertexPairCoarsener<RandomWinsRater>;
using RandomWinsLazyUpdateCoarsener = LazyVertexPairCoarsener<RandomWinsRater>;
using RandomWinsMLCoarsener = MLCoarsener<RandomWinsRater>;
}  // namespace partition
