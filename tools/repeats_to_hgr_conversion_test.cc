/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "tools/repeats_to_hgr_conversion.h"

using ::testing::Test;

namespace kahypar {
namespace phylo {
TEST(Phylo, Works) {
  std::ifstream file("test_instances/repeats.txt");
  const std::string hgr_filename("test_instances/repeats.phylo.hgr");
  convertToHypergraph(file, hgr_filename);

  Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  hypergraph.printGraphState();

  ASSERT_EQ(hypergraph.initialNumNodes(), 5);
  ASSERT_EQ(hypergraph.initialNumPins(), 27);
  ASSERT_EQ(hypergraph.initialNumEdges(), 12);

  std::vector<HypernodeID> pins({ 1, 4 });
  size_t i = 0;
  for (const HypernodeID& pin : hypergraph.pins(0)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 1, 4 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(1)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 0, 1, 4 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(2)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 2, 3 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(3)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 0, 3 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(4)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 1, 4 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(5)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 0, 2 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(6)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 1, 4 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(7)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 0, 1, 4 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(8)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 2, 3 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(9)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 0, 1, 4 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(10)) {
    ASSERT_EQ(pin, pins[i++]);
  }

  pins = { 2, 3 };
  i = 0;
  for (const HypernodeID& pin : hypergraph.pins(11)) {
    ASSERT_EQ(pin, pins[i++]);
  }
}
}  // namespace phylo
}  // namespace kahypar
