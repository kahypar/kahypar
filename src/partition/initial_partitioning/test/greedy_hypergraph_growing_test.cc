/*
 * greedy_hypergraph_growing_test.cc
 *
 *  Created on: 21.05.2015
 *      Author: theuer
 */

#include "gmock/gmock.h"
#include <vector>
#include <unordered_map>
#include <queue>

#include "lib/io/HypergraphIO.h"
#include "lib/datastructure/BucketQueue.h"
#include "lib/datastructure/PriorityQueue.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/test/greedy_hypergraph_growing_TestFixtures.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/datastructure/PriorityQueue.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"


using ::testing::Eq;
using ::testing::Test;

using datastructure::NoDataBinaryMaxHeap;
using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using defs::PartitionID;


using datastructure::BucketQueue;
using datastructure::PriorityQueue;


using Gain = HyperedgeWeight;

using TwoWayFMHeap = NoDataBinaryMaxHeap<HypernodeID, Gain,
                                         std::numeric_limits<HyperedgeWeight> >;

namespace partition {

TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, ChecksCorrectMaxGainValueAndHypernodeAfterPushingSomeHypernodesIntoPriorityQueue) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}

	PriorityQueue<TwoWayFMHeap>* bq = new PriorityQueue<TwoWayFMHeap>(2 * hypergraph.numNodes());
	base->processNodeForBucketPQ(*bq,2,0,true);
	base->processNodeForBucketPQ(*bq,4,0,true);
	base->processNodeForBucketPQ(*bq,6,0,true);
	ASSERT_EQ(bq->size(),3);
	ASSERT_TRUE(bq->max() == 2 && bq->maxKey() == 0);
}



TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, ChecksCorrectGainValueAfterUpdatePriorityQueue) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}
	hypergraph.initializeNumCutHyperedges();
	base->processNodeForBucketPQ(*pq[1],0,1,true);
	ASSERT_EQ(pq[1]->max(),0);
	ASSERT_EQ(pq[1]->maxKey(),2);
	hypergraph.changeNodePart(2,1,0);
	base->processNodeForBucketPQ(*pq[1],0,1,true);
	ASSERT_EQ(pq[1]->max(),0);
	ASSERT_EQ(pq[1]->maxKey(),0);
}

TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, ChecksCorrectMaxGainValueAfterDeltaGainUpdate) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}
	hypergraph.initializeNumCutHyperedges();
	base->processNodeForBucketPQ(*pq[1],0,1,false);
	base->processNodeForBucketPQ(*pq[0],2,0,false);
	base->processNodeForBucketPQ(*pq[0],4,0,false);
	base->processNodeForBucketPQ(*pq[0],6,0,false);

	hypergraph.changeNodePart(2,1,0);
	base->deleteNodeInAllBucketQueues(pq,2);
	FMGainComputationPolicy::deltaGainUpdate(hypergraph,pq,2,1,0);

	hypergraph.changeNodePart(4,1,0);
	base->deleteNodeInAllBucketQueues(pq,4);
	FMGainComputationPolicy::deltaGainUpdate(hypergraph,pq,4,1,0);

	ASSERT_TRUE(pq[0]->max() == 6 && pq[0]->maxKey() == 0);
	ASSERT_EQ(pq[0]->size(), 1);

}

TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, GetCorrectMaxGainHypernodeAndPartition) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}

	config.initial_partitioning.unassigned_part = 0;
	config.initial_partitioning.upper_allowed_partition_weight[0] = 7;
	config.initial_partitioning.upper_allowed_partition_weight[1] = 7;

	base->processNodeForBucketPQ(*pq[1],0,1,false);
	base->processNodeForBucketPQ(*pq[0],2,0,false);
	base->processNodeForBucketPQ(*pq[0],4,0,false);
	base->processNodeForBucketPQ(*pq[0],6,0,false);
	std::pair<HypernodeID,PartitionID> max_gain_move = base->getMaxGainHypernode(pq);
	ASSERT_EQ(max_gain_move.first,0);
	ASSERT_EQ(max_gain_move.second, 1);
	ASSERT_EQ(pq[max_gain_move.second]->maxKey(),2);
}

TEST_F(AGreedyHypergraphGrowingBaseFunctionTest, DeletesAssignedHypernodesFromPriorityQueue) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}

	config.initial_partitioning.unassigned_part = 0;
	config.initial_partitioning.upper_allowed_partition_weight[0] = 7;
	config.initial_partitioning.upper_allowed_partition_weight[1] = 7;

	base->processNodeForBucketPQ(*pq[1],0,1,false);
	base->processNodeForBucketPQ(*pq[0],2,0,false);
	base->processNodeForBucketPQ(*pq[0],4,0,false);
	base->processNodeForBucketPQ(*pq[0],6,0,false);
	base->deleteNodeInAllBucketQueues(pq,0);
	ASSERT_EQ(pq[0]->size(),3);
	ASSERT_EQ(pq[1]->size(),0);
}



TEST_F(AGreedySequentialBisectionTest, ExpectedGreedySequentialBisectionResult) {

	partitioner->partition(2);
	std::vector<HypernodeID> partition_zero {0,1,2,5};
	std::vector<HypernodeID> partition_one {3,4,6};

	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}
}

TEST_F(AGreedyGlobalBisectionTest, ExpectedGreedyGlobalBisectionResult) {

	partitioner->partition(2);
	std::vector<HypernodeID> partition_zero {0,1,2,5};
	std::vector<HypernodeID> partition_one {3,4,6};

	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}
}

TEST_F(AGreedyRoundRobinBisectionTest, ExpectedGreedyRoundRobinBisectionResult) {

	partitioner->partition(2);
	std::vector<HypernodeID> partition_zero {0,1,2,4};
	std::vector<HypernodeID> partition_one {3,5,6};

	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}
}


TEST_F(AKWayGreedySequentialTest, HasValidImbalance) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph,config.initial_partitioning.k),config.partition.epsilon);

}

TEST_F(AKWayGreedySequentialTest, HasNoSignificantLowPartitionWeights) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	//Upper bounds of maximum partition weight should not be exceeded.
	HypernodeWeight heaviest_part = 0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		if(heaviest_part < hypergraph->partWeight(k)) {
			heaviest_part = hypergraph->partWeight(k);
		}
	}

	//No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
	double lower_bound_factor = 50.0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		ASSERT_GE(hypergraph->partWeight(k),(lower_bound_factor/100.0)*heaviest_part);
	}

}

TEST_F(AKWayGreedySequentialTest, LeavesNoHypernodeUnassigned) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AKWayGreedyGlobalTest, HasValidImbalance) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph,config.initial_partitioning.k),config.partition.epsilon);

}

TEST_F(AKWayGreedyGlobalTest, HasNoSignificantLowPartitionWeights) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	//Upper bounds of maximum partition weight should not be exceeded.
	HypernodeWeight heaviest_part = 0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		if(heaviest_part < hypergraph->partWeight(k)) {
			heaviest_part = hypergraph->partWeight(k);
		}
	}

	//No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
	double lower_bound_factor = 50.0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		ASSERT_GE(hypergraph->partWeight(k),(lower_bound_factor/100.0)*heaviest_part);
	}

}

TEST_F(AKWayGreedyGlobalTest, LeavesNoHypernodeUnassigned) {
	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AKWayGreedyRoundRobinTest, HasValidImbalance) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph,config.initial_partitioning.k),config.partition.epsilon);

}

TEST_F(AKWayGreedyRoundRobinTest, HasNoSignificantLowPartitionWeights) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	//Upper bounds of maximum partition weight should not be exceeded.
	HypernodeWeight heaviest_part = 0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		if(heaviest_part < hypergraph->partWeight(k)) {
			heaviest_part = hypergraph->partWeight(k);
		}
	}

	//No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
	double lower_bound_factor = 50.0;
	for(PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		ASSERT_GE(hypergraph->partWeight(k),(lower_bound_factor/100.0)*heaviest_part);
	}

}

TEST_F(AKWayGreedyRoundRobinTest, LeavesNoHypernodeUnassigned) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AGreedyRecursiveBisectionTest, HasValidImbalance) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph,config.initial_partitioning.k), config.partition.epsilon);

}

TEST_F(AGreedyRecursiveBisectionTest, HasNoSignificantLowPartitionWeights) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	//Upper bounds of maximum partition weight should not be exceeded.
	HypernodeWeight heaviest_part = 0;
	for (PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		if (heaviest_part < hypergraph->partWeight(k)) {
			heaviest_part = hypergraph->partWeight(k);
		}
	}

	//No partition weight should fall below under "lower_bound_factor" percent of the heaviest partition weight.
	double lower_bound_factor = 50.0;
	for (PartitionID k = 0; k < config.initial_partitioning.k; k++) {
		ASSERT_GE(hypergraph->partWeight(k),
				(lower_bound_factor / 100.0) * heaviest_part);
	}

}

TEST_F(AGreedyRecursiveBisectionTest, LeavesNoHypernodeUnassigned) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for (HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn), -1);
	}
}

};



