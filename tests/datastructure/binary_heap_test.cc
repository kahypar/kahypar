/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "datastructure/BinaryHeap.h"
#include "definitions.h"

using::testing::Eq;
using::testing::Test;

namespace datastructure {
using MaxHeapType = BinaryMaxHeap<defs::HypernodeID, defs::HyperedgeWeight>;
using MinHeapType = BinaryMinHeap<defs::HypernodeID, defs::HyperedgeWeight>;

template <typename T>
class AHeap : public Test {
 public:
  using HeapType = T;
  AHeap() :
    _heap(20) { }

  HeapType _heap;
};

class AMaxHeap : public Test {
 public:
  AMaxHeap() :
    _heap(20) { }

  MaxHeapType _heap;
};

class AMinHeap : public Test {
 public:
  AMinHeap() :
    _heap(20) { }

  MinHeapType _heap;
};

typedef::testing::Types<MaxHeapType, MinHeapType> Implementations;

TYPED_TEST_CASE(AHeap, Implementations);

TYPED_TEST(AHeap, IsEmptyWhenCreated) {
  ASSERT_THAT(this->_heap.empty(), Eq(true));
}

TYPED_TEST(AHeap, IsNotEmptyAfterInsert) {
  this->_heap.push(0, 1);
  ASSERT_THAT(this->_heap.empty(), Eq(false));
}

TYPED_TEST(AHeap, HasSizeOneAfterOneInsertion) {
  this->_heap.push(0, 1);
  ASSERT_THAT(this->_heap.size(), Eq(1));
}

// Max Heap tests

TEST_F(AMaxHeap, ReturnsTheMaximumElement) {
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_THAT(this->_heap.top(), Eq(0));
}

TEST_F(AMaxHeap, ReturnsTheMaximumKey) {
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_THAT(this->_heap.topKey(), Eq(4));
}

TEST_F(AMaxHeap, BehavesAsExpectedOnUpdate) {
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_THAT(this->_heap.top(), Eq(0));
  this->_heap.updateKey(0, -1);
  ASSERT_THAT(this->_heap.top(), Eq(1));
  ASSERT_THAT(this->_heap.topKey(), Eq(1));
}

TEST_F(AMaxHeap, BehavesAsExpectedOnRemoval) {
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_THAT(this->_heap.top(), Eq(0));
  this->_heap.remove(0);
  ASSERT_THAT(this->_heap.top(), Eq(1));
  ASSERT_THAT(this->_heap.topKey(), Eq(1));
}

TEST_F(AMaxHeap, CanIncreaseKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.increaseKeyBy(0, 4);
  ASSERT_THAT(this->_heap.topKey(), Eq(8));
}

TEST_F(AMaxHeap, CanDecreaseKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.decreaseKeyBy(0, 2);
  ASSERT_THAT(this->_heap.topKey(), Eq(2));
}

TEST_F(AMaxHeap, CanUpdateKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.updateKeyBy(0, 2);
  this->_heap.updateKeyBy(0, -3);
  ASSERT_THAT(this->_heap.topKey(), Eq(3));
}

TEST_F(AMaxHeap, AllowsDecreaseKeyOperations) {
  this->_heap.push(1, 9);
  this->_heap.decreaseKey(1, 2);
  ASSERT_THAT(this->_heap.topKey(), Eq(2));
}

TEST_F(AMaxHeap, AllowsIncreaseKeyOperations) {
  this->_heap.push(1, 4);
  this->_heap.push(2, 5);
  this->_heap.increaseKey(1, 8);
  ASSERT_THAT(this->_heap.topKey(), Eq(8));
}

TEST_F(AMaxHeap, BehavesAsExpectedOnDeletionOfCurrentMaximum) {
  this->_heap.push(2, -1);
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_THAT(this->_heap.top(), Eq(0));
  this->_heap.pop();
  ASSERT_THAT(this->_heap.top(), Eq(1));
  ASSERT_THAT(this->_heap.topKey(), Eq(1));
}

TEST_F(AMaxHeap, AlwaysReturnsMax) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  this->_heap.push(2, 10);
  this->_heap.push(3, 7);
  this->_heap.push(1, 5);

  ASSERT_THAT(this->_heap.top(), Eq(2));
  ASSERT_THAT(this->_heap.topKey(), Eq(10));

  this->_heap.pop();

  ASSERT_THAT(this->_heap.top(), Eq(3));
  ASSERT_THAT(this->_heap.topKey(), Eq(7));

  this->_heap.pop();

  ASSERT_THAT(this->_heap.top(), Eq(1));
  ASSERT_THAT(this->_heap.topKey(), Eq(5));

  this->_heap.pop();
  ASSERT_THAT(this->_heap.top(), Eq(0));
  ASSERT_THAT(this->_heap.topKey(), Eq(4));

  this->_heap.pop();
  ASSERT_THAT(this->_heap.top(), Eq(4));
  ASSERT_THAT(this->_heap.topKey(), Eq(3));

  this->_heap.pop();
  ASSERT_THAT(this->_heap.empty(), Eq(true));
}

TEST_F(AMaxHeap, HandleDuplicateKeys) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  this->_heap.push(2, 3);
  this->_heap.push(3, 3);
  this->_heap.push(1, 4);

  ASSERT_TRUE(this->_heap.top() == 0);
  ASSERT_THAT(this->_heap.topKey(), Eq(4));

  this->_heap.pop();
  ASSERT_TRUE(this->_heap.top() == 1);
  ASSERT_THAT(this->_heap.topKey(), Eq(4));

  this->_heap.pop();
  ASSERT_TRUE(this->_heap.top() == 3);
  ASSERT_THAT(this->_heap.topKey(), Eq(3));

  this->_heap.pop();
  ASSERT_TRUE(this->_heap.top() == 2);
  ASSERT_THAT(this->_heap.topKey(), Eq(3));
}

TEST_F(AMaxHeap, IsEmptyAfterClearing) {
  this->_heap.push(0, 10);
  this->_heap.push(2, 3);
  this->_heap.push(1, 10);
  this->_heap.clear();

  ASSERT_THAT(this->_heap.contains(0), Eq(false));
  ASSERT_THAT(this->_heap.contains(1), Eq(false));
  ASSERT_THAT(this->_heap.contains(2), Eq(false));
  ASSERT_THAT(this->_heap.empty(), Eq(true));
}


TEST_F(AMaxHeap, HandlesManySameKeys) {
  this->_heap.clear();
  this->_heap.push(7, 0);
  this->_heap.push(5, 0);
  this->_heap.push(3, 0);
  this->_heap.push(1, 0);
  this->_heap.push(19, 0);

  ASSERT_THAT(this->_heap.getKey(7), Eq(0));
}

TEST_F(AMaxHeap, IsSwappable) {
  std::vector<MaxHeapType> _pqs;

  _pqs.emplace_back(360, 200);
  _pqs.emplace_back(360, 200);

  _pqs[0].push(257, 0);
  _pqs[0].push(310, 0);
  _pqs[0].push(197, 0);
  _pqs[0].push(243, 0);

  ASSERT_THAT(_pqs[0].getKey(257), Eq(0));

  using std::swap;
  swap(_pqs[0], _pqs[1]);

  ASSERT_THAT(_pqs[1].getKey(257), Eq(0));
}


// Min Heap tests
TEST_F(AMinHeap, ReturnsTheMinimumElement) {
  this->_heap.push(0, 4);
  this->_heap.push(42, 1);
  ASSERT_THAT(this->_heap.top(), Eq(42));
}

TEST_F(AMinHeap, ReturnsTheMinimumKey) {
  this->_heap.push(0, 4);
  this->_heap.push(42, 1);
  ASSERT_THAT(this->_heap.topKey(), Eq(1));
}

TEST_F(AMinHeap, BehavesAsExpectedOnUpdate) {
  this->_heap.push(0, 4);
  this->_heap.push(42, 3);
  ASSERT_THAT(this->_heap.top(), Eq(42));
  this->_heap.updateKey(0, -2);
  ASSERT_THAT(this->_heap.top(), Eq(0));
  ASSERT_THAT(this->_heap.topKey(), Eq(-2));
}

TEST_F(AMinHeap, BehavesAsExpectedOnRemoval) {
  this->_heap.push(0, 1);
  this->_heap.push(1, 4);
  ASSERT_THAT(this->_heap.top(), Eq(0));
  this->_heap.remove(0);
  ASSERT_THAT(this->_heap.top(), Eq(1));
  ASSERT_THAT(this->_heap.topKey(), Eq(4));
}

TEST_F(AMinHeap, CanIncreaseKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.increaseKeyBy(0, 4);
  ASSERT_THAT(this->_heap.topKey(), Eq(8));
}

TEST_F(AMinHeap, CanDecreaseKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.decreaseKeyBy(0, 2);
  ASSERT_THAT(this->_heap.topKey(), Eq(2));
}

TEST_F(AMinHeap, CanUpdateKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.updateKeyBy(0, 2);
  this->_heap.updateKeyBy(0, -3);
  ASSERT_THAT(this->_heap.topKey(), Eq(3));
}

TEST_F(AMinHeap, AllowsDecreaseKeyOperations) {
  this->_heap.push(1, 9);
  this->_heap.decreaseKey(1, 2);
  ASSERT_THAT(this->_heap.topKey(), Eq(2));
}

TEST_F(AMinHeap, AllowsIncreaseKeyOperations) {
  this->_heap.push(1, 4);
  this->_heap.push(2, 5);
  this->_heap.increaseKey(1, 8);
  ASSERT_THAT(this->_heap.topKey(), Eq(5));
}

TEST_F(AMinHeap, BehavesAsExpectedOnDeletionOfCurrentMinimum) {
  this->_heap.push(2, -1);
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_THAT(this->_heap.top(), Eq(2));
  this->_heap.pop();
  ASSERT_THAT(this->_heap.top(), Eq(1));
  ASSERT_THAT(this->_heap.top(), Eq(1));
}

TEST_F(AMinHeap, AlwaysReturnsMin) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  this->_heap.push(2, 10);
  this->_heap.push(3, 7);
  this->_heap.push(1, 5);

  ASSERT_THAT(this->_heap.top(), Eq(4));
  ASSERT_THAT(this->_heap.topKey(), Eq(3));

  this->_heap.pop();

  ASSERT_THAT(this->_heap.top(), Eq(0));
  ASSERT_THAT(this->_heap.topKey(), Eq(4));

  this->_heap.pop();

  ASSERT_THAT(this->_heap.top(), Eq(1));
  ASSERT_THAT(this->_heap.topKey(), Eq(5));

  this->_heap.pop();

  ASSERT_THAT(this->_heap.top(), Eq(3));
  ASSERT_THAT(this->_heap.topKey(), Eq(7));

  this->_heap.pop();

  ASSERT_THAT(this->_heap.top(), Eq(2));
  ASSERT_THAT(this->_heap.topKey(), Eq(10));

  this->_heap.pop();
  ASSERT_THAT(this->_heap.empty(), Eq(true));
}

TEST_F(AMinHeap, HandleDuplicateKeys) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  this->_heap.push(2, 3);
  this->_heap.push(3, 3);
  this->_heap.push(1, 4);

  ASSERT_TRUE(this->_heap.top() == 4);
  ASSERT_THAT(this->_heap.topKey(), Eq(3));

  this->_heap.pop();
  ASSERT_TRUE(this->_heap.top() == 2);
  ASSERT_THAT(this->_heap.topKey(), Eq(3));

  this->_heap.pop();
  ASSERT_TRUE(this->_heap.top() == 3);
  ASSERT_THAT(this->_heap.topKey(), Eq(3));

  this->_heap.pop();
  ASSERT_TRUE(this->_heap.top() == 1);
  ASSERT_THAT(this->_heap.topKey(), Eq(4));
}

TEST_F(AMinHeap, IsEmptyAfterClearing) {
  this->_heap.push(0, 10);
  this->_heap.push(2, 3);
  this->_heap.push(1, 10);
  this->_heap.clear();

  ASSERT_THAT(this->_heap.contains(0), Eq(false));
  ASSERT_THAT(this->_heap.contains(1), Eq(false));
  ASSERT_THAT(this->_heap.contains(2), Eq(false));
  ASSERT_THAT(this->_heap.empty(), Eq(true));
}


TEST_F(AMinHeap, HandlesManySameKeys) {
  this->_heap.clear();
  this->_heap.push(7, 0);
  this->_heap.push(5, 0);
  this->_heap.push(3, 0);
  this->_heap.push(1, 0);
  this->_heap.push(19, 0);

  ASSERT_THAT(this->_heap.getKey(7), Eq(0));
}

TEST_F(AMinHeap, IsSwappable) {
  std::vector<MinHeapType> _pqs;

  _pqs.emplace_back(360, 200);
  _pqs.emplace_back(360, 200);

  _pqs[0].push(257, 0);
  _pqs[0].push(310, 0);
  _pqs[0].push(197, 0);
  _pqs[0].push(243, 0);

  ASSERT_THAT(_pqs[0].getKey(257), Eq(0));

  using std::swap;
  swap(_pqs[0], _pqs[1]);

  ASSERT_THAT(_pqs[1].getKey(257), Eq(0));
}
}  // namespace datastructure
