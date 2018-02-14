/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
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
#include "kahypar/meta/typelist.h"
#include "kahypar/partition/refinement/do_nothing_refiner.h"
#include "kahypar/partition/refinement/flow/2way_flow_refiner.h"
#include "kahypar/partition/refinement/flow/kway_flow_refiner.h"
#include "kahypar/partition/refinement/flow/policies/flow_execution_policy.h"
#include "kahypar/partition/refinement/flow/policies/flow_network_policy.h"

namespace kahypar {
struct FlowRefinerPolicy : meta::PolicyBase {
 public:
  FlowRefinerPolicy() = default;
};

struct DoNothingRefinerPolicy : public FlowRefinerPolicy {
  typedef DoNothingRefiner FlowRefiner;
};

struct TwoWayFlowRefinerPolicy : public FlowRefinerPolicy {
  typedef TwoWayFlowRefiner<HybridNetworkPolicy,
                            ExponentialFlowExecution> FlowRefiner;
};

struct KWayFlowRefinerPolicy : public FlowRefinerPolicy {
  typedef KWayFlowRefiner<HybridNetworkPolicy,
                            ExponentialFlowExecution> FlowRefiner;
};

using TwowWayFlowRefinerPolicyClasses = meta::Typelist<DoNothingRefinerPolicy,
                                                       TwoWayFlowRefinerPolicy>;

using KWayFlowRefinerPolicyClasses = meta::Typelist<DoNothingRefinerPolicy,
                                                    KWayFlowRefinerPolicy>;

}  // namespace kahypar
