/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/meta/abstract_factory.h"
#include "kahypar/meta/static_multi_dispatch_factory.h"
#include "kahypar/meta/typelist.h"
#include "kahypar/partition/coarsening/full_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/coarsening/lazy_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/ml_coarsener.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/initial_partitioning/i_initial_partitioner.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/partition/refinement/flow/2way_flow_refiner.h"
#include "kahypar/partition/refinement/flow/kway_flow_refiner.h"
#include "kahypar/partition/refinement/flow/policies/flow_execution_policy.h"
#include "kahypar/partition/refinement/flow/policies/flow_network_policy.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/kway_fm_cut_refiner.h"
#include "kahypar/partition/refinement/kway_fm_km1_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

namespace kahypar {
using CoarsenerFactory = meta::Factory<CoarseningAlgorithm,
                                       ICoarsener* (*)(Hypergraph&, const Context&,
                                                       const HypernodeWeight)>;

using InitialPartitioningFactory = meta::Factory<InitialPartitionerAlgorithm,
                                                 IInitialPartitioner* (*)(Hypergraph&, Context&)>;

using RatingPolicies = meta::Typelist<RatingScorePolicies, HeavyNodePenaltyPolicies,
                                      CommunityPolicies, PartitionPolicies, AcceptancePolicies,
                                      FixedVertexAcceptancePolicies>;

using MLCoarseningDispatcher = meta::StaticMultiDispatchFactory<MLCoarsener,
                                                                ICoarsener,
                                                                RatingPolicies>;

using FullCoarseningDispatcher = meta::StaticMultiDispatchFactory<FullVertexPairCoarsener,
                                                                  ICoarsener,
                                                                  RatingPolicies>;

using LazyCoarseningDispatcher = meta::StaticMultiDispatchFactory<LazyVertexPairCoarsener,
                                                                  ICoarsener,
                                                                  RatingPolicies>;

using TwoWayFMFactoryDispatcher = meta::StaticMultiDispatchFactory<TwoWayFMRefiner,
                                                                   IRefiner,
                                                                   meta::Typelist<StoppingPolicyClasses> >;

using KWayFMFactoryDispatcher = meta::StaticMultiDispatchFactory<KWayFMRefiner,
                                                                 IRefiner,
                                                                 meta::Typelist<StoppingPolicyClasses> >;

using KWayKMinusOneFactoryDispatcher = meta::StaticMultiDispatchFactory<KWayKMinusOneRefiner,
                                                                        IRefiner,
                                                                        meta::Typelist<StoppingPolicyClasses> >;

using TwoWayFlowFactoryDispatcher = meta::StaticMultiDispatchFactory<TwoWayFlowRefiner,
                                                                     IRefiner,
                                                                     meta::Typelist<FlowNetworkPolicyClasses,
                                                                                    FlowExecutionPolicyClasses> >;

using KWayFlowFactoryDispatcher = meta::StaticMultiDispatchFactory<KWayFlowRefiner,
                                                                   IRefiner,
                                                                   meta::Typelist<FlowNetworkPolicyClasses,
                                                                                  FlowExecutionPolicyClasses> >;
}  // namespace kahypar
