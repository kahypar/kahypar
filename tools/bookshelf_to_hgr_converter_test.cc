/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "tools/bookshelf_to_hgr_converter.h"

using ::testing::Test;

using namespace kahypar;

TEST(TheBookshelfToHgrConverter, ConvertsBookshelfInstancesIntoHypergraphInstances) {
  const std::string bookshelf_filename("test_instances/bookshelf_format.nets");
  const std::string hgr_filename("test_instances/bookshelf_format.nets.hgr");
  convertBookshelfToHgr(bookshelf_filename, hgr_filename);

  Hypergraph hypergraph = io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(hypergraph.initialNumNodes(), 11);
  ASSERT_EQ(hypergraph.initialNumPins(), 13);
  ASSERT_EQ(hypergraph.currentNumEdges(), 3);

  std::vector<HypernodeID> pins_he_0({ 0, 1, 2, 3 });
  size_t i = 0;
  for (const HypernodeID& pin : hypergraph.pins(0)) {
    ASSERT_EQ(pin, pins_he_0[i++]);
  }

  i = 0;
  std::vector<HypernodeID> pins_he_1({ 1, 4, 5, 6, 7, 8, 3 });
  for (const HypernodeID& pin : hypergraph.pins(1)) {
    ASSERT_EQ(pin, pins_he_1[i++]);
  }

  i = 0;
  std::vector<HypernodeID> pins_he_2({ 9, 10 });
  for (const HypernodeID& pin : hypergraph.pins(2)) {
    ASSERT_EQ(pin, pins_he_2[i++]);
  }
}
