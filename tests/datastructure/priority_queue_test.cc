/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "kahypar/datastructure/bucket_queue.h"
#include "kahypar/definitions.h"

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Test;

namespace kahypar {
namespace ds {
using MaxHeapQueue = BinaryMaxHeap<HypernodeID, HyperedgeWeight>;
using BucketQueue = EnhancedBucketQueue<HypernodeID, HyperedgeWeight>;

template <typename T>
class APriorityQueue : public Test {
 public:
  using PrioQueueType = T;
  APriorityQueue() :
    prio_queue(20, 120) { }

  T prio_queue;
};

typedef ::testing::Types<BucketQueue, MaxHeapQueue> Implementations;

TYPED_TEST_CASE(APriorityQueue, Implementations);

TYPED_TEST(APriorityQueue, IsEmptyWhenCreated) {
  ASSERT_THAT(this->prio_queue.empty(), Eq(true));
}

TYPED_TEST(APriorityQueue, IsNotEmptyAfterInsert) {
  this->prio_queue.push(0, 1);
  ASSERT_THAT(this->prio_queue.empty(), Eq(false));
}

TYPED_TEST(APriorityQueue, HasSizeOneAfterOneInsertion) {
  this->prio_queue.push(0, 1);
  ASSERT_THAT(this->prio_queue.size(), Eq(1));
}

TYPED_TEST(APriorityQueue, ReturnsTheMaximumElement) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.top(), Eq(0));
}

TYPED_TEST(APriorityQueue, ReturnsTheMaximumKey) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(4));
}

TYPED_TEST(APriorityQueue, BehavesAsExpectedOnUpdate) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.top(), Eq(0));
  this->prio_queue.updateKey(0, -1);
  ASSERT_THAT(this->prio_queue.top(), Eq(1));
  ASSERT_THAT(this->prio_queue.topKey(), Eq(1));
}

TYPED_TEST(APriorityQueue, BehavesAsExpectedOnRemoval) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.top(), Eq(0));
  this->prio_queue.remove(0);
  ASSERT_THAT(this->prio_queue.top(), Eq(1));
  ASSERT_THAT(this->prio_queue.topKey(), Eq(1));
}

TYPED_TEST(APriorityQueue, CanIncreaseKeysByDelta) {
  this->prio_queue.push(0, 4);
  this->prio_queue.increaseKeyBy(0, 4);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(8));
}

TYPED_TEST(APriorityQueue, CanDecreaseKeysByDelta) {
  this->prio_queue.push(0, 4);
  this->prio_queue.decreaseKeyBy(0, 2);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(2));
}

TYPED_TEST(APriorityQueue, CanUpdateKeysByDelta) {
  this->prio_queue.push(0, 4);
  this->prio_queue.updateKeyBy(0, 2);
  this->prio_queue.updateKeyBy(0, -3);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(3));
}

TYPED_TEST(APriorityQueue, AllowsDecreaseKeyOperations) {
  this->prio_queue.push(1, 9);
  this->prio_queue.decreaseKey(1, 2);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(2));
}

TYPED_TEST(APriorityQueue, AllowsIncreaseKeyOperations) {
  this->prio_queue.push(1, 4);
  this->prio_queue.push(2, 5);
  this->prio_queue.increaseKey(1, 8);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(8));
}

TYPED_TEST(APriorityQueue, BehavesAsExpectedOnDeletionOfCurrentMaximum) {
  this->prio_queue.push(2, -1);
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.top(), Eq(0));
  this->prio_queue.pop();
  ASSERT_THAT(this->prio_queue.top(), Eq(1));
  ASSERT_THAT(this->prio_queue.topKey(), Eq(1));
}

TYPED_TEST(APriorityQueue, AlwaysReturnsMax) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(4, 3);
  this->prio_queue.push(2, 10);
  this->prio_queue.push(3, 7);
  this->prio_queue.push(1, 5);

  ASSERT_THAT(this->prio_queue.top(), Eq(2));
  ASSERT_THAT(this->prio_queue.topKey(), Eq(10));

  this->prio_queue.pop();

  ASSERT_THAT(this->prio_queue.top(), Eq(3));
  ASSERT_THAT(this->prio_queue.topKey(), Eq(7));

  this->prio_queue.pop();

  ASSERT_THAT(this->prio_queue.top(), Eq(1));
  ASSERT_THAT(this->prio_queue.topKey(), Eq(5));

  this->prio_queue.pop();
  ASSERT_THAT(this->prio_queue.top(), Eq(0));
  ASSERT_THAT(this->prio_queue.topKey(), Eq(4));

  this->prio_queue.pop();
  ASSERT_THAT(this->prio_queue.top(), Eq(4));
  ASSERT_THAT(this->prio_queue.topKey(), Eq(3));

  this->prio_queue.pop();
  ASSERT_THAT(this->prio_queue.empty(), Eq(true));
}

TYPED_TEST(APriorityQueue, HandleDuplicateKeys) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(4, 3);
  this->prio_queue.push(2, 3);
  this->prio_queue.push(3, 3);
  this->prio_queue.push(1, 4);

  // bucket queue should use LIFO and therefore return 1
  ASSERT_TRUE(this->prio_queue.top() == 0 || this->prio_queue.top() == 1);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(4));

  this->prio_queue.pop();
  // bucket queue should use LIFO and therefore return 0
  ASSERT_TRUE(this->prio_queue.top() == 1 || this->prio_queue.top() == 0);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(4));

  this->prio_queue.pop();
  ASSERT_TRUE(this->prio_queue.top() == 3 || this->prio_queue.top() == 2);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(3));

  this->prio_queue.pop();
  ASSERT_TRUE(this->prio_queue.top() == 2 || this->prio_queue.top() == 3);
  ASSERT_THAT(this->prio_queue.topKey(), Eq(3));
}

TYPED_TEST(APriorityQueue, IsEmptyAfterClearing) {
  this->prio_queue.push(0, 10);
  this->prio_queue.push(2, 3);
  this->prio_queue.push(1, 10);
  this->prio_queue.clear();

  ASSERT_THAT(this->prio_queue.contains(0), Eq(false));
  ASSERT_THAT(this->prio_queue.contains(1), Eq(false));
  ASSERT_THAT(this->prio_queue.contains(2), Eq(false));
  ASSERT_THAT(this->prio_queue.empty(), Eq(true));
}


TYPED_TEST(APriorityQueue, HandlesManySameKeys) {
  this->prio_queue.clear();
  this->prio_queue.push(7, 0);
  this->prio_queue.push(5, 0);
  this->prio_queue.push(3, 0);
  this->prio_queue.push(1, 0);
  this->prio_queue.push(19, 0);

  ASSERT_THAT(this->prio_queue.getKey(7), Eq(0));
}

TEST(ABucketQueue, ChangesPositionOfElementsOnZeroGainUpdate) {
  BucketQueue bucket_pq(10, 100);

  bucket_pq.push(0, 10);
  bucket_pq.push(2, 3);
  bucket_pq.push(1, 10);

  ASSERT_THAT(bucket_pq.top(), Eq(1));
  ASSERT_THAT(bucket_pq.topKey(), Eq(10));
  bucket_pq.updateKey(0, 10);
  ASSERT_THAT(bucket_pq.top(), Eq(0));
  ASSERT_THAT(bucket_pq.topKey(), Eq(10));
}

TYPED_TEST(APriorityQueue, IsSwappable) {
  // special type TypeParam is used to get current
  // implementation type
  std::vector<TypeParam> _pqs;

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
}  // namespace ds
}  // namespace kahypar
