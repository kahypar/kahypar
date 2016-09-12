/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gtest/gtest.h"

#include "definitions.h"
#include "io/HypergraphIO.h"
#include "CnfToHgrConversion.h"

using::testing::Test;


namespace cnfconversion {
TEST(ACnfToHgrConversionRoutine, ConvertsCNFinstancesIntoHypergraphInstances) {
  std::string cnf_filename("test_instances/SampleSAT.cnf");
  std::string hgr_filename("test_instances/SampleSAT.cnf.hgr");
  convertInstance(cnf_filename, hgr_filename);

  defs::Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(hypergraph.initialNumNodes(), 8);
  ASSERT_EQ(hypergraph.initialNumPins(), 9);
  ASSERT_EQ(hypergraph.currentNumEdges(), 3);

  std::vector<HypernodeID> pins_he_0({ 0, 1, 2 });
  size_t i = 0;
  for (const HypernodeID pin : hypergraph.pins(0)) {
    ASSERT_EQ(pin, pins_he_0[i++]);
  }

  i = 0;
  std::vector<HypernodeID> pins_he_1({ 3, 4, 5, 2 });
  for (const HypernodeID pin : hypergraph.pins(1)) {
    ASSERT_EQ(pin, pins_he_1[i++]);
  }

  i = 0;
  std::vector<HypernodeID> pins_he_2({ 6, 7 });
  for (const HypernodeID pin : hypergraph.pins(2)) {
    ASSERT_EQ(pin, pins_he_2[i++]);
  }
}
}  // namespace cnfconversion
