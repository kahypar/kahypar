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

#include "kahypar/datastructure/sparse_set.h"
#include "kahypar/definitions.h"

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Test;

namespace kahypar {
namespace ds {
template <typename T>
class ASparseSet : public Test {
 public:
  ASparseSet() :
    sparse_set(20) { }

  T sparse_set;

  using TestType = T;
};

template <typename T>
class ASparseSetSupportingDeletions : public Test {
 public:
  ASparseSetSupportingDeletions() :
    sparse_set(20) { }

  T sparse_set;
};

typedef ::testing::Types<SparseSet<HypernodeID>,
                         InsertOnlySparseSet<PartitionID> > Implementations;

typedef ::testing::Types<SparseSet<HypernodeID> > ImplementationsWithDeletion;

TYPED_TEST_CASE(ASparseSet, Implementations);

TYPED_TEST_CASE(ASparseSetSupportingDeletions, ImplementationsWithDeletion);

TYPED_TEST(ASparseSet, ReturnsTrueIfElementIsInTheSet) {
  this->sparse_set.add(5);
  ASSERT_TRUE(this->sparse_set.contains(5));
}

TEST(AnInsertOnlySparseSet, HandlesThresholdOverflow) {
  InsertOnlySparseSet<PartitionID> sparse_set(20);
  sparse_set.add(5);
  ASSERT_TRUE(sparse_set.contains(5));

  sparse_set._threshold = std::numeric_limits<PartitionID>::max() - 1;
  sparse_set.clear();
  ASSERT_EQ(sparse_set._threshold, 0);
  ASSERT_FALSE(sparse_set.contains(5));
}

TYPED_TEST(ASparseSet, ReturnsFalseIfElementIsNotInTheSet) {
  this->sparse_set.add(6);
  ASSERT_FALSE(this->sparse_set.contains(5));
}

TYPED_TEST(ASparseSet, AllowsMoveAssigment) {
  this->sparse_set.add(6);

  typename TestFixture::TestType another_set(20);
  another_set = std::move(this->sparse_set);

  ASSERT_TRUE(another_set.contains(6));
}


TYPED_TEST(ASparseSet, ReturnsFalseIfSetIsEmpty) {
  ASSERT_FALSE(this->sparse_set.contains(5));
}

TYPED_TEST(ASparseSet, AllowsIterationOverSetElements) {
  std::vector<HypernodeID> v { 6, 1, 3 };

  this->sparse_set.add(6);
  this->sparse_set.add(1);
  this->sparse_set.add(3);

  size_t i = 0;
  for (const auto& element : this->sparse_set) {
    ASSERT_THAT(element, Eq(v[i++]));
  }
}

TYPED_TEST(ASparseSet, DoesNotContainElementsAfterItIsCleared) {
  this->sparse_set.add(6);
  this->sparse_set.add(1);
  this->sparse_set.add(3);

  this->sparse_set.clear();

  ASSERT_THAT(this->sparse_set.size(), Eq(0));
  ASSERT_FALSE(this->sparse_set.contains(6));
  ASSERT_FALSE(this->sparse_set.contains(1));
  ASSERT_FALSE(this->sparse_set.contains(3));
}


TYPED_TEST(ASparseSetSupportingDeletions, ReturnsFalseAfterElementIsRemoved) {
  this->sparse_set.add(6);
  this->sparse_set.add(1);
  this->sparse_set.add(3);

  this->sparse_set.remove(6);

  ASSERT_FALSE(this->sparse_set.contains(6));
  ASSERT_TRUE(this->sparse_set.contains(1));
  ASSERT_TRUE(this->sparse_set.contains(3));
}


TYPED_TEST(ASparseSetSupportingDeletions, HasCorrectSizeAfterElementIsRemoved) {
  this->sparse_set.add(6);
  this->sparse_set.add(1);
  this->sparse_set.add(3);

  this->sparse_set.remove(1);

  ASSERT_THAT(this->sparse_set.size(), Eq(2));
}

TYPED_TEST(ASparseSetSupportingDeletions, DoesNotIterateOverDeletedElements) {
  this->sparse_set.add(6);
  this->sparse_set.add(1);
  this->sparse_set.add(4);
  this->sparse_set.add(5);
  this->sparse_set.add(7);
  this->sparse_set.add(0);

  this->sparse_set.remove(1);
  this->sparse_set.remove(7);

  for (const auto& element : this->sparse_set) {
    ASSERT_NE(element, 1);
    ASSERT_NE(element, 7);
  }
}
}  // namespace ds
}  // namespace kahypar
