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

#include "kahypar/partition/partitioner.h"

namespace kahypar {
namespace partition {
namespace mutate {
static constexpr bool debug = true;

Individual vCycleWithNewInitialPartitioning(Hypergraph& hg, const Individual& in, const Context& context) {
  hg.setPartitionVector(in.partition());
  Context temporary_context(context);
  temporary_context.evolutionary.action =
    Action(meta::Int2Type<static_cast<int>(Decision::mutation)>(),
           meta::Int2Type<static_cast<int>(Subtype::vcycle_initial_partitioning)>());

  DBG << V(temporary_context.evolutionary.action.action) << "===>"
      << V(temporary_context.evolutionary.action.subtype);
  Partitioner().partition(hg, temporary_context);
  return Individual(hg);
}
Individual stableNetMutate(Hypergraph& hg, const Individual& in, const Context& context) {
  hg.setPartitionVector(in.partition());
  Context temporary_context(context);
  temporary_context.evolutionary.action =
    Action { meta::Int2Type<static_cast<int>(Decision::mutation)>(),
             meta::Int2Type<static_cast<int>(Subtype::stable_net)>() };
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
  DBG << V(temporary_context.evolutionary.action.action) << "===>"
      << V(temporary_context.evolutionary.action.subtype);
  Partitioner().partition(hg, temporary_context);
  // TODO (I need to test whether this call to partiton works)
  return Individual(hg);
}
// TODO implement
Individual stableNetMutateWithVCycle(Hypergraph& hg, const Individual& in, const Context& context) {
  hg.setPartitionVector(in.partition());
  Context temporary_context(context);
  temporary_context.evolutionary.action =
    Action { meta::Int2Type<static_cast<int>(Decision::mutation)>(),
             meta::Int2Type<static_cast<int>(Subtype::stable_net_vcycle)>() };
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
  DBG << V(temporary_context.evolutionary.action.action) << "===>"
      << V(temporary_context.evolutionary.action.subtype);
  Partitioner partitioner;
  partitioner.partition(hg, temporary_context);
  // TODO(andre): this doesn't do anything
  // action.requires.initial_partitioning = false;
  // action.requires.vcycle_stable_net_collection = false;
  partitioner.partition(hg, temporary_context);
  return Individual(hg);
}
}  // namespace mutate
}  // namespace partition
}  // namespace kahypar
