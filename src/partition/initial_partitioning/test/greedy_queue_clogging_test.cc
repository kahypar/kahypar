/*
 * greedy_queue_clogging_test.cc
 *
 *  Created on: 16.11.2015
 *      Author: theuer
 */

#include "gmock/gmock.h"
#include <set>

#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/policies/GreedyQueueCloggingPolicy.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"

using ::testing::Eq;
using ::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using partition::FMGainComputationPolicy;
using datastructure::KWayPriorityQueue;

using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
std::numeric_limits<HyperedgeWeight> >;
using Gain = HyperedgeWeight;

namespace partition {

class AGreedyQueueCloggingTest: public Test {
public:
	AGreedyQueueCloggingTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), pq(
					2), parts(), partEnabled(), current_id(0), is_upper_bound_released(
					false) {
		PartitionID k = 4;
		pq.initialize(hypergraph.initialNumNodes());
		initializeConfiguration(k);

	}

	template<class GainComputationPolicy>
	void pushHypernodesIntoQueue(std::vector<HypernodeID>& nodes, bool assign = true) {
		std::set<HypernodeID> part_0;
		part_0.insert(0); part_0.insert(1); part_0.insert(2); part_0.insert(6);

		if(assign) {
			for (HypernodeID hn : hypergraph.nodes()) {
				if(part_0.find(hn) != part_0.end()) {
					hypergraph.setNodePart(hn, 0);
				}
				else {
					hypergraph.setNodePart(hn, 1);
				}
			}
		}

		for(HypernodeID hn : nodes) {
			if(part_0.find(hn) != part_0.end()) {
				pq.insert(hn, 1, GainComputationPolicy::calculateGain(hypergraph, hn, 1));
				pq.enablePart(1);
			}
			else {
				pq.insert(hn, 0, GainComputationPolicy::calculateGain(hypergraph, hn, 0));
				pq.enablePart(0);
			}
		}
	}

	void initializeConfiguration(PartitionID k) {
		config.initial_partitioning.k = k;
		config.partition.k = k;
		config.initial_partitioning.epsilon = 0.05;
		config.partition.epsilon = 0.05;
		config.initial_partitioning.seed = 1;
		config.initial_partitioning.rollback = true;
		config.initial_partitioning.upper_allowed_partition_weight.resize(k);
		config.initial_partitioning.perfect_balance_partition_weight.resize(k);
		for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
			config.initial_partitioning.perfect_balance_partition_weight[i] =
					ceil(
							hypergraph.totalWeight()
									/ static_cast<double>(config.initial_partitioning.k));
			config.initial_partitioning.upper_allowed_partition_weight[i] =
					ceil(
							hypergraph.totalWeight()
									/ static_cast<double>(config.initial_partitioning.k))
							* (1.0 + config.partition.epsilon);
		}

		partEnabled.assign(config.initial_partitioning.k, true);
		parts.clear();
		for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
			parts.push_back(i);
		}

	}

	KWayRefinementPQ pq;
	Hypergraph hypergraph;
	Configuration config;
	std::vector<PartitionID> parts;
	std::vector<bool> partEnabled;
	PartitionID current_id;
	bool is_upper_bound_released;
};

TEST_F(AGreedyQueueCloggingTest,ChecksRoundRobinNextQueueID) {
	ASSERT_EQ(current_id, 0);
	ASSERT_TRUE(
			RoundRobinQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id, 1);
	ASSERT_TRUE(
			RoundRobinQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id, 2);
	ASSERT_TRUE(
			RoundRobinQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id, 3);
	ASSERT_TRUE(
			RoundRobinQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id, 0);
}

TEST_F(AGreedyQueueCloggingTest,ChecksRoundRobinNextQueueIDIfSomePartsAreDisabled) {
	partEnabled[1] = false;
	partEnabled[3] = false;
	ASSERT_EQ(current_id, 0);
	ASSERT_TRUE(
			RoundRobinQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id, 2);
	ASSERT_TRUE(
			RoundRobinQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id, 0);
	ASSERT_TRUE(
			RoundRobinQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id, 2);
}

TEST_F(AGreedyQueueCloggingTest,ChecksIfRoundRobinReturnsFalseIfEveryPartIsDisabled) {
	partEnabled[0] = false;
	partEnabled[1] = false;
	partEnabled[2] = false;
	partEnabled[3] = false;
	ASSERT_FALSE(
			RoundRobinQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id, -1);
}

TEST_F(AGreedyQueueCloggingTest,ChecksGlobalNextQueueID) {
	initializeConfiguration(2);
	std::vector<HypernodeID> nodes = {0,2,5};
	pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);
	current_id = 1;
	ASSERT_TRUE(
			GlobalQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,0);
}

TEST_F(AGreedyQueueCloggingTest,ChecksGlobalNextQueueIDIfSomePartsAreDisabled) {
	initializeConfiguration(2);
	std::vector<HypernodeID> nodes = {0,2,5};
	pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);

	current_id = 1;
	ASSERT_TRUE(
			GlobalQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,0);

	partEnabled[0] = false;
	ASSERT_TRUE(
			GlobalQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,1);
}

TEST_F(AGreedyQueueCloggingTest,ChecksIfGlobalReturnsFalseIfEveryPartIsDisabled) {
	initializeConfiguration(2);
	std::vector<HypernodeID> nodes = {0,2,5};
	pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);

	partEnabled[0] = false; partEnabled[1] = false;
	ASSERT_FALSE(
			GlobalQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,-1);
}

TEST_F(AGreedyQueueCloggingTest,ChecksSequentialNextQueueID) {
	initializeConfiguration(2);
	config.initial_partitioning.unassigned_part = -1;
	std::vector<HypernodeID> nodes = {0,1,2,3,4,5,6};
	pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes,false);
	config.initial_partitioning.upper_allowed_partition_weight[0] = 2;
	config.initial_partitioning.upper_allowed_partition_weight[1] = 2;

	ASSERT_TRUE(
			SequentialQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,0);

	hypergraph.setNodePart(3,0);
	pq.remove(3,0);
	ASSERT_TRUE(
			SequentialQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,0);

	hypergraph.setNodePart(4,0);
	pq.remove(4,0);
	ASSERT_TRUE(
			SequentialQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,1);
}

TEST_F(AGreedyQueueCloggingTest,ChecksSequentialNextQueueIDWithUnassignedPartPlusOne) {
	initializeConfiguration(2);
	config.initial_partitioning.unassigned_part = 1;
	std::vector<HypernodeID> nodes = {0,1,2,3,4,5,6};
	pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes,false);
	config.initial_partitioning.upper_allowed_partition_weight[0] = 2;
	config.initial_partitioning.upper_allowed_partition_weight[1] = 2;

	ASSERT_TRUE(
			SequentialQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,0);

	hypergraph.setNodePart(3,0);
	pq.remove(3,0);
	ASSERT_TRUE(
			SequentialQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,0);

	hypergraph.setNodePart(4,0);
	pq.remove(4,0);
	ASSERT_FALSE(
			SequentialQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,-1);
}

TEST_F(AGreedyQueueCloggingTest,ChecksSequentialNextQueueIDBehaviourIfUpperBoundIsReleased) {
	initializeConfiguration(2);
	std::vector<HypernodeID> nodes = {0,2,5};
	pushHypernodesIntoQueue<FMGainComputationPolicy>(nodes);
	is_upper_bound_released = true;
	current_id = 1;

	ASSERT_TRUE(
			SequentialQueueCloggingPolicy::nextQueueID(hypergraph, config, pq,
					current_id, partEnabled, parts, is_upper_bound_released));
	ASSERT_EQ(current_id,0);
}

}
;

