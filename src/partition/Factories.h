/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_FACTORIES_H_
#define SRC_PARTITION_FACTORIES_H_

#include "lib/core/Factory.h"
#include "lib/core/Parameters.h"
#include "lib/core/PolicyRegistry.h"
#include "lib/core/StaticDispatcher.h"
#include "lib/core/Typelist.h"
#include "partition/coarsening/DoNothingCoarsener.h"
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/LazyUpdateHeavyEdgeCoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/refinement/DoNothingRefiner.h"
#include "partition/refinement/FMFactoryExecutor.h"
#include "partition/refinement/HyperedgeFMRefiner.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/LPRefiner.h"
#include "partition/refinement/MaxGainNodeKWayFMRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"

using core::Parameters;
using core::Factory;
using core::PolicyRegistry;
using core::NullPolicy;
using core::StaticDispatcher;
using core::Typelist;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;

namespace partition {
using CoarsenerFactory = Factory<CoarseningAlgorithm,
                                 ICoarsener* (*)(Hypergraph&, const Configuration&,
                                                 const HypernodeWeight)>;


using RefinerFactory = Factory<RefinementAlgorithm,
                               IRefiner* (*)(Hypergraph&, const Configuration&)>;

using InitialPartitioningFactory = Factory<InitialPartitionerAlgorithm,
                                           IInitialPartitioner* (*)(Hypergraph&, Configuration&)>;

using TwoWayFMFactoryExecutor = KFMFactoryExecutor<TwoWayFMRefiner>;
using TwoWayFMFactoryDispatcher = StaticDispatcher<TwoWayFMFactoryExecutor,
                                                   Typelist<NumberOfFruitlessMovesStopsSearch,
                                                            RandomWalkModelStopsSearch,
                                                            nGPRandomWalkStopsSearch>,
                                                   Typelist<NullPolicy>,
                                                   IRefiner*>;
using HyperedgeFMFactoryExecutor = FMFactoryExecutor<HyperedgeFMRefiner>;
using HyperedgeFMFactoryDispatcher = StaticDispatcher<HyperedgeFMFactoryExecutor,
                                                      Typelist<NumberOfFruitlessMovesStopsSearch,
                                                               RandomWalkModelStopsSearch,
                                                               nGPRandomWalkStopsSearch>,
                                                      Typelist<OnlyRemoveIfBothQueuesClogged>,
                                                      IRefiner*>;

using KWayFMFactoryExecutor = KFMFactoryExecutor<KWayFMRefiner>;
using KWayFMFactoryDispatcher = StaticDispatcher<KWayFMFactoryExecutor,
                                                 Typelist<NumberOfFruitlessMovesStopsSearch,
                                                          RandomWalkModelStopsSearch,
                                                          nGPRandomWalkStopsSearch>,
                                                 Typelist<NullPolicy>,
                                                 IRefiner*>;
using MaxGainNodeKWayFMFactoryExecutor = KFMFactoryExecutor<MaxGainNodeKWayFMRefiner>;
using MaxGainNodeKWayFMFactoryDispatcher = StaticDispatcher<MaxGainNodeKWayFMFactoryExecutor,
                                                            Typelist<NumberOfFruitlessMovesStopsSearch,
                                                                     RandomWalkModelStopsSearch,
                                                                     nGPRandomWalkStopsSearch>,
                                                            Typelist<NullPolicy>,
                                                            IRefiner*>;


using RandomWinsRater = Rater<defs::RatingType, RandomRatingWins>;
using RandomWinsHeuristicCoarsener = HeuristicHeavyEdgeCoarsener<RandomWinsRater>;
using RandomWinsFullCoarsener = FullHeavyEdgeCoarsener<RandomWinsRater>;
using RandomWinsLazyUpdateCoarsener = LazyUpdateHeavyEdgeCoarsener<RandomWinsRater>;
}  // namespace partition
#endif  // SRC_PARTITION_FACTORIES_H_
