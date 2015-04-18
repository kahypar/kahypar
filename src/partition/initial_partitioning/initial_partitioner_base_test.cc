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

		partitioner = new InitialPartitionerBase(hypergraph, config);

	}

	InitialPartitionerBase* partitioner;
	Hypergraph hypergraph;
	Configuration config;

};

TEST_F(AInitialPartionerBaseTest, TestTest) {
	partitioner->assignHypernodeToPartition(0,0);
	ASSERT_THAT(hypergraph.partID(2), Eq(0));
}

}


