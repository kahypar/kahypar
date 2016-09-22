/***************************************************************************
 *  Copyright (C) 2015 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#include <map>
#include <memory>

#include "gmock/gmock.h"

#include "kahypar/datastructure/binary_heap.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/kway_priority_queue.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/initial_partitioning/initial_partitioner_base.h"
#include "kahypar/partition/initial_partitioning/policies/ip_gain_computation_policy.h"

using::testing::Eq;
using::testing::Test;

using KWayRefinementPQ = kahypar::ds::KWayPriorityQueue<kahypar::HypernodeID, kahypar::HyperedgeWeight,
                                                        std::numeric_limits<kahypar::HyperedgeWeight>, true>;
namespace kahypar {
class AGainComputationPolicy : public Test {
 public:
  AGainComputationPolicy() :
    pq(2),
    hypergraph(7, 4,
               HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    config(),
    visit(hypergraph.initialNumNodes()) {
    HypernodeWeight hypergraph_weight = 0;
    for (HypernodeID hn : hypergraph.nodes()) {
      hypergraph_weight += hypergraph.nodeWeight(hn);
    }
    pq.initialize(hypergraph.initialNumNodes());
    initializeConfiguration(hypergraph_weight);
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

  void initializeConfiguration(HypernodeWeight hypergraph_weight) {
    config.initial_partitioning.k = 2;
    config.partition.k = 2;
    config.initial_partitioning.epsilon = 0.05;
    config.partition.epsilon = 0.05;
    config.initial_partitioning.upper_allowed_partition_weight.resize(2);
    config.initial_partitioning.perfect_balance_partition_weight.resize(2);
    for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
      config.initial_partitioning.perfect_balance_partition_weight[i] =
        ceil(
          hypergraph_weight
          / static_cast<double>(config.initial_partitioning.k));
    }
  }

  KWayRefinementPQ pq;
  Hypergraph hypergraph;
  Configuration config;
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
  FMGainComputationPolicy::deltaGainUpdate(hypergraph, config, pq, 3, 1, 0,
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
  ds::FastResetFlagArray<> visit(hypergraph.initialNumNodes());
  FMGainComputationPolicy::deltaGainUpdate(hypergraph, config, pq, 0, -1, 1,
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
  FMGainComputationPolicy::deltaGainUpdate(hypergraph, config, pq, 4, -1, 0,
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
  ds::FastResetFlagArray<> visit(hypergraph.initialNumNodes());
  MaxPinGainComputationPolicy::deltaGainUpdate(hypergraph, config, pq, 3, 1,
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
  ds::FastResetFlagArray<> visit(hypergraph.initialNumNodes());
  MaxNetGainComputationPolicy::deltaGainUpdate(hypergraph, config, pq, 3, 1,
                                               0, visit);
  ASSERT_EQ(pq.key(0, 1), 1);
  ASSERT_EQ(pq.key(1, 1), 1);
  ASSERT_EQ(pq.key(2, 1), 1);
  ASSERT_EQ(pq.key(4, 0), 2);
  ASSERT_EQ(pq.key(5, 0), 1);
  ASSERT_EQ(pq.key(6, 0), 2);
}
}  // namespace kahypar
