#include "gmock/gmock.h"

#include "TwoWayFMRefiner.h"
#include "../lib/datastructure/Hypergraph.h"

namespace partition {
using ::testing::Test;
using ::testing::Eq;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;

class ATwoWayFMRefiner : public Test {
 public:
  ATwoWayFMRefiner() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      refiner(hypergraph) {
    hypergraph.setPartitionIndex(0,0);
    hypergraph.setPartitionIndex(1,1);
    hypergraph.setPartitionIndex(2,1);
    hypergraph.setPartitionIndex(3,0);
    hypergraph.setPartitionIndex(4,0);
    hypergraph.setPartitionIndex(5,1);
    hypergraph.setPartitionIndex(6,1);
  }

  HypergraphType hypergraph;
  TwoWayFMRefiner<HypergraphType> refiner;
};

TEST_F(ATwoWayFMRefiner, IdentifiesBorderHypernodes) {
  ASSERT_THAT(refiner.isBorderNode(0), Eq(true));
  ASSERT_THAT(refiner.isBorderNode(1), Eq(true));
  ASSERT_THAT(refiner.isBorderNode(5), Eq(false));
}

TEST_F(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement) {
  ASSERT_THAT(refiner.gain(1), Eq(1));
  ASSERT_THAT(refiner.gain(5), Eq(-1));
}

TEST_F(ATwoWayFMRefiner, ComputesPartitionSizesOfHE) {
  refiner.calculatePartitionSizes(0);
  ASSERT_THAT(refiner.partitionPinCount(0, 0), Eq(1));
  ASSERT_THAT(refiner.partitionPinCount(0 ,1), Eq(1));

  refiner.calculatePartitionSizes(3);
  ASSERT_THAT(refiner.partitionPinCount(3, 0), Eq(0));
  ASSERT_THAT(refiner.partitionPinCount(3, 1), Eq(3));
}

TEST_F(ATwoWayFMRefiner, ChecksIfPartitionSizesOfHEAreAlreadyCalculated) {
  ASSERT_THAT(refiner.partitionSizesCalculated(0), Eq(false));

  refiner.calculatePartitionSizes(0);

  ASSERT_THAT(refiner.partitionSizesCalculated(0), Eq(true));
}
} // namespace partition
