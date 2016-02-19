/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/SparseSet.h"
#include "lib/definitions.h"

using::testing::Eq;
using::testing::DoubleEq;
using::testing::Test;

namespace datastructure {
class ASparseSet : public Test {
 public:
  ASparseSet() :
    sparse_set(20) { }

  SparseSet<defs::HypernodeID> sparse_set;
};


TEST_F(ASparseSet, ReturnsTrueIfElementIsInTheSet) {
  sparse_set.add(5);
  ASSERT_TRUE(sparse_set.contains(5));
}

TEST_F(ASparseSet, ReturnsFalseIfElementIsNotInTheSet) {
  sparse_set.add(6);
  ASSERT_FALSE(sparse_set.contains(5));
}

TEST_F(ASparseSet, ReturnsFalseIfSetIsEmpty) {
  ASSERT_FALSE(sparse_set.contains(5));
}

TEST_F(ASparseSet, ReturnsFalseAfterElementIsRemoved) {
  sparse_set.add(6);
  sparse_set.add(1);
  sparse_set.add(3);

  sparse_set.remove(6);

  ASSERT_FALSE(sparse_set.contains(6));
  ASSERT_TRUE(sparse_set.contains(1));
  ASSERT_TRUE(sparse_set.contains(3));
}


TEST_F(ASparseSet, HasCorrectSizeAfterElementIsRemoved) {
  sparse_set.add(6);
  sparse_set.add(1);
  sparse_set.add(3);

  sparse_set.remove(1);

  ASSERT_THAT(sparse_set.size(), Eq(2));
}

TEST_F(ASparseSet, AllowsIterationOverSetElements) {
  std::vector<defs::HypernodeID> v { 6, 1, 3 };

  sparse_set.add(6);
  sparse_set.add(1);
  sparse_set.add(3);

  size_t i = 0;
  for (const auto element : sparse_set) {
    ASSERT_THAT(element, Eq(v[i++]));
  }
}

TEST_F(ASparseSet, DoesNotContainElementsAfterItIsCleared) {
  sparse_set.add(6);
  sparse_set.add(1);
  sparse_set.add(3);

  sparse_set.clear();

  ASSERT_THAT(sparse_set.size(), Eq(0));
  ASSERT_FALSE(sparse_set.contains(6));
  ASSERT_FALSE(sparse_set.contains(1));
  ASSERT_FALSE(sparse_set.contains(3));
}
}  // namespace datastructure
