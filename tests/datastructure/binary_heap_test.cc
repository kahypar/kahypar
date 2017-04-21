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

#include "kahypar/datastructure/binary_heap.h"
#include "kahypar/definitions.h"

using ::testing::Test;

namespace kahypar {
namespace ds {
using MaxHeapType = BinaryMaxHeap<HypernodeID, HyperedgeWeight>;
using MinHeapType = BinaryMinHeap<HypernodeID, HyperedgeWeight>;

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

typedef ::testing::Types<MaxHeapType, MinHeapType> Implementations;

TYPED_TEST_CASE(AHeap, Implementations);

TYPED_TEST(AHeap, IsEmptyWhenCreated) {
  ASSERT_EQ(this->_heap.empty(), true);
}

TYPED_TEST(AHeap, IsNotEmptyAfterInsert) {
  this->_heap.push(0, 1);
  ASSERT_EQ(this->_heap.empty(), false);
}

TYPED_TEST(AHeap, HasSizeOneAfterOneInsertion) {
  this->_heap.push(0, 1);
  ASSERT_EQ(this->_heap.size(), 1);
}

// Max Heap tests

TEST_F(AMaxHeap, ReturnsTheMaximumElement) {
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_EQ(this->_heap.top(), 0);
}

TEST_F(AMaxHeap, ReturnsTheMaximumKey) {
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_EQ(this->_heap.topKey(), 4);
}

TEST_F(AMaxHeap, BehavesAsExpectedOnUpdate) {
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_EQ(this->_heap.top(), 0);
  this->_heap.updateKey(0, -1);
  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.topKey(), 1);
}

TEST_F(AMaxHeap, BehavesAsExpectedOnRemoval) {
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_EQ(this->_heap.top(), 0);
  this->_heap.remove(0);
  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.topKey(), 1);
}

TEST_F(AMaxHeap, CanIncreaseKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.increaseKeyBy(0, 4);
  ASSERT_EQ(this->_heap.topKey(), 8);
}

TEST_F(AMaxHeap, CanDecreaseKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.decreaseKeyBy(0, 2);
  ASSERT_EQ(this->_heap.topKey(), 2);
}

TEST_F(AMaxHeap, CanUpdateKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.updateKeyBy(0, 2);
  this->_heap.updateKeyBy(0, -3);
  ASSERT_EQ(this->_heap.topKey(), 3);
}

TEST_F(AMaxHeap, AllowsDecreaseKeyOperations) {
  this->_heap.push(1, 9);
  this->_heap.decreaseKey(1, 2);
  ASSERT_EQ(this->_heap.topKey(), 2);
}

TEST_F(AMaxHeap, AllowsIncreaseKeyOperations) {
  this->_heap.push(1, 4);
  this->_heap.push(2, 5);
  this->_heap.increaseKey(1, 8);
  ASSERT_EQ(this->_heap.topKey(), 8);
}

TEST_F(AMaxHeap, BehavesAsExpectedOnDeletionOfCurrentMaximum) {
  this->_heap.push(2, -1);
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_EQ(this->_heap.top(), 0);
  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.topKey(), 1);
}

TEST_F(AMaxHeap, AlwaysReturnsMax) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  this->_heap.push(2, 10);
  this->_heap.push(3, 7);
  this->_heap.push(1, 5);

  ASSERT_EQ(this->_heap.top(), 2);
  ASSERT_EQ(this->_heap.topKey(), 10);

  this->_heap.pop();

  ASSERT_EQ(this->_heap.top(), 3);
  ASSERT_EQ(this->_heap.topKey(), 7);

  this->_heap.pop();

  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.topKey(), 5);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 0);
  ASSERT_EQ(this->_heap.topKey(), 4);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 4);
  ASSERT_EQ(this->_heap.topKey(), 3);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.empty(), true);
}

TEST_F(AMaxHeap, HandleDuplicateKeys) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  this->_heap.push(2, 3);
  this->_heap.push(3, 3);
  this->_heap.push(1, 4);

  ASSERT_EQ(this->_heap.top(), 0);
  ASSERT_EQ(this->_heap.topKey(), 4);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.topKey(), 4);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 3);
  ASSERT_EQ(this->_heap.topKey(), 3);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 2);
  ASSERT_EQ(this->_heap.topKey(), 3);
}

TEST_F(AMaxHeap, IsEmptyAfterClearing) {
  this->_heap.push(0, 10);
  this->_heap.push(2, 3);
  this->_heap.push(1, 10);
  this->_heap.clear();

  ASSERT_EQ(this->_heap.contains(0), false);
  ASSERT_EQ(this->_heap.contains(1), false);
  ASSERT_EQ(this->_heap.contains(2), false);
  ASSERT_EQ(this->_heap.empty(), true);
}


TEST_F(AMaxHeap, HandlesManySameKeys) {
  this->_heap.clear();
  this->_heap.push(7, 0);
  this->_heap.push(5, 0);
  this->_heap.push(3, 0);
  this->_heap.push(1, 0);
  this->_heap.push(19, 0);

  ASSERT_EQ(this->_heap.getKey(7), 0);
}

TEST_F(AMaxHeap, IsSwappable) {
  std::vector<MaxHeapType> _pqs;

  _pqs.emplace_back(360, 200);
  _pqs.emplace_back(360, 200);

  _pqs[0].push(257, 0);
  _pqs[0].push(310, 0);
  _pqs[0].push(197, 0);
  _pqs[0].push(243, 0);

  ASSERT_EQ(_pqs[0].getKey(257), 0);

  using std::swap;
  swap(_pqs[0], _pqs[1]);

  ASSERT_EQ(_pqs[1].getKey(257), 0);
}


// Min Heap tests
TEST_F(AMinHeap, ReturnsTheMinimumElement) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 1);
  ASSERT_EQ(this->_heap.top(), 4);
}

TEST_F(AMinHeap, ReturnsTheMinimumKey) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 1);
  ASSERT_EQ(this->_heap.topKey(), 1);
}

TEST_F(AMinHeap, BehavesAsExpectedOnUpdate) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  ASSERT_EQ(this->_heap.top(), 4);
  this->_heap.updateKey(0, -2);
  ASSERT_EQ(this->_heap.top(), 0);
  ASSERT_EQ(this->_heap.topKey(), -2);
}

TEST_F(AMinHeap, BehavesAsExpectedOnRemoval) {
  this->_heap.push(0, 1);
  this->_heap.push(1, 4);
  ASSERT_EQ(this->_heap.top(), 0);
  this->_heap.remove(0);
  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.topKey(), 4);
}

TEST_F(AMinHeap, CanIncreaseKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.increaseKeyBy(0, 4);
  ASSERT_EQ(this->_heap.topKey(), 8);
}

TEST_F(AMinHeap, CanDecreaseKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.decreaseKeyBy(0, 2);
  ASSERT_EQ(this->_heap.topKey(), 2);
}

TEST_F(AMinHeap, CanUpdateKeysByDelta) {
  this->_heap.push(0, 4);
  this->_heap.updateKeyBy(0, 2);
  this->_heap.updateKeyBy(0, -3);
  ASSERT_EQ(this->_heap.topKey(), 3);
}

TEST_F(AMinHeap, AllowsDecreaseKeyOperations) {
  this->_heap.push(1, 9);
  this->_heap.decreaseKey(1, 2);
  ASSERT_EQ(this->_heap.topKey(), 2);
}

TEST_F(AMinHeap, AllowsIncreaseKeyOperations) {
  this->_heap.push(1, 4);
  this->_heap.push(2, 5);
  this->_heap.increaseKey(1, 8);
  ASSERT_EQ(this->_heap.topKey(), 5);
}

TEST_F(AMinHeap, BehavesAsExpectedOnDeletionOfCurrentMinimum) {
  this->_heap.push(2, -1);
  this->_heap.push(0, 4);
  this->_heap.push(1, 1);
  ASSERT_EQ(this->_heap.top(), 2);
  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.top(), 1);
}

TEST_F(AMinHeap, AlwaysReturnsMin) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  this->_heap.push(2, 10);
  this->_heap.push(3, 7);
  this->_heap.push(1, 5);

  ASSERT_EQ(this->_heap.top(), 4);
  ASSERT_EQ(this->_heap.topKey(), 3);

  this->_heap.pop();

  ASSERT_EQ(this->_heap.top(), 0);
  ASSERT_EQ(this->_heap.topKey(), 4);

  this->_heap.pop();

  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.topKey(), 5);

  this->_heap.pop();

  ASSERT_EQ(this->_heap.top(), 3);
  ASSERT_EQ(this->_heap.topKey(), 7);

  this->_heap.pop();

  ASSERT_EQ(this->_heap.top(), 2);
  ASSERT_EQ(this->_heap.topKey(), 10);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.empty(), true);
}

TEST_F(AMinHeap, HandleDuplicateKeys) {
  this->_heap.push(0, 4);
  this->_heap.push(4, 3);
  this->_heap.push(2, 3);
  this->_heap.push(3, 3);
  this->_heap.push(1, 4);

  ASSERT_EQ(this->_heap.top(), 4);
  ASSERT_EQ(this->_heap.topKey(), 3);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 2);
  ASSERT_EQ(this->_heap.topKey(), 3);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 3);
  ASSERT_EQ(this->_heap.topKey(), 3);

  this->_heap.pop();
  ASSERT_EQ(this->_heap.top(), 1);
  ASSERT_EQ(this->_heap.topKey(), 4);
}

TEST_F(AMinHeap, IsEmptyAfterClearing) {
  this->_heap.push(0, 10);
  this->_heap.push(2, 3);
  this->_heap.push(1, 10);
  this->_heap.clear();

  ASSERT_EQ(this->_heap.contains(0), false);
  ASSERT_EQ(this->_heap.contains(1), false);
  ASSERT_EQ(this->_heap.contains(2), false);
  ASSERT_EQ(this->_heap.empty(), true);
}


TEST_F(AMinHeap, HandlesManySameKeys) {
  this->_heap.clear();
  this->_heap.push(7, 0);
  this->_heap.push(5, 0);
  this->_heap.push(3, 0);
  this->_heap.push(1, 0);
  this->_heap.push(19, 0);

  ASSERT_EQ(this->_heap.getKey(7), 0);
}

TEST_F(AMinHeap, IsSwappable) {
  std::vector<MinHeapType> _pqs;

  _pqs.emplace_back(360, 200);
  _pqs.emplace_back(360, 200);

  _pqs[0].push(257, 0);
  _pqs[0].push(310, 0);
  _pqs[0].push(197, 0);
  _pqs[0].push(243, 0);

  ASSERT_EQ(_pqs[0].getKey(257), 0);

  using std::swap;
  swap(_pqs[0], _pqs[1]);

  ASSERT_EQ(_pqs[1].getKey(257), 0);
}
}  // namespace ds
}  // namespace kahypar
