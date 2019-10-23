/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "tools/hgr_to_mtx_conversion.h"

#include <vector>

using ::testing::Test;

namespace kahypar {
TEST(AHypergraph, CanBeConvertedToMatrixMarketFormatForMondriaanPartitioning) {
  const std::string input_hypergraph_filename("test_instances/mondriaan_example.hgr");
  Hypergraph input_hypergraph = kahypar::io::createHypergraphFromFile(input_hypergraph_filename, 2);

  const std::string mtx_filename("test_instances/mondriaan_example.mtx");

  writeHypergraphInMatrixMarketFormat(input_hypergraph, mtx_filename);

  std::ifstream mtx_file(mtx_filename);

  std::vector<int> expected = { 4, 5, 8, 2, 1, 1, 2, 1, 2, 2, 3, 2, 3, 3, 3, 4, 4, 3, 4, 4, 1, 1, 1, 1, 1 };
  std::reverse(std::begin(expected), std::end(expected));

  std::string line;
  std::getline(mtx_file, line);
  // skip any comments
  while (line[0] == '%') {
    std::getline(mtx_file, line);
  }

  for (int i = 0; i < 14; ++i) {
    std::istringstream line_stream(line);
    int input = -1;
    while (line_stream >> input) {
      if (expected.back() == input) {
        expected.pop_back();
      }
    }
    std::getline(mtx_file, line);
  }
  for (const auto x :  expected) {
    LOG << x;
  }
  ASSERT_TRUE(expected.empty());
}
}  // namespace kahypar
