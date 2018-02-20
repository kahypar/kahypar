/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <vector>

#include "kahypar/partition/evolutionary/edge_frequency.h"
#include "kahypar/partition/partitioner.h"

namespace kahypar {
namespace partition {
namespace mutate {
static constexpr bool debug = false;

Individual vCycleWithNewInitialPartitioning(Hypergraph& hg, const Individual& in,
                                            const Context& context) {
  hg.reset();
  hg.setPartition(in.partition());
  Context temporary_context(context);
  temporary_context.evolutionary.action =
    Action(meta::Int2Type<static_cast<int>(EvoDecision::mutation)>(),
           meta::Int2Type<static_cast<int>(EvoMutateStrategy::new_initial_partitioning_vcycle)>());

  DBG << V(temporary_context.evolutionary.action.decision());
  DBG << "initial" << V(in.fitness()) << V(metrics::imbalance(hg, context));
  DBG << "initial" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  Partitioner().partition(hg, temporary_context);
  DBG << "after mutate" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  io::serializer::serializeEvolutionary(temporary_context, hg);
  io::printEvolutionaryInformation(temporary_context);
  return Individual(hg);
}

Individual vCycle(Hypergraph& hg, const Individual& in,
                  const Context& context) {
  hg.reset();
  hg.setPartition(in.partition());
  Context temporary_context(context);
  temporary_context.evolutionary.action =
    Action(meta::Int2Type<static_cast<int>(EvoDecision::mutation)>(),
           meta::Int2Type<static_cast<int>(EvoMutateStrategy::vcycle)>());

  DBG << V(temporary_context.evolutionary.action.decision());
  DBG << "initial" << V(in.fitness()) << V(metrics::imbalance(hg, context));
  DBG << "initial" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  Partitioner().partition(hg, temporary_context);
  DBG << "after mutate" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  io::serializer::serializeEvolutionary(temporary_context, hg);
  io::printEvolutionaryInformation(temporary_context);
  return Individual(hg);
}


}  // namespace mutate
}  // namespace partition
}  // namespace kahypar
