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

using::testing::Eq;
using::testing::DoubleEq;
using::testing::Test;

namespace kahypar {
namespace ds {
template <typename T>
class ASparseMap : public Test {
 public:
  ASparseMap() :
    sparse_map(20) { }

  T sparse_map;
};

template <typename T>
class ASparseMapSupportingDeletions : public Test {
 public:
  ASparseMapSupportingDeletions() :
    sparse_map(20) { }

  T sparse_map;
};

typedef::testing::Types<SparseMap<HypernodeID, double>,
                        InsertOnlySparseMap<HypernodeID, double> > Implementations;

typedef::testing::Types<SparseMap<HypernodeID, double> > ImplementationsWithDeletion;

TYPED_TEST_CASE(ASparseMap, Implementations);
TYPED_TEST_CASE(ASparseMapSupportingDeletions, ImplementationsWithDeletion);


TYPED_TEST(ASparseMap, ReturnsTrueIfElementIsInTheSet) {
  this->sparse_map.add(5, 5.5);
  this->sparse_map.add(6, 6.5);
  ASSERT_TRUE(this->sparse_map.contains(6));
  ASSERT_TRUE(this->sparse_map.contains(5));
}

TEST(AnInsertOnlySparseMap, HandlesThresholdOverflow) {
  InsertOnlySparseMap<HypernodeID, double> sparse_map(20);
  sparse_map.add(5, 5.5);
  ASSERT_TRUE(sparse_map.contains(5));

  sparse_map._threshold = std::numeric_limits<size_t>::max() - 1;
  sparse_map.clear();
  ASSERT_TRUE(sparse_map._threshold == 0);
  ASSERT_FALSE(sparse_map.contains(5));
}

TYPED_TEST(ASparseMap, ReturnsFalseIfElementIsNotInTheSet) {
  this->sparse_map.add(6, 6.6);
  ASSERT_FALSE(this->sparse_map.contains(5));
}

TYPED_TEST(ASparseMap, ReturnsFalseIfSetIsEmpty) {
  ASSERT_FALSE(this->sparse_map.contains(5));
}

TYPED_TEST(ASparseMapSupportingDeletions, ReturnsFalseAfterElementIsRemoved) {
  this->sparse_map.add(6, 6.6);
  this->sparse_map.add(1, 1.1);
  this->sparse_map.add(3, 3.3);

  this->sparse_map.remove(6);

  ASSERT_FALSE(this->sparse_map.contains(6));
  ASSERT_TRUE(this->sparse_map.contains(1));
  ASSERT_TRUE(this->sparse_map.contains(3));
}


TYPED_TEST(ASparseMapSupportingDeletions, HasCorrectSizeAfterElementIsRemoved) {
  this->sparse_map.add(6, 6.6);
  this->sparse_map.add(1, 1.1);
  this->sparse_map.add(3, 3.3);

  this->sparse_map.remove(1);

  ASSERT_THAT(this->sparse_map.size(), Eq(2));
}

TYPED_TEST(ASparseMap, AllowsIterationOverSetElements) {
  std::vector<HypernodeID> v { 6, 1, 3 };

  this->sparse_map.add(6, 6.6);
  this->sparse_map.add(1, 1.1);
  this->sparse_map.add(3, 3.3);

  size_t i = 0;
  for (const auto& element : this->sparse_map) {
    ASSERT_THAT(element.key, Eq(v[i++]));
  }
}

TYPED_TEST(ASparseMap, DoesNotContainElementsAfterItIsCleared) {
  this->sparse_map.add(6, 6.6);
  this->sparse_map.add(1, 1.1);
  this->sparse_map.add(3, 3.3);

  this->sparse_map.clear();

  ASSERT_THAT(this->sparse_map.size(), Eq(0));
  ASSERT_FALSE(this->sparse_map.contains(6));
  ASSERT_FALSE(this->sparse_map.contains(1));
  ASSERT_FALSE(this->sparse_map.contains(3));
}
}  // namespace ds
}  // namespace kahypar
