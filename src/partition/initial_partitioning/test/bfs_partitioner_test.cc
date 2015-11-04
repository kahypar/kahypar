/*
 * bfs_partitioner_test.cc
 *
 *  Created on: 21.05.2015
 *      Author: theuer
 */


#include "gmock/gmock.h"
#include <vector>
#include <unordered_map>
#include <queue>

#include "lib/io/HypergraphIO.h"
#include "partition/Metrics.h"
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



namespace partition {

void initializeConfiguration(Configuration& config, PartitionID k,
		HypernodeWeight hypergraph_weight) {
	config.initial_partitioning.k = k;
	config.partition.k = k;
	config.initial_partitioning.epsilon = 0.05;
	config.partition.epsilon = 0.05;
	config.initial_partitioning.seed = 1;
    config.initial_partitioning.unassigned_part = 1;
	config.initial_partitioning.rollback = false;
	config.initial_partitioning.refinement = false;
	config.initial_partitioning.upper_allowed_partition_weight.resize(
			config.initial_partitioning.k);
	config.initial_partitioning.perfect_balance_partition_weight.resize(
			config.initial_partitioning.k);
	for (int i = 0; i < config.initial_partitioning.k; i++) {
		config.initial_partitioning.perfect_balance_partition_weight[i] = ceil(
				hypergraph_weight
						/ static_cast<double>(config.initial_partitioning.k));
		config.initial_partitioning.upper_allowed_partition_weight[i] = ceil(
				hypergraph_weight
						/ static_cast<double>(config.initial_partitioning.k))
				* (1.0 + config.partition.epsilon);
	}
	config.partition.perfect_balance_part_weights[0] = config.initial_partitioning.perfect_balance_partition_weight[0];
	config.partition.perfect_balance_part_weights[1] = config.initial_partitioning.perfect_balance_partition_weight[1];
	config.partition.max_part_weights[0] = config.initial_partitioning.upper_allowed_partition_weight[0];
	config.partition.max_part_weights[1] = config.initial_partitioning.upper_allowed_partition_weight[1];
}

class ABFSBisectionInitialPartionerTest: public Test {
public:
	ABFSBisectionInitialPartionerTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, 12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), partitioner(
					nullptr) {

		PartitionID k = 2;
		initializeConfiguration(config, k, 7);

		partitioner = new BFSInitialPartitioner<TestStartNodeSelectionPolicy>(
				hypergraph, config);
	}

	BFSInitialPartitioner<TestStartNodeSelectionPolicy>* partitioner;
	Hypergraph hypergraph;
	Configuration config;

};

class AKWayBFSInitialPartitionerTest: public Test {
public:
	AKWayBFSInitialPartitionerTest() :
			config(), partitioner(nullptr), hypergraph(nullptr) {

		std::string coarse_graph_filename =
				"test_instances/ibm01.hgr";

		HypernodeID num_hypernodes;
		HyperedgeID num_hyperedges;
		HyperedgeIndexVector index_vector;
		HyperedgeVector edge_vector;
		HyperedgeWeightVector hyperedge_weights;
		HypernodeWeightVector hypernode_weights;

		PartitionID k = 4;
		io::readHypergraphFile(
				coarse_graph_filename,
				num_hypernodes, num_hyperedges, index_vector, edge_vector,
				&hyperedge_weights, &hypernode_weights);
		hypergraph = new Hypergraph(num_hypernodes, num_hyperedges,
				index_vector, edge_vector, k, &hyperedge_weights,
				&hypernode_weights);

		HypernodeWeight hypergraph_weight = 0;
		for (HypernodeID hn : hypergraph->nodes()) {
			hypergraph_weight += hypergraph->nodeWeight(hn);
		}
		initializeConfiguration(config, k, hypergraph_weight);

		partitioner = new BFSInitialPartitioner<TestStartNodeSelectionPolicy>(
				*hypergraph, config);
	}

	BFSInitialPartitioner<TestStartNodeSelectionPolicy>* partitioner;
	Hypergraph* hypergraph;
	Configuration config;

};



TEST_F(ABFSBisectionInitialPartionerTest, ChecksCorrectBisectionCut) {

	partitioner->partition(hypergraph,config);
	std::vector<HypernodeID> partition_zero {0,1,2,3};
	std::vector<HypernodeID> partition_one {4,5,6};
	for(unsigned int i = 0; i < partition_zero.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_zero[i]),0);
	}
	for(unsigned int i = 0; i < partition_one.size(); i++) {
		ASSERT_EQ(hypergraph.partID(partition_one[i]),1);
	}

}



TEST_F(ABFSBisectionInitialPartionerTest, LeavesNoHypernodeUnassigned) {

	partitioner->partition(hypergraph,config);

	for(HypernodeID hn : hypergraph.nodes()) {
		ASSERT_NE(hypergraph.partID(hn),-1);
	}

}

TEST_F(ABFSBisectionInitialPartionerTest, HasCorrectInQueueMapValuesAfterPushingIncidentHypernodesNodesIntoQueue) {

	std::queue<HypernodeID> q;
	std::vector<bool> in_queue(7,false);
	std::vector<bool> hyperedge_in_queue(7,false);
	in_queue[0] = true;
	q.push(0);
	config.initial_partitioning.unassigned_part = -1;
	partitioner->pushIncidentHypernodesIntoQueue(q,0,in_queue,hyperedge_in_queue);
	for(HypernodeID hn = 0; hn < 5; hn++) {
		ASSERT_TRUE(in_queue[hn]);
	}
	for(HypernodeID hn = 5; hn < 7; hn++) {
		ASSERT_FALSE(in_queue[hn]);
	}

}

TEST_F(ABFSBisectionInitialPartionerTest, HasCorrectHypernodesIntoQueueAfterPushingIncidentHypernodesIntoQueue) {

	std::queue<HypernodeID> q;
	std::vector<bool> in_queue(7,false);
	std::vector<bool> hyperedge_in_queue(7,false);
	in_queue[0] = true;
	q.push(0);
	config.initial_partitioning.unassigned_part = -1;
	partitioner->pushIncidentHypernodesIntoQueue(q,0,in_queue,hyperedge_in_queue);
	std::vector<HypernodeID> expected_in_queue {0,2,1,3,4};
	for(unsigned int i = 0; i < expected_in_queue.size(); i++) {
		HypernodeID hn = q.front(); q.pop();
		ASSERT_EQ(hn,expected_in_queue[i]);
	}

	ASSERT_TRUE(q.empty());
}

TEST_F(AKWayBFSInitialPartitionerTest, HasValidImbalance) {

	partitioner->partition(*hypergraph,config);

	ASSERT_LE(metrics::imbalance(*hypergraph,config),config.partition.epsilon);

}

TEST_F(AKWayBFSInitialPartitionerTest, HasNoSignificantLowPartitionWeights) {

	partitioner->partition(*hypergraph,config);

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

TEST_F(AKWayBFSInitialPartitionerTest, LeavesNoHypernodeUnassigned) {

	partitioner->partition(*hypergraph,config);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}

TEST_F(AKWayBFSInitialPartitionerTest, GrowPartitionOnPartitionMinus1) {
	config.initial_partitioning.unassigned_part = -1;
	partitioner->partition(*hypergraph,config);

	for(HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn),-1);
	}
}


};






