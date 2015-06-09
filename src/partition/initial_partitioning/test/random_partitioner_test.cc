#include "gmock/gmock.h"
#include <vector>
#include <unordered_map>
#include <queue>

#include "lib/io/HypergraphIO.h"
#include "lib/datastructure/BucketQueue.h"
#include "lib/datastructure/PriorityQueue.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/RandomInitialPartitioner.h"
#include "partition/initial_partitioning/RecursiveBisection.h"
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
	config.initial_partitioning.min_ils_iterations = 30;
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
}

class ARandomBisectionInitialPartitionerTest: public Test {
public:
	ARandomBisectionInitialPartitionerTest() :
			config(), partitioner(nullptr), hypergraph(nullptr) {

		config.initial_partitioning.coarse_graph_filename =
				"test_instances/ibm01.hgr";

		HypernodeID num_hypernodes;
		HyperedgeID num_hyperedges;
		HyperedgeIndexVector index_vector;
		HyperedgeVector edge_vector;
		HyperedgeWeightVector* hyperedge_weights = NULL;
		HypernodeWeightVector* hypernode_weights = NULL;

		PartitionID k = 2;
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

		partitioner = new RandomInitialPartitioner(
				*hypergraph, config);
	}

	RandomInitialPartitioner* partitioner;
	Hypergraph* hypergraph;
	Configuration config;

};

class AKWayRandomInitialPartitionerTest: public Test {
public:
	AKWayRandomInitialPartitionerTest() :
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

		partitioner = new RandomInitialPartitioner(*hypergraph, config);
	}

	RandomInitialPartitioner* partitioner;
	Hypergraph* hypergraph;
	Configuration config;

};

class ARandomRecursiveBisectionTest: public Test {
public:
	ARandomRecursiveBisectionTest() :
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

		partitioner = new RecursiveBisection<RandomInitialPartitioner>(
				*hypergraph, config);
	}

	RecursiveBisection<RandomInitialPartitioner>* partitioner;
	Hypergraph* hypergraph;
	Configuration config;

};

TEST_F(ARandomBisectionInitialPartitionerTest, HasValidImbalance) {

	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph), config.partition.epsilon);

}

TEST_F(ARandomBisectionInitialPartitionerTest, LeavesNoHypernodeUnassigned) {

	partitioner->partition(config.initial_partitioning.k);

	for (HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn), -1);
	}

}

TEST_F(AKWayRandomInitialPartitionerTest, HasValidImbalance) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph), config.partition.epsilon);

}

TEST_F(AKWayRandomInitialPartitionerTest, HasNoSignificantLowPartitionWeights) {

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

TEST_F(AKWayRandomInitialPartitionerTest, LeavesNoHypernodeUnassigned) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for (HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn), -1);
	}
}

TEST_F(ARandomRecursiveBisectionTest, HasValidImbalance) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	ASSERT_LE(metrics::imbalance(*hypergraph), config.partition.epsilon);

}

TEST_F(ARandomRecursiveBisectionTest, HasNoSignificantLowPartitionWeights) {

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

TEST_F(ARandomRecursiveBisectionTest, LeavesNoHypernodeUnassigned) {

	PartitionID k = 32;
	initializePartitioning(k);
	partitioner->partition(config.initial_partitioning.k);

	for (HypernodeID hn : hypergraph->nodes()) {
		ASSERT_NE(hypergraph->partID(hn), -1);
	}
}

}
;
