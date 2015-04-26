/*
 * initial_partitioner_base_test.cc
 *
 *  Created on: Apr 17, 2015
 *      Author: theuer
 */

#include "gmock/gmock.h"

#include "partition/initial_partitioning/InitialPartitionerBase.h"

using ::testing::Eq;
using ::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;

namespace partition {

class AInitialPartionerBaseTest: public Test {
public:
	AInitialPartionerBaseTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), partitioner(
					nullptr) {


		initializeConfiguration();
		partitioner = new InitialPartitionerBase(hypergraph, config);
		partitioner->recalculateBalanceConstraints();
	}

	void initializeConfiguration() {
		config.initial_partitioning.k = 2;
		config.partition.k = 2;
		config.initial_partitioning.epsilon = 0.05;
		config.initial_partitioning.seed = 1;
		config.initial_partitioning.ils_iterations = 30;
		config.initial_partitioning.rollback = true;
		config.initial_partitioning.upper_allowed_partition_weight.resize(2);
		config.initial_partitioning.lower_allowed_partition_weight.resize(2);
	}

	InitialPartitionerBase* partitioner;
	Hypergraph hypergraph;
	Configuration config;

};

TEST_F(AInitialPartionerBaseTest, AAssignHypernodesToPartitionTest) {
	//Assign hypernodes
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(0,0));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(1,0));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(2,0));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(3,0));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(4,1));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(5,1));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(6,1));


	//Check, if all hypernodes are assigned correctly
	ASSERT_EQ(hypergraph.partID(0),0);
	ASSERT_EQ(hypergraph.partID(1),0);
	ASSERT_EQ(hypergraph.partID(2),0);
	ASSERT_EQ(hypergraph.partID(3),0);
	ASSERT_EQ(hypergraph.partID(4),1);
	ASSERT_EQ(hypergraph.partID(5),1);
	ASSERT_EQ(hypergraph.partID(6),1);

	//Changing hypernode partition id
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(3,1));
	ASSERT_FALSE(partitioner->assignHypernodeToPartition(3,1));
}

TEST_F(AInitialPartionerBaseTest, ARollbackToBestBisectionCutTest) {
	//Assign hypernodes
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(0,0,-1,true));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(1,0,-1,true));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(2,0,-1,true));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(3,0,-1,true));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(4,1));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(5,1));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(6,1));

	//Perform rollback to best bisection cut
	HyperedgeWeight cut_before = metrics::hyperedgeCut(hypergraph);
	partitioner->rollbackToBestBisectionCut();
	HyperedgeWeight cut_after = metrics::hyperedgeCut(hypergraph);

	//The best bisection cut is 2 (0,1,2 -> 0 and 3,4,5,6 -> 1)
	ASSERT_TRUE(cut_after < cut_before);
	ASSERT_EQ(cut_after,2);
	for(HypernodeID hn : hypergraph.nodes()) {
		if(hn <= 2)
			ASSERT_EQ(hypergraph.partID(hn), 0);
		else
			ASSERT_EQ(hypergraph.partID(hn), 1);
	}

}

TEST_F(AInitialPartionerBaseTest, AExtractingHypergraphPartitionTest) {

	//Assign hypernodes
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(0,0));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(1,0));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(2,0));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(3,0));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(4,1));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(5,1));
	ASSERT_TRUE(partitioner->assignHypernodeToPartition(6,1));

	//Extract partition 0
	HypernodeID num_hypernodes_0;
	HyperedgeID num_hyperedges_0;
	HyperedgeIndexVector index_vector_0;
	HyperedgeVector edge_vector_0;
	HyperedgeWeightVector hyperedge_weights_0;
	HypernodeWeightVector hypernode_weights_0;
	std::vector<HypernodeID> hgToExtractedHypergraphMapper_0;
	partitioner->extractPartitionAsHypergraph(hypergraph, 0,
			num_hypernodes_0, num_hyperedges_0, index_vector_0,
			edge_vector_0, hyperedge_weights_0, hypernode_weights_0,
			hgToExtractedHypergraphMapper_0);
	Hypergraph partition_0(num_hypernodes_0, num_hyperedges_0,
			index_vector_0, edge_vector_0, config.initial_partitioning.k,
			&hyperedge_weights_0, &hypernode_weights_0);

	//Checking hypernode and hyperedge count
	ASSERT_EQ(partition_0.numNodes(),4);
	ASSERT_EQ(partition_0.numEdges(),1);

	//Testing hypernode mapping
	std::vector<HypernodeID> test_node {0,1,2,3}; int i = 0;
	for(HypernodeID hn : partition_0.nodes()) {
		ASSERT_EQ( hgToExtractedHypergraphMapper_0[hn], test_node[i]);
		ASSERT_EQ(partition_0.nodeWeight(hn), hypergraph.nodeWeight(hgToExtractedHypergraphMapper_0[hn]));
		i++;
	}

	//Testing extract hyperedge
	std::vector<HypernodeID> test_node_in_edge {0,2}; i = 0;
	for(HypernodeID hn : partition_0.pins(0)) {
			ASSERT_EQ(hn,test_node_in_edge[i]);
			i++;
	}

	//Extract partition 1
	HypernodeID num_hypernodes_1;
	HyperedgeID num_hyperedges_1;
	HyperedgeIndexVector index_vector_1;
	HyperedgeVector edge_vector_1;
	HyperedgeWeightVector hyperedge_weights_1;
	HypernodeWeightVector hypernode_weights_1;
	std::vector<HypernodeID> hgToExtractedHypergraphMapper_1;
	partitioner->extractPartitionAsHypergraph(hypergraph, 1,
			num_hypernodes_1, num_hyperedges_1, index_vector_1,
			edge_vector_1, hyperedge_weights_1, hypernode_weights_1,
			hgToExtractedHypergraphMapper_1);
	Hypergraph partition_1(num_hypernodes_1, num_hyperedges_1,
			index_vector_1, edge_vector_1, config.initial_partitioning.k,
			&hyperedge_weights_1, &hypernode_weights_1);

	//Checking hypernode and hyperedge count
	ASSERT_EQ(partition_1.numNodes(),3);
	ASSERT_EQ(partition_1.numEdges(),0);

	//Testing hypernode mapping
	std::vector<HypernodeID> test_node_2 {4,5,6}; i = 0;
	for(HypernodeID hn : partition_1.nodes()) {
		ASSERT_EQ( hgToExtractedHypergraphMapper_1[hn], test_node_2[i]);
		ASSERT_EQ(partition_1.nodeWeight(hn), hypergraph.nodeWeight(hgToExtractedHypergraphMapper_1[hn]));
		i++;
	}

}

}


