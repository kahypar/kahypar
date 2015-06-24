/*
 * gain_computation_test.cc
 *
 *  Created on: 20.05.2015
 *      Author: theuer
 */



/*
 * initial_partitioner_base_test.cc
 *
 *  Created on: Apr 17, 2015
 *      Author: theuer
 */

#include <map>

#include "gmock/gmock.h"

#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "external/binary_heap/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"

using ::testing::Eq;
using ::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using partition::FMGainComputationPolicy;

using Gain = HyperedgeWeight;

using TwoWayFMHeap = NoDataBinaryMaxHeap<HypernodeID, Gain,
std::numeric_limits<HyperedgeWeight> >;
using PrioQueue = PriorityQueue<TwoWayFMHeap>;

namespace partition {

class AGainComputationTest: public Test {
public:
	AGainComputationTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(),pq() {

		HypernodeWeight hypergraph_weight = 0;
		for (HypernodeID hn : hypergraph.nodes()) {
			hypergraph_weight += hypergraph.nodeWeight(hn);
		}

		for(HypernodeID hn = 0; hn < 3; hn++) {
			hypergraph.setNodePart(hn,0);
		}
		for(HypernodeID hn = 3; hn < 7; hn++) {
			hypergraph.setNodePart(hn,1);
		}

		initializeConfiguration(hypergraph_weight);

		pq.resize(2);
		for (PartitionID i = 0; i < 2; i++) {
			pq[i] = new PrioQueue(hypergraph.numNodes());
		}
	}

	template<class GainComputationPolicy>
	void pushAllHypernodesIntoQueue() {
		pq[1]->insert(0,GainComputationPolicy::calculateGain(hypergraph,0,1));
		pq[1]->insert(1,GainComputationPolicy::calculateGain(hypergraph,1,1));
		pq[1]->insert(2,GainComputationPolicy::calculateGain(hypergraph,2,1));
		pq[0]->insert(3,GainComputationPolicy::calculateGain(hypergraph,3,0));
		pq[0]->insert(4,GainComputationPolicy::calculateGain(hypergraph,4,0));
		pq[0]->insert(5,GainComputationPolicy::calculateGain(hypergraph,5,0));
		pq[0]->insert(6,GainComputationPolicy::calculateGain(hypergraph,6,0));
	}

	void initializeConfiguration(HypernodeWeight hypergraph_weight) {
		config.initial_partitioning.k = 2;
		config.partition.k = 2;
		config.initial_partitioning.epsilon = 0.05;
		config.partition.epsilon = 0.05;
		config.initial_partitioning.seed = 1;
		config.initial_partitioning.min_ils_iterations = 30;
		config.initial_partitioning.rollback = true;
		config.initial_partitioning.upper_allowed_partition_weight.resize(2);
		config.initial_partitioning.perfect_balance_partition_weight.resize(2);
		for (PartitionID i = 0; i < config.initial_partitioning.k; i++) {
			config.initial_partitioning.perfect_balance_partition_weight[i] =
					ceil(
							hypergraph_weight
									/ static_cast<double>(config.initial_partitioning.k));
		}
	}

	std::vector<PrioQueue*> pq;
	Hypergraph hypergraph;
	Configuration config;

};



TEST_F(AGainComputationTest, ChecksCorrectFMGainComputation) {
	ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph,0,1),-1);
	ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph,1,1),0);
	ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph,2,1),0);
	ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph,3,0),-1);
	ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph,4,0),-1);
	ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph,5,0),0);
	ASSERT_EQ(FMGainComputationPolicy::calculateGain(hypergraph,6,0),-1);
}

TEST_F(AGainComputationTest, ChecksCorrectFMGainsAfterDeltaGainUpdate) {
	hypergraph.initializeNumCutHyperedges();
	pushAllHypernodesIntoQueue<FMGainComputationPolicy>();
	hypergraph.changeNodePart(3,1,0);
	pq[0]->remove(3);
	FMGainComputationPolicy::deltaGainUpdate(hypergraph,pq,3,1,0);
	ASSERT_EQ(pq[1]->key(0),-1);
	ASSERT_EQ(pq[1]->key(1),0);
	ASSERT_EQ(pq[1]->key(2),0);
	ASSERT_EQ(pq[0]->key(4),1);
	ASSERT_EQ(pq[0]->key(5),0);
	ASSERT_EQ(pq[0]->key(6),0);
}

TEST_F(AGainComputationTest, ChecksCorrectMaxPinGainComputation) {
	ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph,0,1),2);
	ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph,1,1),2);
	ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph,2,1),2);
	ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph,3,0),2);
	ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph,4,0),2);
	ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph,5,0),1);
	ASSERT_EQ(MaxPinGainComputationPolicy::calculateGain(hypergraph,6,0),1);
}

TEST_F(AGainComputationTest, ChecksCorrectMaxPinGainsAfterDeltaGainUpdate) {
	hypergraph.initializeNumCutHyperedges();
	pushAllHypernodesIntoQueue<MaxPinGainComputationPolicy>();
	hypergraph.changeNodePart(3,1,0);
	pq[0]->remove(3);
	MaxPinGainComputationPolicy::deltaGainUpdate(hypergraph,pq,3,1,0);
	ASSERT_EQ(pq[1]->key(0),1);
	ASSERT_EQ(pq[1]->key(1),1);
	ASSERT_EQ(pq[1]->key(2),2);
	ASSERT_EQ(pq[0]->key(4),3);
	ASSERT_EQ(pq[0]->key(5),1);
	ASSERT_EQ(pq[0]->key(6),2);
}

TEST_F(AGainComputationTest, ChecksCorrectMaxNetGainComputation) {
	ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph,0,1),1);
	ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph,1,1),1);
	ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph,2,1),1);
	ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph,3,0),1);
	ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph,4,0),1);
	ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph,5,0),1);
	ASSERT_EQ(MaxNetGainComputationPolicy::calculateGain(hypergraph,6,0),1);
}

TEST_F(AGainComputationTest, ChecksCorrectMaxNetGainsAfterDeltaGainUpdate) {
	hypergraph.initializeNumCutHyperedges();
	pushAllHypernodesIntoQueue<MaxNetGainComputationPolicy>();
	hypergraph.changeNodePart(3,1,0);
	pq[0]->remove(3);
	MaxNetGainComputationPolicy::deltaGainUpdate(hypergraph,pq,3,1,0);
	ASSERT_EQ(pq[1]->key(0),1);
	ASSERT_EQ(pq[1]->key(1),1);
	ASSERT_EQ(pq[1]->key(2),1);
	ASSERT_EQ(pq[0]->key(4),2);
	ASSERT_EQ(pq[0]->key(5),1);
	ASSERT_EQ(pq[0]->key(6),2);
}

}


