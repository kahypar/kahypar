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

#include "kahypar/meta/registrar.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/2way_fm_flow_refiner.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/partition/refinement/do_nothing_refiner.h"
#include "kahypar/partition/refinement/flow/2way_flow_refiner.h"
#include "kahypar/partition/refinement/flow/kway_flow_refiner.h"
#include "kahypar/partition/refinement/flow/policies/flow_execution_policy.h"
#include "kahypar/partition/refinement/flow/policies/flow_network_policy.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/kway_fm_cut_refiner.h"
#include "kahypar/partition/refinement/kway_fm_flow_refiner.h"
#include "kahypar/partition/refinement/kway_fm_km1_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

#define REGISTER_DISPATCHED_REFINER(id, dispatcher, ...)          \
  static meta::Registrar<RefinerFactory> register_ ## dispatcher( \
    id,                                                           \
    [](Hypergraph& hypergraph, const Context& context) {          \
    return dispatcher::create(                                    \
      std::forward_as_tuple(hypergraph, context),                 \
      __VA_ARGS__                                                 \
      );                                                          \
  })

#define REREGISTER_REFINER(id, refiner, t)                            \
  static meta::Registrar<RefinerFactory> register_ ## refiner ## t(   \
    id,                                                               \
    [](Hypergraph& hypergraph, const Context& context) -> IRefiner* { \
    return new refiner(hypergraph, context);                          \
  })

#define REGISTER_REFINER(id, refiner)  REREGISTER_REFINER(id, refiner, 1)


namespace kahypar {
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::twoway_fm,
                            TwoWayFMFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_fm,
                            KWayFMFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_fm_km1,
                            KWayKMinusOneFactoryDispatcher,
                            meta::PolicyRegistry<RefinementStoppingRule>::getInstance().getPolicy(
                              context.local_search.fm.stopping_rule));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::twoway_flow,
                            TwoWayFlowFactoryDispatcher,
                            meta::PolicyRegistry<FlowNetworkType>::getInstance().getPolicy(
                              context.local_search.flow.network),
                            meta::PolicyRegistry<FlowExecutionMode>::getInstance().getPolicy(
                              context.local_search.flow.execution_policy));
REGISTER_DISPATCHED_REFINER(RefinementAlgorithm::kway_flow,
                            KWayFlowFactoryDispatcher,
                            meta::PolicyRegistry<FlowNetworkType>::getInstance().getPolicy(
                              context.local_search.flow.network),
                            meta::PolicyRegistry<FlowExecutionMode>::getInstance().getPolicy(
                              context.local_search.flow.execution_policy));
REGISTER_REFINER(RefinementAlgorithm::twoway_fm_flow, TwoWayFMFlowRefiner);
REGISTER_REFINER(RefinementAlgorithm::kway_fm_flow_km1, KWayFMFlowRefiner);
REREGISTER_REFINER(RefinementAlgorithm::kway_fm_flow, KWayFMFlowRefiner, 2);
REGISTER_REFINER(RefinementAlgorithm::do_nothing, DoNothingRefiner);
}  // namespace kahypar
