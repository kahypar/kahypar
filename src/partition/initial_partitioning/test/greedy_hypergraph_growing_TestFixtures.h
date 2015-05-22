/*
 * greedy_hypergraph_growing_TestFixtures.h
 *
 *  Created on: Apr 29, 2015
 *      Author: theuer
 */

#ifndef SRC_PARTITION_INITIAL_PARTITIONING_GREEDY_HYPERGRAPH_GROWING_TESTFIXTURES_H_
#define SRC_PARTITION_INITIAL_PARTITIONING_GREEDY_HYPERGRAPH_GROWING_TESTFIXTURES_H_

#include <vector>

#include "gmock/gmock.h"

#include "lib/io/HypergraphIO.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/IInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingBaseFunctions.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingSequentialInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingGlobalInitialPartitioner.h"
#include "partition/initial_partitioning/GreedyHypergraphGrowingRoundRobinInitialPartitioner.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"
#include "partition/initial_partitioning/RecursiveBisection.h"



using ::testing::Test;

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
	config.initial_partitioning.erase_components = false;
	config.initial_partitioning.balance = true;
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
	Randomize::setSeed(config.initial_partitioning.seed);
}

class AGreedyHypergraphGrowingBaseFunctionTest: public Test {
public:
	AGreedyHypergraphGrowingBaseFunctionTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, 12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), base(
					nullptr) {

		PartitionID k = 2;
		initializeConfiguration(config, k, 7);

		base = new GreedyHypergraphGrowingBaseFunctions< FMGainComputationPolicy>(
				hypergraph, config);
		pq.resize(2);
		pq[0] = new PrioQueue(hypergraph.numNodes());
		pq[1] = new PrioQueue(hypergraph.numNodes());
	}

	std::vector<PrioQueue*> pq;
	GreedyHypergraphGrowingBaseFunctions<FMGainComputationPolicy>* base;
	Hypergraph hypergraph;
	Configuration config;

};



class AGreedySequentialBisectionTest: public Test {
public:
	AGreedySequentialBisectionTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, 12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), partitioner(
					nullptr) {

		PartitionID k = 2;
		initializeConfiguration(config, k, 7);

		partitioner = new GreedyHypergraphGrowingSequentialInitialPartitioner<
				TestStartNodeSelectionPolicy, FMGainComputationPolicy>(
				hypergraph, config);
	}

	GreedyHypergraphGrowingSequentialInitialPartitioner<
			TestStartNodeSelectionPolicy, FMGainComputationPolicy>* partitioner;
	Hypergraph hypergraph;
	Configuration config;

};

class AGreedyGlobalBisectionTest: public Test {
public:
	AGreedyGlobalBisectionTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, 12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), partitioner(
					nullptr) {

		PartitionID k = 2;
		initializeConfiguration(config, k, 7);

		partitioner = new GreedyHypergraphGrowingGlobalInitialPartitioner<
				TestStartNodeSelectionPolicy, FMGainComputationPolicy>(
				hypergraph, config);
	}

	GreedyHypergraphGrowingGlobalInitialPartitioner<
			TestStartNodeSelectionPolicy, FMGainComputationPolicy>* partitioner;
	Hypergraph hypergraph;
	Configuration config;

};

class AGreedyRoundRobinBisectionTest: public Test {
public:
	AGreedyRoundRobinBisectionTest() :
			hypergraph(7, 4,
					HyperedgeIndexVector { 0, 2, 6, 9, 12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), partitioner(
					nullptr) {

		PartitionID k = 2;
		initializeConfiguration(config, k, 7);

		partitioner = new GreedyHypergraphGrowingRoundRobinInitialPartitioner<
				TestStartNodeSelectionPolicy, FMGainComputationPolicy>(
				hypergraph, config);
	}

	GreedyHypergraphGrowingRoundRobinInitialPartitioner<
			TestStartNodeSelectionPolicy, FMGainComputationPolicy>* partitioner;
	Hypergraph hypergraph;
	Configuration config;

};


class AKWayGreedySequentialTest: public Test {
public:
	AKWayGreedySequentialTest() :
			config(), partitioner(nullptr), hypergraph(nullptr) {
	}

	void initializePartitioning(PartitionID k) {
		config.initial_partitioning.coarse_graph_filename =
				"test_instances/ibm01.hgr";
		HypernodeID num_hypernodes;
		HyperedgeID num_hyperedges;
		HyperedgeIndexVector index_vector;
		HyperedgeVector edge_vector;
		HyperedgeWeightVector* hyperedge_weights = NULL;
		HypernodeWeightVector* hypernode_weights = NULL;

		io::readHypergraphFile(
				config.initial_partitioning.coarse_graph_filename,
				num_hypernodes, num_hyperedges, index_vector, edge_vector,
				hyperedge_weights, hypernode_weights);
		hypergraph = new Hypergraph(num_hypernodes, num_hyperedges,
				index_vector, edge_vector, k, hyperedge_weights,
				hypernode_weights);
		HypernodeWeight hypergraph_weight = 0;
		for (HypernodeID hn : hypergraph->nodes()) {
			hypergraph_weight += hypergraph->nodeWeight(hn);
		}
		initializeConfiguration(config, k, hypergraph_weight);

		partitioner = new GreedyHypergraphGrowingSequentialInitialPartitioner<
				TestStartNodeSelectionPolicy, FMGainComputationPolicy>(
				*hypergraph, config);
	}

	GreedyHypergraphGrowingSequentialInitialPartitioner<
			TestStartNodeSelectionPolicy, FMGainComputationPolicy>* partitioner;
	Hypergraph* hypergraph;
	Configuration config;

};

class AKWayGreedyGlobalTest: public Test {
public:
	AKWayGreedyGlobalTest() :
			config(), partitioner(nullptr), hypergraph(nullptr) {
	}

	void initializePartitioning(PartitionID k) {
		config.initial_partitioning.coarse_graph_filename =
				"test_instances/ibm01.hgr";

		HypernodeID num_hypernodes;
		HyperedgeID num_hyperedges;
		HyperedgeIndexVector index_vector;
		HyperedgeVector edge_vector;
		HyperedgeWeightVector* hyperedge_weights = NULL;
		HypernodeWeightVector* hypernode_weights = NULL;
		io::readHypergraphFile(
				config.initial_partitioning.coarse_graph_filename,
				num_hypernodes, num_hyperedges, index_vector, edge_vector,
				hyperedge_weights, hypernode_weights);
		hypergraph = new Hypergraph(num_hypernodes, num_hyperedges,
				index_vector, edge_vector, k, hyperedge_weights,
				hypernode_weights);
		HypernodeWeight hypergraph_weight = 0;
		for (HypernodeID hn : hypergraph->nodes()) {
			hypergraph_weight += hypergraph->nodeWeight(hn);
		}
		initializeConfiguration(config, k, hypergraph_weight);

		partitioner = new GreedyHypergraphGrowingGlobalInitialPartitioner<
				TestStartNodeSelectionPolicy, FMGainComputationPolicy>(
				*hypergraph, config);
	}

	GreedyHypergraphGrowingGlobalInitialPartitioner<
			TestStartNodeSelectionPolicy, FMGainComputationPolicy>* partitioner;
	Hypergraph* hypergraph;
	Configuration config;

};

class AKWayGreedyRoundRobinTest: public Test {
public:
	AKWayGreedyRoundRobinTest() :
			config(), partitioner(nullptr), hypergraph(nullptr) {
	}

	void initializePartitioning(PartitionID k) {
		config.initial_partitioning.coarse_graph_filename =
				"test_instances/ibm01.hgr";

		HypernodeID num_hypernodes;
		HyperedgeID num_hyperedges;
		HyperedgeIndexVector index_vector;
		HyperedgeVector edge_vector;
		HyperedgeWeightVector* hyperedge_weights = NULL;
		HypernodeWeightVector* hypernode_weights = NULL;

		io::readHypergraphFile(
				config.initial_partitioning.coarse_graph_filename,
				num_hypernodes, num_hyperedges, index_vector, edge_vector,
				hyperedge_weights, hypernode_weights);
		hypergraph = new Hypergraph(num_hypernodes, num_hyperedges,
				index_vector, edge_vector, k, hyperedge_weights,
				hypernode_weights);

		HypernodeWeight hypergraph_weight = 0;
		for (HypernodeID hn : hypergraph->nodes()) {
			hypergraph_weight += hypergraph->nodeWeight(hn);
		}
		initializeConfiguration(config, k, hypergraph_weight);

		partitioner = new GreedyHypergraphGrowingRoundRobinInitialPartitioner<
				TestStartNodeSelectionPolicy, FMGainComputationPolicy>(
				*hypergraph, config);
	}

	GreedyHypergraphGrowingRoundRobinInitialPartitioner<
			TestStartNodeSelectionPolicy, FMGainComputationPolicy>* partitioner;
	Hypergraph* hypergraph;
	Configuration config;

};

class AGreedyRecursiveBisectionTest: public Test {
public:
	AGreedyRecursiveBisectionTest() :
			config(), partitioner(nullptr), hypergraph(nullptr) {
	}

	void initializePartitioning(PartitionID k) {
		config.initial_partitioning.coarse_graph_filename =
				"test_instances/ibm01.hgr";

		HypernodeID num_hypernodes;
		HyperedgeID num_hyperedges;
		HyperedgeIndexVector index_vector;
		HyperedgeVector edge_vector;
		HyperedgeWeightVector* hyperedge_weights = NULL;
		HypernodeWeightVector* hypernode_weights = NULL;

		io::readHypergraphFile(
				config.initial_partitioning.coarse_graph_filename,
				num_hypernodes, num_hyperedges, index_vector, edge_vector,
				hyperedge_weights, hypernode_weights);
		hypergraph = new Hypergraph(num_hypernodes, num_hyperedges,
				index_vector, edge_vector, k, hyperedge_weights,
				hypernode_weights);

		HypernodeWeight hypergraph_weight = 0;
		for (HypernodeID hn : hypergraph->nodes()) {
			hypergraph_weight += hypergraph->nodeWeight(hn);
		}
		initializeConfiguration(config, k, hypergraph_weight);

		partitioner = new RecursiveBisection<GreedyHypergraphGrowingGlobalInitialPartitioner<
				TestStartNodeSelectionPolicy, FMGainComputationPolicy>>(
				*hypergraph, config);
	}

	RecursiveBisection<GreedyHypergraphGrowingGlobalInitialPartitioner<
	TestStartNodeSelectionPolicy, FMGainComputationPolicy>>* partitioner;
	Hypergraph* hypergraph;
	Configuration config;

};



}

#endif /* SRC_PARTITION_INITIAL_PARTITIONING_GREEDY_HYPERGRAPH_GROWING_TESTFIXTURES_H_ */
