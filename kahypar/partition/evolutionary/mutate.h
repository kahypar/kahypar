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
#include "kahypar/partition/evolutionary/stablenet.h"
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
  return Individual(hg);
}

Individual vCycle(Hypergraph& hg, const Individual& in,
                  const Context& context) {
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
  return Individual(hg);
}

Individual removeStableNets(Hypergraph& hg, const Individual& in, const Context& context) {
  Context temporary_context(context);
  hg.setPartition(in.partition());

  temporary_context.evolutionary.action =
    Action { meta::Int2Type<static_cast<int>(EvoDecision::mutation)>(),
             meta::Int2Type<static_cast<int>(EvoMutateStrategy::single_stable_net)>() };

  temporary_context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;

  DBG << "initial" << V(in.fitness()) << V(metrics::imbalance(hg, context));
  DBG << "initial" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  Partitioner().partition(hg, temporary_context);
  DBG << "after vcycle for stable net collection"
      << V(metrics::km1(hg))
      << V(metrics::imbalance(hg, context));

  stablenet::removeStableNets(hg, temporary_context,
                              temporary_context.evolutionary.stable_nets_final);

  DBG << "after stable net removal:" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));

  std::vector<PartitionID> new_partition;
  for (const HypernodeID& hn : hg.nodes()) {
    new_partition.push_back(hg.partID(hn));
  }

  hg.setPartition(new_partition);

  Partitioner().partition(hg, temporary_context);
  DBG << "final result" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  io::serializer::serializeEvolutionary(context, hg);
  return Individual(hg);
}

Individual edgeFrequency(Hypergraph& hg, const Context& context, const Population& population) {
  hg.reset();
  Context temporary_context(context);

  temporary_context.evolutionary.action =
    Action { meta::Int2Type<static_cast<int>(EvoDecision::mutation)>(),
             meta::Int2Type<static_cast<int>(EvoMutateStrategy::edge_frequency)>() };

  temporary_context.coarsening.rating.rating_function = RatingFunction::edge_frequency;
  temporary_context.coarsening.rating.partition_policy = RatingPartitionPolicy::normal;
  temporary_context.coarsening.rating.heavy_node_penalty_policy =
    HeavyNodePenaltyPolicy::edge_frequency_penalty;

  temporary_context.evolutionary.edge_frequency =
    computeEdgeFrequency(population.listOfBest(context.evolutionary.edge_frequency_amount),
                         hg.initialNumEdges());

  DBG << V(temporary_context.evolutionary.action.decision());

  Partitioner().partition(hg, temporary_context);
  DBG << "final result" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  io::serializer::serializeEvolutionary(temporary_context, hg);
  return Individual(hg);
}

Individual removePopulationStableNets(Hypergraph& hg, const Population& population const Context& context) {
  // No action required as we do not access the partitioner for this

  DBG << "action.decision() = population_stable_net";
  DBG << V(context.evolutionary.stable_net_amount);
  std::vector<HyperedgeID> stable_nets =
    stablenet::stableNetsFromIndividuals(context,
                                         population.listOfBest(context.evolutionary.stable_net_amount),
                                         hg.initialNumEdges());
  
  DBG << V(stable_nets.size());
  //hg.reset();
  //hg.setPartition(in.partition());
  stablenet::removeStableNets(hg, context, stable_nets);

  Context temporary_context(context);
  //TODO action is required for output but population_stable_net does not want to be casted

  hg.reset();
  Partitioner().partition(hg, temporary_context);
  
  
  // Output action for logging
    temporary_context.evolutionary.action =
    Action { meta::Int2Type<static_cast<int>(EvoDecision::mutation)>(),
             meta::Int2Type<static_cast<int>(EvoMutateStrategy::population_stable_net)>() };
  //Output action
  DBG << "final result" << V(metrics::km1(hg)) << V(metrics::imbalance(hg, context));
  io::serializer::serializeEvolutionary(temporary_context, hg);
  return Individual(hg);
}
}  // namespace mutate
}  // namespace partition
}  // namespace kahypar
