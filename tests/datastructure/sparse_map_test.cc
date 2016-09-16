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
  ASSERT_TRUE(this->sparse_map.contains(5));
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
}  // namespace datastructure
