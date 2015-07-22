/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <gmock/gmock.h>

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "tools/CnfToHgrConversion.h"

using::testing::Test;
using::testing::Eq;


namespace cnfconversion {
TEST(ACnfToHgrConversionRoutine, ConvertsCNFinstancesIntoHypergraphInstances) {
  std::string cnf_filename("test_instances/SampleSAT.cnf");
  std::string hgr_filename("test_instances/SampleSAT.cnf.hgr");
  convertInstance(cnf_filename, hgr_filename);

  defs::Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_THAT(hypergraph.numNodes(), Eq(8));
  ASSERT_THAT(hypergraph.numPins(), Eq(9));
  ASSERT_THAT(hypergraph.numEdges(), Eq(3));

  std::vector<HypernodeID> pins_he_0({ 0, 1, 2 });
  size_t i = 0;
  for (const HypernodeID pin : hypergraph.pins(0)) {
    ASSERT_THAT(pin, Eq(pins_he_0[i++]));
  }

  i = 0;
  std::vector<HypernodeID> pins_he_1({ 3, 4, 5, 2 });
  for (const HypernodeID pin : hypergraph.pins(1)) {
    ASSERT_THAT(pin, Eq(pins_he_1[i++]));
  }

  i = 0;
  std::vector<HypernodeID> pins_he_2({ 6, 7 });
  for (const HypernodeID pin : hypergraph.pins(2)) {
    ASSERT_THAT(pin, Eq(pins_he_2[i++]));
  }
}
}  // namespace cnfconversion
