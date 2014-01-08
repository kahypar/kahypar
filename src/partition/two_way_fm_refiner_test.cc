#include "gmock/gmock.h"

#include "TwoWayFMRefiner.h"
#include "../lib/datastructure/Hypergraph.h"

namespace partition {
using ::testing::Test;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;

class ATwoWayFMRefiner : public Test {
 public:
  ATwoWayFMRefiner() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      refiner(hypergraph) {}

  HypergraphType hypergraph;
  TwoWayFMRefiner<HypergraphType> refiner;
};

TEST_F(ATwoWayFMRefiner, TakesAHypergraphAsInput) {
}

} // namespace partition
