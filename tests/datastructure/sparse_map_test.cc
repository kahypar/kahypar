/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "datastructure/sparse_map.h"
#include "definitions.h"

using::testing::Eq;
using::testing::DoubleEq;
using::testing::Test;

namespace datastructure {
class ASparseMap : public Test {
 public:
  ASparseMap() :
    sparse_map(20) { }

  SparseMap<defs::HypernodeID, double> sparse_map;
};


TEST_F(ASparseMap, ReturnsTrueIfElementIsInTheSet) {
  sparse_map.add(5, 5.5);
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
  std::vector<defs::HypernodeID> v { 6, 1, 3 };

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
}  // namespace datastructure
