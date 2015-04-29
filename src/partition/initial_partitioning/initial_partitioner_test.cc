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

#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/BFSInitialPartitioner.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"

using ::testing::Eq;
using ::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using defs::PartitionID;

using partition::BFSInitialPartitioner;
using partition::TestStartNodeSelectionPolicy;

namespace partition {

void initializeConfiguration(Configuration& config, HypernodeWeight hypergraph_weight) {
	config.initial_partitioning.k = 2;
	config.partition.k = 2;
	config.initial_partitioning.epsilon = 0.05;
	config.partition.epsilon = 0.05;
	config.initial_partitioning.seed = 1;
	config.initial_partitioning.ils_iterations = 30;
	config.initial_partitioning.rollback = false;
	config.initial_partitioning.refinement = false;
	config.initial_partitioning.upper_allowed_partition_weight.resize(config.initial_partitioning.k);
	config.initial_partitioning.lower_allowed_partition_weight.resize(config.initial_partitioning.k);
	for (int i = 0; i < config.initial_partitioning.k; i++) {
		config.initial_partitioning.lower_allowed_partition_weight[i] =
		ceil(
				hypergraph_weight
				/ static_cast<double>(config.initial_partitioning.k))
		* (1.0 - config.partition.epsilon);
		config.initial_partitioning.upper_allowed_partition_weight[i] =
		ceil(
				hypergraph_weight
				/ static_cast<double>(config.initial_partitioning.k))
		* (1.0 + config.partition.epsilon);
	}
}

class ABFSInitialPartionerTest: public Test {
public:
	ABFSInitialPartionerTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), partitioner(
					nullptr) {


		initializeConfiguration(config, 7);
		partitioner = new BFSInitialPartitioner<TestStartNodeSelectionPolicy>(hypergraph, config);
	}

	BFSInitialPartitioner<TestStartNodeSelectionPolicy>* partitioner;
	Hypergraph hypergraph;
	Configuration config;

};

TEST_F(ABFSInitialPartionerTest, AInitialPartionerBaseTest) {

	partitioner->partition(2);
	std::vector<HypernodeID> partition_zero {0,1,2,3};
	std::vector<HypernodeID> partition_one {4,5,6};
	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}

	ASSERT_LE(hypergraph.partWeight(0),config.initial_partitioning.upper_allowed_partition_weight[0]);
	ASSERT_LE(hypergraph.partWeight(1),config.initial_partitioning.upper_allowed_partition_weight[1]);

}

TEST_F(ABFSInitialPartionerTest, APushHyperedgesIntoQueueTest) {

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
	std::vector<HypernodeID> expected_in_queue {0,2,1,3,4};
	for(unsigned int i = 0; i < expected_in_queue.size(); i++) {
		HypernodeID hn = q.front(); q.pop();
		ASSERT_EQ(hn,expected_in_queue[i]);
	}

	ASSERT_TRUE(q.empty());


}



}


#endif /* SRC_PARTITION_INITIAL_PARTITIONING_INITIAL_PARTITIONER_TEST_CC_ */
