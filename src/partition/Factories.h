/***************************************************************************
 *  Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "lib/core/Factory.h"
#include "lib/core/PolicyRegistry.h"
#include "lib/core/StaticDoubleDispatchFactory.h"
#include "lib/core/StaticMultiDispatchFactory.h"
#include "lib/core/Typelist.h"
#include "partition/coarsening/DoNothingCoarsener.h"
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/LazyUpdateHeavyEdgeCoarsener.h"
#include "partition/coarsening/MLCoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/refinement/DoNothingRefiner.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/KWayKMinusOneRefiner.h"
#include "partition/refinement/LPRefiner.h"
#include "partition/refinement/MaxGainNodeKWayFMRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMStopPolicies.h"
#include "partition/refinement/policies/TwoFMRebalancePolicies.h"

using core::Factory;
using core::PolicyRegistry;
using core::NullPolicy;
using core::StaticDoubleDispatchFactory;
using core::StaticMultiDispatchFactory;
using core::Typelist;
using partition::AdvancedRandomWalkModelStopsSearch;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;
using partition::GlobalRebalancing;
using partition::NoGlobalRebalancing;

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

// using MaxGainNodeKWayFMFactoryExecutor = KFMFactoryExecutor<MaxGainNodeKWayFMRefiner>;
// using MaxGainNodeKWayFMFactoryDispatcher = StaticDoubleDispatchFactory<MaxGainNodeKWayFMFactoryExecutor,
//                                                             Typelist<NumberOfFruitlessMovesStopsSearch,
//                                                                      RandomWalkModelStopsSearch,
//                                                                      nGPRandomWalkStopsSearch>,
//                                                             Typelist<NullPolicy>,
//                                                             IRefiner*>;


using RandomWinsRater = Rater<defs::RatingType, RandomRatingWins>;
using RandomWinsHeuristicCoarsener = HeuristicHeavyEdgeCoarsener<RandomWinsRater>;
using RandomWinsFullCoarsener = FullHeavyEdgeCoarsener<RandomWinsRater>;
using RandomWinsLazyUpdateCoarsener = LazyUpdateHeavyEdgeCoarsener<RandomWinsRater>;
using RandomWinsMLCoarsener = MLCoarsener<RandomWinsRater>;
}  // namespace partition
