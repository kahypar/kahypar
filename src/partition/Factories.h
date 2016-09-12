/***************************************************************************
 *  Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "meta/Factory.h"
#include "meta/PolicyRegistry.h"
#include "meta/StaticDoubleDispatchFactory.h"
#include "meta/StaticMultiDispatchFactory.h"
#include "meta/Typelist.h"
#include "partition/coarsening/DoNothingCoarsener.h"
#include "partition/coarsening/FullVertexPairCoarsener.h"
#include "partition/coarsening/HeuristicVertexPairCoarsener.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/LazyVertexPairCoarsener.h"
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

using meta::Factory;
using meta::PolicyRegistry;
using meta::NullPolicy;
using meta::StaticDoubleDispatchFactory;
using meta::StaticMultiDispatchFactory;
using meta::Typelist;
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
using RandomWinsHeuristicCoarsener = HeuristicVertexPairCoarsener<RandomWinsRater>;
using RandomWinsFullCoarsener = FullVertexPairCoarsener<RandomWinsRater>;
using RandomWinsLazyUpdateCoarsener = LazyVertexPairCoarsener<RandomWinsRater>;
using RandomWinsMLCoarsener = MLCoarsener<RandomWinsRater>;
}  // namespace partition
