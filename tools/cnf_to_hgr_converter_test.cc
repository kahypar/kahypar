/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include "gtest/gtest.h"

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "tools/cnf_to_hgr_conversion.h"

using ::testing::Test;

using namespace kahypar;

namespace cnfconversion {
TEST(ACnfToHgrConversionRoutine, ConvertsCNFinstancesIntoLiteralHypergraphRepresentation) {
  std::string cnf_filename("test_instances/SampleSAT.cnf");
  std::string hgr_filename("test_instances/SampleSAT.cnf.hgr");
  convertInstance(cnf_filename, hgr_filename, HypergraphRepresentation::Literal);

  Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(hypergraph.initialNumNodes(), 8);
  ASSERT_EQ(hypergraph.initialNumPins(), 8);
  ASSERT_EQ(hypergraph.initialNumEdges(), 3);

  std::vector<HypernodeID> pins_he_0({ 0, 1, 2 });
  size_t i = 0;
  for (const HypernodeID& pin : hypergraph.pins(0)) {
    ASSERT_EQ(pin, pins_he_0[i++]);
  }

  i = 0;
  std::vector<HypernodeID> pins_he_1({ 3, 4 });
  for (const HypernodeID& pin : hypergraph.pins(1)) {
    ASSERT_EQ(pin, pins_he_1[i++]);
  }

  i = 0;
  std::vector<HypernodeID> pins_he_2({ 5, 6, 7 });
  for (const HypernodeID& pin : hypergraph.pins(2)) {
    ASSERT_EQ(pin, pins_he_2[i++]);
  }
}

TEST(ACnfToHgrConversionRoutine, ConvertsCNFinstancesIntoPrimalHypergraphRepresentation) {
  std::string cnf_filename("test_instances/SampleSAT.cnf");
  std::string hgr_filename("test_instances/SampleSAT.cnf.hgr");
  convertInstance(cnf_filename, hgr_filename, HypergraphRepresentation::Primal);

  Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(hypergraph.initialNumNodes(), 5);
  ASSERT_EQ(hypergraph.initialNumPins(), 8);
  ASSERT_EQ(hypergraph.initialNumEdges(), 3);

  std::vector<HypernodeID> pins_he_0({ 0, 1, 2 });
  size_t i = 0;
  for (const HypernodeID& pin : hypergraph.pins(0)) {
    ASSERT_EQ(pin, pins_he_0[i++]);
  }

  i = 0;
  std::vector<HypernodeID> pins_he_1({ 3, 4 });
  for (const HypernodeID& pin : hypergraph.pins(1)) {
    ASSERT_EQ(pin, pins_he_1[i++]);
  }

  i = 0;
  std::vector<HypernodeID> pins_he_2({ 0, 1, 4 });
  for (const HypernodeID& pin : hypergraph.pins(2)) {
    ASSERT_EQ(pin, pins_he_2[i++]);
  }
}

TEST(ACnfToHgrConversionRoutine, ConvertsCNFinstancesIntoDualHypergraphRepresentation) {
  std::string cnf_filename("test_instances/SampleSAT.cnf");
  std::string hgr_filename("test_instances/SampleSAT.cnf.hgr");
  convertInstance(cnf_filename, hgr_filename, HypergraphRepresentation::Dual);

  Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(hypergraph.initialNumNodes(), 3);
  ASSERT_EQ(hypergraph.initialNumPins(), 8);
  ASSERT_EQ(hypergraph.initialNumEdges(), 5);

  const std::vector<HypernodeID> pins_he_0({ 0, 2 });
  size_t i = 0;
  for (const HypernodeID& pin : hypergraph.pins(0)) {
    ASSERT_EQ(pin, pins_he_0[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_1({ 0, 2 });
  for (const HypernodeID& pin : hypergraph.pins(1)) {
    ASSERT_EQ(pin, pins_he_1[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_2({ 0 });
  for (const HypernodeID& pin : hypergraph.pins(2)) {
    ASSERT_EQ(pin, pins_he_2[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_3({ 1 });
  for (const HypernodeID& pin : hypergraph.pins(3)) {
    ASSERT_EQ(pin, pins_he_3[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_4({ 1, 2 });
  for (const HypernodeID& pin : hypergraph.pins(4)) {
    ASSERT_EQ(pin, pins_he_4[i++]);
  }
}

TEST(ACnfToHgrConversionRoutine, IgnoresMissingVariablesInPrimalRepresentation) {
  std::string cnf_filename("test_instances/SampleSAT_missing_variables.cnf");
  std::string hgr_filename("test_instances/SampleSAT_missing_variables.cnf.hgr");
  convertInstance(cnf_filename, hgr_filename, HypergraphRepresentation::Primal);

  Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(hypergraph.initialNumNodes(), 8);
  ASSERT_EQ(hypergraph.initialNumPins(), 10);
  ASSERT_EQ(hypergraph.initialNumEdges(), 3);

  const std::vector<HypernodeID> pins_he_0({ 0, 1, 2 });
  size_t i = 0;
  for (const HypernodeID& pin : hypergraph.pins(0)) {
    ASSERT_EQ(pin, pins_he_0[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_1({ 3, 4, 5 });
  for (const HypernodeID& pin : hypergraph.pins(1)) {
    ASSERT_EQ(pin, pins_he_1[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_2({ 6, 1, 0, 7 });
  for (const HypernodeID& pin : hypergraph.pins(2)) {
    ASSERT_EQ(pin, pins_he_2[i++]);
  }
}


TEST(ACnfToHgrConversionRoutine, IgnoresMissingVariablesInDualRepresentation) {
  std::string cnf_filename("test_instances/SampleSAT_missing_variables.cnf");
  std::string hgr_filename("test_instances/SampleSAT_missing_variables.cnf.hgr");
  convertInstance(cnf_filename, hgr_filename, HypergraphRepresentation::Dual);

  Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(hypergraph.initialNumNodes(), 3);
  ASSERT_EQ(hypergraph.initialNumPins(), 10);
  ASSERT_EQ(hypergraph.initialNumEdges(), 8);

  const std::vector<HypernodeID> pins_he_0({ 0, 2 });
  size_t i = 0;
  for (const HypernodeID& pin : hypergraph.pins(0)) {
    ASSERT_EQ(pin, pins_he_0[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_1({ 0, 2 });
  for (const HypernodeID& pin : hypergraph.pins(1)) {
    ASSERT_EQ(pin, pins_he_1[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_2({ 0 });
  for (const HypernodeID& pin : hypergraph.pins(2)) {
    ASSERT_EQ(pin, pins_he_2[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_3({ 1 });
  for (const HypernodeID& pin : hypergraph.pins(3)) {
    ASSERT_EQ(pin, pins_he_3[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_4({ 1 });
  for (const HypernodeID& pin : hypergraph.pins(4)) {
    ASSERT_EQ(pin, pins_he_4[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_5({ 1 });
  for (const HypernodeID& pin : hypergraph.pins(5)) {
    ASSERT_EQ(pin, pins_he_5[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_6({ 2 });
  for (const HypernodeID& pin : hypergraph.pins(6)) {
    ASSERT_EQ(pin, pins_he_6[i++]);
  }

  i = 0;
  const std::vector<HypernodeID> pins_he_7({ 2 });
  for (const HypernodeID& pin : hypergraph.pins(7)) {
    ASSERT_EQ(pin, pins_he_7[i++]);
  }
}
}  // namespace cnfconversion
