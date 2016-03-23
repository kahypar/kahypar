/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/ConnectivitySet.h"
#include "lib/datastructure/InsertOnlyConnectivitySet.h"
#include "lib/datastructure/SparseSet.h"
#include "lib/definitions.h"

using::testing::Eq;
using::testing::DoubleEq;
using::testing::Test;

namespace datastructure {
template <typename T>
class ASparseSet : public Test {
 public:
  ASparseSet() :
    sparse_set(20) { }

  T sparse_set;
};

template <typename T>
class ASparseSetSupportingDeletions : public Test {
 public:
  ASparseSetSupportingDeletions() :
    sparse_set(20) { }

  T sparse_set;
};

typedef::testing::Types<SparseSet<defs::HypernodeID>, ConnectivitySet<defs::PartitionID>,
                        InsertOnlyConnectivitySet<defs::PartitionID> > Implementations;

typedef::testing::Types<SparseSet<defs::HypernodeID>, ConnectivitySet<defs::PartitionID> >
  ImplementationsWithDeletion;

TYPED_TEST_CASE(ASparseSet, Implementations);

TYPED_TEST_CASE(ASparseSetSupportingDeletions, ImplementationsWithDeletion);

TYPED_TEST(ASparseSet, ReturnsTrueIfElementIsInTheSet) {
  this->sparse_set.add(5);
  ASSERT_TRUE(this->sparse_set.contains(5));
}

TYPED_TEST(ASparseSet, ReturnsFalseIfElementIsNotInTheSet) {
  this->sparse_set.add(6);
  ASSERT_FALSE(this->sparse_set.contains(5));
}

TYPED_TEST(ASparseSet, ReturnsFalseIfSetIsEmpty) {
  ASSERT_FALSE(this->sparse_set.contains(5));
}

TYPED_TEST(ASparseSet, AllowsIterationOverSetElements) {
  std::vector<defs::HypernodeID> v { 6, 1, 3 };

  this->sparse_set.add(6);
  this->sparse_set.add(1);
  this->sparse_set.add(3);

  size_t i = 0;
  for (const auto element : this->sparse_set) {
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

  for (const auto element : this->sparse_set) {
    ASSERT_NE(element, 1);
    ASSERT_NE(element, 7);
  }
}
}  // namespace datastructure
