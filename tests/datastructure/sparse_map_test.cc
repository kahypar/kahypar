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

#include "gmock/gmock.h"

#include "kahypar/datastructure/sparse_map.h"
#include "kahypar/definitions.h"

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Test;

namespace kahypar {
namespace ds {
class ASparseMap : public Test {
 public:
  ASparseMap() :
    sparse_map(20) { }

  SparseMap<HypernodeID, double> sparse_map;
};

TEST_F(ASparseMap, ReturnsTrueIfElementIsInTheSet) {
  sparse_map.add(5, 5.5);
  sparse_map.add(6, 6.5);
  ASSERT_TRUE(sparse_map.contains(6));
  ASSERT_TRUE(sparse_map.contains(5));
}

TEST_F(ASparseMap, ReturnsFalseIfElementIsNotInTheSet) {
  sparse_map.add(6, 6.6);
  ASSERT_FALSE(sparse_map.contains(5));
}

TEST_F(ASparseMap, ReturnsFalseIfSetIsEmpty) {
  ASSERT_FALSE(sparse_map.contains(5));
}

TEST_F(ASparseMap, ReturnsFalseAfterElementIsRemoved) {
  sparse_map.add(6, 6.6);
  sparse_map.add(1, 1.1);
  sparse_map.add(3, 3.3);

  sparse_map.remove(6);

  ASSERT_FALSE(sparse_map.contains(6));
  ASSERT_TRUE(sparse_map.contains(1));
  ASSERT_TRUE(sparse_map.contains(3));
}


TEST_F(ASparseMap, HasCorrectSizeAfterElementIsRemoved) {
  sparse_map.add(6, 6.6);
  sparse_map.add(1, 1.1);
  sparse_map.add(3, 3.3);

  sparse_map.remove(1);

  ASSERT_THAT(sparse_map.size(), Eq(2));
}

TEST_F(ASparseMap, AllowsIterationOverSetElements) {
  std::vector<HypernodeID> v { 6, 1, 3 };

  sparse_map.add(6, 6.6);
  sparse_map.add(1, 1.1);
  sparse_map.add(3, 3.3);

  size_t i = 0;
  for (const auto& element : sparse_map) {
    ASSERT_THAT(element.key, Eq(v[i++]));
  }
}

TEST_F(ASparseMap, DoesNotContainElementsAfterItIsCleared) {
  sparse_map.add(6, 6.6);
  sparse_map.add(1, 1.1);
  sparse_map.add(3, 3.3);

  sparse_map.clear();

  ASSERT_THAT(sparse_map.size(), Eq(0));
  ASSERT_FALSE(sparse_map.contains(6));
  ASSERT_FALSE(sparse_map.contains(1));
  ASSERT_FALSE(sparse_map.contains(3));
}
}  // namespace ds
}  // namespace kahypar
