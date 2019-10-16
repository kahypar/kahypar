/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "kahypar/partition/refinement/flow/policies/flow_execution_policy.h"
#include "kahypar/partition/refinement/flow/policies/flow_network_policy.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

#define REGISTER_POLICY(policy, id, policy_class)                                  \
  static meta::Registrar<meta::PolicyRegistry<policy> > register_ ## policy_class( \
    id, new policy_class())

namespace kahypar {
// //////////////////////////////////////////////////////////////////////////////
//                            Rating Functions
// //////////////////////////////////////////////////////////////////////////////
using RandomWinsRaterHeavyEdgeRater = VertexPairRater<>;

// //////////////////////////////////////////////////////////////////////////////
//                       Coarsening / Rating Policies
// //////////////////////////////////////////////////////////////////////////////
REGISTER_POLICY(RatingPartitionPolicy, RatingPartitionPolicy::normal,
                NormalPartitionPolicy);
REGISTER_POLICY(RatingPartitionPolicy, RatingPartitionPolicy::evolutionary,
                EvoPartitionPolicy);
REGISTER_POLICY(CommunityPolicy, CommunityPolicy::use_communities,
                UseCommunityStructure);
REGISTER_POLICY(CommunityPolicy, CommunityPolicy::ignore_communities,
                IgnoreCommunityStructure);

REGISTER_POLICY(HeavyNodePenaltyPolicy, HeavyNodePenaltyPolicy::no_penalty,
                NoWeightPenalty);
REGISTER_POLICY(HeavyNodePenaltyPolicy, HeavyNodePenaltyPolicy::multiplicative_penalty,
                MultiplicativePenalty);
REGISTER_POLICY(HeavyNodePenaltyPolicy, HeavyNodePenaltyPolicy::edge_frequency_penalty,
                EdgeFrequencyPenalty);

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

REGISTER_POLICY(FixVertexContractionAcceptancePolicy,
                FixVertexContractionAcceptancePolicy::free_vertex_only,
                AllowFreeOnFixedFreeOnFree);
REGISTER_POLICY(FixVertexContractionAcceptancePolicy,
                FixVertexContractionAcceptancePolicy::fixed_vertex_allowed,
                AllowFreeOnFixedFreeOnFreeFixedOnFixed);
REGISTER_POLICY(FixVertexContractionAcceptancePolicy,
                FixVertexContractionAcceptancePolicy::equivalent_vertices,
                AllowFreeOnFreeFixedOnFixed);

// //////////////////////////////////////////////////////////////////////////////
//                       Local Search Algorithm Policies
// //////////////////////////////////////////////////////////////////////////////
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::simple,
                NumberOfFruitlessMovesStopsSearch);
REGISTER_POLICY(RefinementStoppingRule, RefinementStoppingRule::adaptive_opt,
                AdvancedRandomWalkModelStopsSearch);

REGISTER_POLICY(FlowExecutionMode, FlowExecutionMode::constant,
                ConstantFlowExecution);
REGISTER_POLICY(FlowExecutionMode, FlowExecutionMode::multilevel,
                MultilevelFlowExecution);
REGISTER_POLICY(FlowExecutionMode, FlowExecutionMode::exponential,
                ExponentialFlowExecution);

REGISTER_POLICY(FlowNetworkType, FlowNetworkType::lawler,
                LawlerNetworkPolicy);
REGISTER_POLICY(FlowNetworkType, FlowNetworkType::heuer,
                HeuerNetworkPolicy);
REGISTER_POLICY(FlowNetworkType, FlowNetworkType::wong,
                WongNetworkPolicy);
REGISTER_POLICY(FlowNetworkType, FlowNetworkType::hybrid,
                HybridNetworkPolicy);
}  // namespace kahypar
