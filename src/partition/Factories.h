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
#include "partition/coarsening/FullHeavyEdgeCoarsener.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/HyperedgeCoarsener.h"
#include "partition/coarsening/HyperedgeRatingPolicies.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/LazyUpdateHeavyEdgeCoarsener.h"
#include "partition/coarsening/Rater.h"
#include "partition/refinement/FMFactoryExecutor.h"
#include "partition/refinement/HyperedgeFMRefiner.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KWayFMRefiner.h"
#include "partition/refinement/LPRefiner.h"
#include "partition/refinement/MaxGainNodeKWayFMRefiner.h"
#include "partition/refinement/TwoWayFMRefiner.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMStopPolicies.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"

using core::Parameters;
using core::Factory;
using core::PolicyRegistry;
using core::NullPolicy;
using core::StaticDispatcher;
using core::Typelist;
using partition::NumberOfFruitlessMovesStopsSearch;
using partition::RandomWalkModelStopsSearch;
using partition::nGPRandomWalkStopsSearch;
using partition::EdgeWeightDivMultPinWeight;

namespace partition {
struct CoarsenerFactoryParameters {
  CoarsenerFactoryParameters(Hypergraph& hgr, Configuration& conf) :
    hypergraph(hgr),
    config(conf) { }
  Hypergraph& hypergraph;
  Configuration& config;
};

struct InitialPartitioningFactoryParameters {
	InitialPartitioningFactoryParameters(Hypergraph& hgr, Configuration& conf) :
			hypergraph(hgr), config(conf) {
	}
	Hypergraph& hypergraph;
	Configuration& config;
};

using CoarsenerFactory = Factory<ICoarsener, CoarseningAlgorithm,
                                 ICoarsener* (*)(const CoarsenerFactoryParameters&),
                                 CoarsenerFactoryParameters>;

using RefinerFactory = Factory<IRefiner, RefinementAlgorithm,
                               IRefiner* (*)(const RefinerParameters&),
                               RefinerParameters>;

using TwoWayFMFactoryExecutor = KFMFactoryExecutor<TwoWayFMRefiner>;
using HyperedgeFMFactoryExecutor = FMFactoryExecutor<HyperedgeFMRefiner>;
using KWayFMFactoryExecutor = KFMFactoryExecutor<KWayFMRefiner>;
using MaxGainNodeKWayFMFactoryExecutor = KFMFactoryExecutor<MaxGainNodeKWayFMRefiner>;
using TwoWayFMFactoryDispatcher = StaticDispatcher<TwoWayFMFactoryExecutor,
                                                   Typelist<NumberOfFruitlessMovesStopsSearch,
                                                            RandomWalkModelStopsSearch,
                                                            nGPRandomWalkStopsSearch>,
                                                   Typelist<NullPolicy>,
                                                   IRefiner*>;
using HyperedgeFMFactoryDispatcher = StaticDispatcher<HyperedgeFMFactoryExecutor,
                                                      Typelist<NumberOfFruitlessMovesStopsSearch,
                                                               RandomWalkModelStopsSearch,
                                                               nGPRandomWalkStopsSearch>,
                                                      Typelist<OnlyRemoveIfBothQueuesClogged>,
                                                      IRefiner*>;
using KWayFMFactoryDispatcher = StaticDispatcher<KWayFMFactoryExecutor,
                                                 Typelist<NumberOfFruitlessMovesStopsSearch,
                                                          RandomWalkModelStopsSearch,
                                                          nGPRandomWalkStopsSearch>,
                                                 Typelist<NullPolicy>,
                                                 IRefiner*>;
using MaxGainNodeKWayFMFactoryDispatcher = StaticDispatcher<MaxGainNodeKWayFMFactoryExecutor,
                                                            Typelist<NumberOfFruitlessMovesStopsSearch,
                                                                     RandomWalkModelStopsSearch,
                                                                     nGPRandomWalkStopsSearch>,
                                                            Typelist<NullPolicy>,
                                                            IRefiner*>;
using CoarsenerFactory = Factory<ICoarsener, CoarseningAlgorithm,
                                 ICoarsener* (*)(const CoarsenerFactoryParameters&),
                                 CoarsenerFactoryParameters>;

using RefinerFactory = Factory<IRefiner, RefinementAlgorithm,
                               IRefiner* (*)(const RefinerParameters&),
                               RefinerParameters>;

using InitialPartitioningFactory = Factory<IInitialPartitioner, InitialPartitionerAlgorithm,
										IInitialPartitioner* (*)(const InitialPartitioningFactoryParameters&),
										InitialPartitioningFactoryParameters>;

using RandomWinsRater = Rater<defs::RatingType, RandomRatingWins>;
using RandomWinsHeuristicCoarsener = HeuristicHeavyEdgeCoarsener<RandomWinsRater>;
using RandomWinsFullCoarsener = FullHeavyEdgeCoarsener<RandomWinsRater>;
using RandomWinsLazyUpdateCoarsener = LazyUpdateHeavyEdgeCoarsener<RandomWinsRater>;
using HyperedgeCoarsener2 = HyperedgeCoarsener<EdgeWeightDivMultPinWeight>;
}  // namespace partition
#endif  // SRC_PARTITION_FACTORIES_H_
