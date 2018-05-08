/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
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

#include <map>
#include <memory>

#include "gmock/gmock.h"

#include "kahypar/datastructure/binary_heap.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/kway_priority_queue.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/policies/ip_gain_computation_policy.h"

using ::testing::Eq;
using ::testing::Test;

namespace kahypar {
class AGainComputationPolicy : public Test {
  using KWayRefinementPQ = ds::KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                                 std::numeric_limits<HyperedgeWeight>, true>;

 public:
  AGainComputationPolicy() :
    pq(2),
    hypergraph(7, 4,
               HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    context(),
    visit(hypergraph.initialNumNodes()) {
    HypernodeWeight hypergraph_weight = 0;
    for (const HypernodeID& hn : hypergraph.nodes()) {
      hypergraph_weight += hypergraph.nodeWeight(hn);
    }
    pq.initialize(hypergraph.initialNumNodes());
    initializeContext(hypergraph_weight);
  }

  template <class GainComputationPolicy>
  void pushAllHypernodesIntoQueue(bool assign = true, bool push = true) {
    if (assign) {
      for (HypernodeID hn = 0; hn < 3; hn++) {
        hypergraph.setNodePart(hn, 0);
      }
      for (HypernodeID hn = 3; hn < 7; hn++) {
        hypergraph.setNodePart(hn, 1);
      }
    }

    if (push) {
      pq.insert(0, 1, GainComputationPolicy::calculateGain(hypergraph, 0, 1, visit));
      pq.insert(1, 1, GainComputationPolicy::calculateGain(hypergraph, 1, 1, visit));
      pq.insert(2, 1, GainComputationPolicy::calculateGain(hypergraph, 2, 1, visit));
      pq.insert(3, 0, GainComputationPolicy::calculateGain(hypergraph, 3, 0, visit));
      pq.insert(4, 0, GainComputationPolicy::calculateGain(hypergraph, 4, 0, visit));
      pq.insert(5, 0, GainComputationPolicy::calculateGain(hypergraph, 5, 0, visit));
      pq.insert(6, 0, GainComputationPolicy::calculateGain(hypergraph, 6, 0, visit));
    }
  }

  void initializeContext(HypernodeWeight hypergraph_weight) {
    context.initial_partitioning.k = 2;
    context.partition.k = 2;
    context.partition.epsilon = 0.05;
    context.initial_partitioning.upper_allowed_partition_weight.resize(2);
    context.initial_partitioning.perfect_balance_partition_weight.resize(2);
    for (PartitionID i = 0; i < context.initial_partitioning.k; i++) {
      context.initial_partitioning.perfect_balance_partition_weight.push_back(
        ceil(
          hypergraph_weight
          / static_cast<double>(context.initial_partitioning.k)));
    }
  }

  KWayRefinementPQ pq;
  Hypergraph hypergraph;
  Context context;
  ds::FastResetFlagArray<> visit;
};

TEST_F(AGainComputationPolicy, ComputesCorrectFMGains) {
  pushAllHypernodesIntoQueue<FMGainComputationPolicy>(true, false);
  ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph, 0, 1, visit), -1);
  ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph, 1, 1, visit), 0);
  ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph, 2, 1, visit), 0);
  ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph, 3, 0, visit), -1);
  ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph, 4, 0, visit), -1);
  ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph, 5, 0, visit), 0);
  ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph, 6, 0, visit), -1);
}

TEST_F(AGainComputationPolicy, PerformsCorrectFMDeltaGainUpdates) {
  pushAllHypernodesIntoQueue<FMGainComputationPolicy>();
  hypergraph.initializeNumCutHyperedges();
  hypergraph.changeNodePart(3, 1, 0);
  pq.remove(3, 0);
  ds::FastResetFlagArray<> visit(hypergraph.initialNumNodes());
  FMGainComputationPolicy::deltaGainUpdate(hypergraph, context, pq, 3, 1, 0,
                                           visit);
  ASSERT_EQ(pq.key(0, 1), -1);
  ASSERT_EQ(pq.key(1, 1), 0);
  ASSERT_EQ(pq.key(2, 1), 0);
  ASSERT_EQ(pq.key(4, 0), 1);
  ASSERT_EQ(pq.key(5, 0), 0);
  ASSERT_EQ(pq.key(6, 0), 0);
}

TEST_F(AGainComputationPolicy, ComputesCorrectFMGainsAfterDeltaGainUpdateOnUnassignedPartMinusOne) {
  pushAllHypernodesIntoQueue<FMGainComputationPolicy>(false);

  // Test correct gain values before
  ASSERT_EQ(pq.key(0, 1), 0);
  ASSERT_EQ(pq.key(1, 1), 0);
  ASSERT_EQ(pq.key(2, 1), 0);
  ASSERT_EQ(pq.key(3, 0), 0);
  ASSERT_EQ(pq.key(4, 0), 0);
  ASSERT_EQ(pq.key(5, 0), 0);
  ASSERT_EQ(pq.key(6, 0), 0);

  hypergraph.setNodePart(0, 1);
  pq.remove(0, 1);
  FMGainComputationPolicy::deltaGainUpdate(hypergraph, context, pq, 0, -1, 1,
                                           visit);

  // Test correct gain values after delta gain
  ASSERT_EQ(pq.key(1, 1), 0);
  ASSERT_EQ(pq.key(2, 1), 0);
  ASSERT_EQ(pq.key(3, 0), -1);
  ASSERT_EQ(pq.key(4, 0), -1);
  ASSERT_EQ(pq.key(5, 0), 0);
  ASSERT_EQ(pq.key(6, 0), 0);

  hypergraph.setNodePart(4, 0);
  pq.remove(4, 0);
  FMGainComputationPolicy::deltaGainUpdate(hypergraph, context, pq, 4, -1, 0,
                                           visit);

  // Test correct gain values after delta gain
  ASSERT_EQ(pq.key(1, 1), 0);
  ASSERT_EQ(pq.key(2, 1), 0);
  ASSERT_EQ(pq.key(3, 0), 0);
  ASSERT_EQ(pq.key(5, 0), 0);
  ASSERT_EQ(pq.key(6, 0), 0);
}

TEST_F(AGainComputationPolicy, ComputesCorrectZeroGainForMovesToSameBlock) {
  pushAllHypernodesIntoQueue<FMGainComputationPolicy>(true, false);
  ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph, 0, 0, visit), 0);
}


TEST_F(AGainComputationPolicy, ComputesCorrectMaxPinGains) {
  pushAllHypernodesIntoQueue<MaxPinGainComputationPolicy>(true, false);
  ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph, 0, 1, visit), 2);
  ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph, 1, 1, visit), 2);
  ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph, 2, 1, visit), 2);
  ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph, 3, 0, visit), 2);
  ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph, 4, 0, visit), 2);
  ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph, 5, 0, visit), 1);
  ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph, 6, 0, visit), 1);
}

TEST_F(AGainComputationPolicy, ComputesCorrectMaxPinDeltaGains) {
  pushAllHypernodesIntoQueue<MaxPinGainComputationPolicy>();
  hypergraph.initializeNumCutHyperedges();
  hypergraph.changeNodePart(3, 1, 0);
  pq.remove(3, 0);
  MaxPinGainComputationPolicy::deltaGainUpdate(hypergraph, context, pq, 3, 1,
                                               0, visit);
  ASSERT_EQ(pq.key(0, 1), 1);
  ASSERT_EQ(pq.key(1, 1), 1);
  ASSERT_EQ(pq.key(2, 1), 2);
  ASSERT_EQ(pq.key(4, 0), 3);
  ASSERT_EQ(pq.key(5, 0), 1);
  ASSERT_EQ(pq.key(6, 0), 2);
}

TEST_F(AGainComputationPolicy, ComputesCorrectMaxNetGainGains) {
  pushAllHypernodesIntoQueue<MaxNetGainComputationPolicy>(true, false);
  ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph, 0, 1, visit), 1);
  ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph, 1, 1, visit), 1);
  ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph, 2, 1, visit), 1);
  ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph, 3, 0, visit), 1);
  ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph, 4, 0, visit), 1);
  ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph, 5, 0, visit), 1);
  ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph, 6, 0, visit), 1);
}

TEST_F(AGainComputationPolicy, ComputesCorrectMaxNetDeltaGains) {
  pushAllHypernodesIntoQueue<MaxNetGainComputationPolicy>();
  hypergraph.initializeNumCutHyperedges();
  hypergraph.changeNodePart(3, 1, 0);
  pq.remove(3, 0);
  MaxNetGainComputationPolicy::deltaGainUpdate(hypergraph, context, pq, 3, 1,
                                               0, visit);
  ASSERT_EQ(pq.key(0, 1), 1);
  ASSERT_EQ(pq.key(1, 1), 1);
  ASSERT_EQ(pq.key(2, 1), 1);
  ASSERT_EQ(pq.key(4, 0), 2);
  ASSERT_EQ(pq.key(5, 0), 1);
  ASSERT_EQ(pq.key(6, 0), 2);
}
}  // namespace kahypar
