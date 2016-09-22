/***************************************************************************
 *  Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "kahypar/meta/abstract_factory.h"
#include "kahypar/meta/static_double_dispatch_factory.h"
#include "kahypar/meta/static_multi_dispatch_factory.h"
#include "kahypar/meta/typelist.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/kway_fm_cut_refiner.h"
#include "kahypar/partition/refinement/kway_fm_km1_refiner.h"
#include "kahypar/partition/refinement/lp_refiner.h"
#include "kahypar/partition/refinement/policies/2fm_rebalancing_policy.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

namespace kahypar {
using CoarsenerFactory = meta::Factory<CoarseningAlgorithm,
                                       ICoarsener* (*)(Hypergraph&, const Configuration&,
                                                       const HypernodeWeight)>;


using RefinerFactory = meta::Factory<RefinementAlgorithm,
                                     IRefiner* (*)(Hypergraph&, const Configuration&)>;

using InitialPartitioningFactory = meta::Factory<InitialPartitionerAlgorithm,
                                                 IInitialPartitioner* (*)(Hypergraph&, Configuration&)>;

using TwoWayFMFactoryDispatcher = meta::StaticMultiDispatchFactory<TwoWayFMRefiner,
                                                                   IRefiner,
                                                                   meta::Typelist<StoppingPolicyClasses,
                                                                                  RebalancingPolicyClasses> >;

using KWayFMFactoryDispatcher = meta::StaticMultiDispatchFactory<KWayFMRefiner,
                                                                 IRefiner,
                                                                 meta::Typelist<StoppingPolicyClasses> >;

using KWayKMinusOneFactoryDispatcher = meta::StaticMultiDispatchFactory<KWayKMinusOneRefiner,
                                                                        IRefiner,
                                                                        meta::Typelist<StoppingPolicyClasses> >;
}  // namespace kahypar
