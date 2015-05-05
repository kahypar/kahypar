/*
 * bfs_initial_partitioner_test.cc
 *
 *  Created on: 29.04.2015
 *      Author: heuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_INITIAL_PARTITIONER_TEST_CC_
#define SRC_PARTITION_INITIAL_PARTITIONING_INITIAL_PARTITIONER_TEST_CC_


#include "gmock/gmock.h"
#include <vector>
#include <unordered_map>
#include <queue>

#include "lib/io/HypergraphIO.h"
#include "lib/datastructure/BucketQueue.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/initial_partitioner_test_TestFixtures.h"

using ::testing::Eq;
using ::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using defs::PartitionID;

using datastructure::BucketQueue;

using Gain = HyperedgeWeight;

namespace partition {


TEST_F(ABFSInitialPartionerTest, ExpectedBFSBisectionHypernodeToPartitionAssignmentTest) {

	partitioner->partition(2);
	std::vector<HypernodeID> partition_zero {0,1,2,3};
	std::vector<HypernodeID> partition_one {4,5,6};
	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}

}

TEST_F(ABFSInitialPartionerTest, BFSBisectionNoUnassignedHypernodesTest) {

	partitioner->partition(2);

	for(HypernodeID hn : hypergraph.nodes()) {
		ASSERT_NE(hypergraph.partID(hn),-1);
	}

}

TEST_F(ABFSInitialPartionerInstanceTest, KWayBFSImbalanceTest) {

	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph),config.partition.epsilon);

}

TEST_F(ABFSInitialPartionerInstanceTest, KWayBFSLowerBoundPartitionWeightTest) {

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

TEST_F(ABFSInitialPartionerInstanceTest, KWayBFSNoUnassignedHypernodesTest) {

	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(ABFSInitialPartionerTest, BFSInQueueMapUpdateAfterPushingIncidentHypernodesInQueue) {

	std::queue<HypernodeID> q;
	std::unordered_map<HypernodeID,bool> in_queue;
	in_queue[0] = true;
	q.push(0);
	PartitionID tmp = -1;
	partitioner->pushIncidentHyperedgesIntoQueue(q,0,in_queue,tmp);
	for(HypernodeID hn = 0; hn < 5; hn++) {
		ASSERT_TRUE(in_queue[hn]);
	}
	for(HypernodeID hn = 5; hn < 7; hn++) {
		ASSERT_FALSE(in_queue[hn]);
	}

}

TEST_F(ABFSInitialPartionerTest, BFSExpectedHypernodesInQueueAfterPushingIncidentHypernodesInQueue) {

	std::queue<HypernodeID> q;
	std::unordered_map<HypernodeID,bool> in_queue;
	in_queue[0] = true;
	q.push(0);
	PartitionID tmp = -1;
	partitioner->pushIncidentHyperedgesIntoQueue(q,0,in_queue,tmp);
	std::vector<HypernodeID> expected_in_queue {0,2,1,3,4};
	for(unsigned int i = 0; i < expected_in_queue.size(); i++) {
		HypernodeID hn = q.front(); q.pop();
		ASSERT_EQ(hn,expected_in_queue[i]);
	}

	ASSERT_TRUE(q.empty());
}

TEST_F(ARandomInitialPartionerInstanceTest, BisectionRandomImbalanceTest) {

	PartitionID k = 2;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph),config.partition.epsilon);

}

TEST_F(ARandomInitialPartionerInstanceTest, RandomBisectionNoUnassignedHypernodesTest) {

	PartitionID k = 2;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}

}

TEST_F(ARandomInitialPartionerInstanceTest, KWayRandomImbalanceTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph),config.partition.epsilon);

}

TEST_F(ARandomInitialPartionerInstanceTest, KWayRandomLowerBoundPartitionWeightTest) {

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

TEST_F(ARandomInitialPartionerInstanceTest, KWayRandomNoUnassignedHypernodesTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}


TEST_F(AGreedyInitialPartionerTest, ExpectedGreedyBisectionHypernodeToPartitionAssignmentTest) {

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

TEST_F(AGreedyInitialPartionerTest, GreedyBisectionNoUnassignedHypernodesTest) {

	partitioner->partition(2);

	for(HypernodeID hn : hypergraph.nodes()) {
		ASSERT_NE(hypergraph.partID(hn),-1);
	}

}


TEST_F(AGreedyInitialPartionerTest, ExpectedMaxGainValueAndHypernodeAfterPushingSomeHypernodesIntoBucketQueue) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}

	BucketQueue<HypernodeID,Gain> bq(2 * hypergraph.numNodes());
	partitioner->processNodeForBucketPQ(bq,2,0);
	partitioner->processNodeForBucketPQ(bq,4,0);
	partitioner->processNodeForBucketPQ(bq,6,0);
	ASSERT_EQ(bq.size(),3);
	ASSERT_TRUE(bq.getMax() == 2 && bq.getMaxKey() == 0);


}

TEST_F(AGreedyInitialPartionerTest, ExpectedMaxGainValueAfterUpdateAHypernodeIntoBucketQueue) {

	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn != 0) {
			hypergraph.setNodePart(hn,1);
		}
		else {
			hypergraph.setNodePart(hn,0);
		}
	}

	BucketQueue<HypernodeID,Gain> bq(2 * hypergraph.numNodes());
	partitioner->processNodeForBucketPQ(bq,2,0);
	partitioner->processNodeForBucketPQ(bq,4,0);
	partitioner->processNodeForBucketPQ(bq,6,0);


	hypergraph.changeNodePart(2,1,0);
	hypergraph.changeNodePart(4,1,0);
	bq.deleteNode(2);
	bq.deleteNode(4);
	partitioner->processNodeForBucketPQ(bq,6,0);
	ASSERT_TRUE(bq.getMax() == 6 && bq.getMaxKey() == 0);
	ASSERT_EQ(bq.size(), 1);

}

TEST_F(AGreedyInitialPartionerInstanceTest, KWayGreedySequentialImbalanceTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph),config.partition.epsilon);

}

TEST_F(AGreedyInitialPartionerInstanceTest, KWayGreedySequentialLowerBoundPartitionWeightTest) {

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

TEST_F(AGreedyInitialPartionerInstanceTest, KWayGreedySequentialNoUnassignedHypernodesTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AGreedyGlobalInitialPartionerInstanceTest, KWayGreedyGlobalImbalanceTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph),config.partition.epsilon);

}

TEST_F(AGreedyGlobalInitialPartionerInstanceTest, KWayGreedyGlobalLowerBoundPartitionWeightTest) {

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

TEST_F(AGreedyGlobalInitialPartionerInstanceTest, KWayGreedyGlobalNoUnassignedHypernodesTest) {
	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AGreedyRoundRobinInitialPartionerInstanceTest, KWayGreedyRoundRobinImbalanceTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph),config.partition.epsilon);

}

TEST_F(AGreedyRoundRobinInitialPartionerInstanceTest, KWayGreedyRoundRobinLowerBoundPartitionWeightTest) {

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

TEST_F(AGreedyRoundRobinInitialPartionerInstanceTest, KWayGreedyRoundRobinNoUnassignedHypernodesTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(ARecursiveBisectionInstanceTest, KWayRecursiveGreedyImbalanceTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph),config.partition.epsilon);

}

TEST_F(ARecursiveBisectionInstanceTest, KWayRecursiveGreedyLowerBoundPartitionWeightTest) {

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

TEST_F(ARecursiveBisectionInstanceTest, KWayRecursiveGreedyNoUnassignedHypernodesTest) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}




}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIAL_PARTITIONER_TEST_CC_ */
