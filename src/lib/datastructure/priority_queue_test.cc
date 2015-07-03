/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/EnhancedBucketQueue.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/definitions.h"

using::testing::Eq;
using::testing::DoubleEq;
using::testing::Test;

namespace datastructure {
using MaxHeapQueue = NoDataBinaryMaxHeap<defs::HypernodeID, defs::HyperedgeWeight>;
using BucketQueue = EnhancedBucketQueue<defs::HypernodeID, defs::HyperedgeWeight>;

template <typename T>
class APriorityQueue : public Test {
 public:
  using PrioQueueType = T;
  APriorityQueue() :
    prio_queue(20, 120) { }

  T prio_queue;
};

typedef::testing::Types<BucketQueue, MaxHeapQueue> Implementations;

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
  ASSERT_THAT(this->prio_queue.getMax(), Eq(0));
}

TYPED_TEST(APriorityQueue, ReturnsTheMaximumKey) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(4));
}

TYPED_TEST(APriorityQueue, BehavesAsExpectedOnUpdate) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.getMax(), Eq(0));
  this->prio_queue.updateKey(0, -1);
  ASSERT_THAT(this->prio_queue.getMax(), Eq(1));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(1));
}

TYPED_TEST(APriorityQueue, BehavesAsExpectedOnRemoval) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.getMax(), Eq(0));
  this->prio_queue.deleteNode(0);
  ASSERT_THAT(this->prio_queue.getMax(), Eq(1));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(1));
}

TYPED_TEST(APriorityQueue, BehavesAsExpectedOnDeletionOfCurrentMaximum) {
  this->prio_queue.push(2, -1);
  this->prio_queue.push(0, 4);
  this->prio_queue.push(1, 1);
  ASSERT_THAT(this->prio_queue.getMax(), Eq(0));
  this->prio_queue.deleteMax();
  ASSERT_THAT(this->prio_queue.getMax(), Eq(1));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(1));
}

TYPED_TEST(APriorityQueue, AlwaysReturnsMax) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(4, 3);
  this->prio_queue.push(2, 10);
  this->prio_queue.push(3, 7);
  this->prio_queue.push(1, 5);

  ASSERT_THAT(this->prio_queue.getMax(), Eq(2));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(10));

  this->prio_queue.deleteMax();

  ASSERT_THAT(this->prio_queue.getMax(), Eq(3));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(7));

  this->prio_queue.deleteMax();

  ASSERT_THAT(this->prio_queue.getMax(), Eq(1));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(5));

  this->prio_queue.deleteMax();
  ASSERT_THAT(this->prio_queue.getMax(), Eq(0));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(4));

  this->prio_queue.deleteMax();
  ASSERT_THAT(this->prio_queue.getMax(), Eq(4));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(3));

  this->prio_queue.deleteMax();
  ASSERT_THAT(this->prio_queue.empty(), Eq(true));
}

TYPED_TEST(APriorityQueue, HandleDuplicateKeys) {
  this->prio_queue.push(0, 4);
  this->prio_queue.push(4, 3);
  this->prio_queue.push(2, 3);
  this->prio_queue.push(3, 3);
  this->prio_queue.push(1, 4);

  // bucket queue should use LIFO and therefore return 1
  ASSERT_TRUE(this->prio_queue.getMax() == 0 || this->prio_queue.getMax() == 1);
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(4));

  this->prio_queue.deleteMax();
  // bucket queue should use LIFO and therefore return 0
  ASSERT_TRUE(this->prio_queue.getMax() == 1 || this->prio_queue.getMax() == 0);
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(4));

  this->prio_queue.deleteMax();
  ASSERT_THAT(this->prio_queue.getMax(), Eq(3));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(3));

  this->prio_queue.deleteMax();
  ASSERT_THAT(this->prio_queue.getMax(), Eq(2));
  ASSERT_THAT(this->prio_queue.getMaxKey(), Eq(3));
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

TEST(ABucketQueue, ChangesPositionOfElementsOnZeroGainUpdate) {
  BucketQueue bucket_pq(10, 100);

  bucket_pq.push(0, 10);
  bucket_pq.push(2, 3);
  bucket_pq.push(1, 10);

  ASSERT_THAT(bucket_pq.getMax(), Eq(1));
  ASSERT_THAT(bucket_pq.getMaxKey(), Eq(10));
  bucket_pq.updateKey(0, 10);
  ASSERT_THAT(bucket_pq.getMax(), Eq(0));
  ASSERT_THAT(bucket_pq.getMaxKey(), Eq(10));
}



}  // namespace datastructure
