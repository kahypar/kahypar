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
#include "kahypar/partition/context.h"
#include "kahypar/partition/initial_partitioning/initial_partitioning.h"

#define REGISTER_INITIAL_PARTITIONER(id, ip)                               \
  static meta::Registrar<InitialPartitioningFactory> register_ ## ip(      \
    id,                                                                    \
    [](Hypergraph& hypergraph, Context& context) -> IInitialPartitioner* { \
    return new ip(hypergraph, context);                                    \
  })

namespace kahypar {
using BFSInitialPartitionerBFS = BFSInitialPartitioner<BFSStartNodeSelectionPolicy<> >;
using LPInitialPartitionerBFS_FM =
  LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                     FMGainComputationPolicy>;
using GHGInitialPartitionerBFS_FM_SEQ =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            FMGainComputationPolicy,
                                            SequentialQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_FM_GLO =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            FMGainComputationPolicy,
                                            GlobalQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_FM_RND =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            FMGainComputationPolicy,
                                            RoundRobinQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXP_SEQ =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            MaxPinGainComputationPolicy,
                                            SequentialQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXP_GLO =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            MaxPinGainComputationPolicy,
                                            GlobalQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXP_RND =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            MaxPinGainComputationPolicy,
                                            RoundRobinQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXN_SEQ =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            MaxNetGainComputationPolicy,
                                            SequentialQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXN_GLO =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            MaxNetGainComputationPolicy,
                                            GlobalQueueSelectionPolicy>;
using GHGInitialPartitionerBFS_MAXN_RND =
  GreedyHypergraphGrowingInitialPartitioner<BFSStartNodeSelectionPolicy<>,
                                            MaxNetGainComputationPolicy,
                                            RoundRobinQueueSelectionPolicy>;
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::random,
                             RandomInitialPartitioner);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::bfs, BFSInitialPartitionerBFS);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::lp, LPInitialPartitionerBFS_FM);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_sequential,
                             GHGInitialPartitionerBFS_FM_SEQ);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_global,
                             GHGInitialPartitionerBFS_FM_GLO);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_round,
                             GHGInitialPartitionerBFS_FM_RND);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_sequential_maxpin,
                             GHGInitialPartitionerBFS_MAXP_SEQ);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_global_maxpin,
                             GHGInitialPartitionerBFS_MAXP_GLO);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_round_maxpin,
                             GHGInitialPartitionerBFS_MAXP_RND);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_sequential_maxnet,
                             GHGInitialPartitionerBFS_MAXN_SEQ);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_global_maxnet,
                             GHGInitialPartitionerBFS_MAXN_GLO);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::greedy_round_maxnet,
                             GHGInitialPartitionerBFS_MAXN_RND);
REGISTER_INITIAL_PARTITIONER(InitialPartitionerAlgorithm::pool, PoolInitialPartitioner);
}  // namespace kahypar
