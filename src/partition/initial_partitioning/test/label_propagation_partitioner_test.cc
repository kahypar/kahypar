/*
 * label_propagation_partitioner_test.cc
 *
 *  Created on: 24.07.2015
 *      Author: theuer
 */
#include "gmock/gmock.h"
#include <vector>
#include <unordered_map>
#include <queue>

#include "lib/io/HypergraphIO.h"
#include "partition/Metrics.h"
#include "partition/initial_partitioning/InitialPartitionerBase.h"
#include "partition/initial_partitioning/LabelPropagationInitialPartitioner.h"
#include "partition/initial_partitioning/RecursiveBisection.h"
#include "partition/initial_partitioning/policies/StartNodeSelectionPolicy.h"
#include "partition/initial_partitioning/policies/GainComputationPolicy.h"

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
	config.initial_partitioning.epsilon = 1.0;
	config.partition.epsilon = 1.0;
	config.initial_partitioning.seed = 1;
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
}

class ALabelPropagationMaxGainMoveTest: public Test {
public:
	ALabelPropagationMaxGainMoveTest() :
			hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
					HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }), config(), partitioner(
					nullptr) {

		PartitionID k = 2;
		initializeConfiguration(config, k, 7);

		partitioner = new LabelPropagationInitialPartitioner<
				BFSStartNodeSelectionPolicy, FMGainComputationPolicy>(
				hypergraph, config);
	}

	LabelPropagationInitialPartitioner<BFSStartNodeSelectionPolicy,
			FMGainComputationPolicy>* partitioner;
	Hypergraph hypergraph;
	Configuration config;

};

TEST_F(ALabelPropagationMaxGainMoveTest, CalculateMaxGainMoveOfAnUnassignedHypernodeIfAllHypernodesAreAssigned) {
	hypergraph.setNodePart(1, 0);
	hypergraph.setNodePart(2, 1);
	hypergraph.setNodePart(3, 0);
	hypergraph.setNodePart(4, 0);
	hypergraph.setNodePart(5, 1);
	hypergraph.setNodePart(6, 1);
	std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(0);
	ASSERT_EQ(max_move.first, 0);
	ASSERT_EQ(max_move.second, -1);
}

TEST_F(ALabelPropagationMaxGainMoveTest, CalculateMaxGainMoveOfAnAssignedHypernodeIfAllHypernodesAreAssigned) {
	hypergraph.setNodePart(0, 0);
	hypergraph.setNodePart(1, 1);
	hypergraph.setNodePart(2, 1);
	hypergraph.setNodePart(3, 1);
	hypergraph.setNodePart(4, 1);
	hypergraph.setNodePart(5, 1);
	hypergraph.setNodePart(6, 1);
	std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(0);
	ASSERT_EQ(max_move.first, 1);
	ASSERT_EQ(max_move.second, 2);
}

TEST_F(ALabelPropagationMaxGainMoveTest, CalculateMaxGainMoveIfThereAStillUnassignedNodesOfAHyperedgeAreLeft) {
	hypergraph.setNodePart(0, 0);
	hypergraph.setNodePart(1, 1);
	hypergraph.setNodePart(3, 1);
	std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(0);
	ASSERT_EQ(max_move.first, 1);
	ASSERT_EQ(max_move.second, 1);
}

TEST_F(ALabelPropagationMaxGainMoveTest, AllMaxGainMovesAZeroGainMovesIfNoHypernodeIsAssigned) {
	for (HypernodeID hn : hypergraph.nodes()) {
		std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(
				hn);
		ASSERT_EQ(max_move.first, -1);
	}
}

TEST_F(ALabelPropagationMaxGainMoveTest, MaxGainMoveIsAZeroGainMoveIfANetHasOnlyOneAssignedHypernode) {
	hypergraph.setNodePart(0, 0);
	const HypernodeID hn = 0;
	std::pair<PartitionID, Gain> max_move = partitioner->computeMaxGainMove(hn);
	ASSERT_EQ(max_move.first, 0);
	ASSERT_EQ(max_move.second, 0);
}


}
;

