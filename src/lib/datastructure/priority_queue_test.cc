#include "gmock/gmock.h"

#include "PriorityQueue.h"
#include "../definitions.h"

using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::Test;

class APriorityQueue : public Test {
 public:
  APriorityQueue() :
      prio_queue(3,3) {}
  
  PriorityQueue<defs::HyperNodeID, defs::RatingType> prio_queue;
};

TEST_F(APriorityQueue, IsEmptyWhenCreated) {
  ASSERT_THAT(prio_queue.empty(), Eq(true));
}

TEST_F(APriorityQueue, IsNotEmptyAfterInsert) {
  prio_queue.insert(0, 1.1);
  ASSERT_THAT(prio_queue.empty(), Eq(false));
}

TEST_F(APriorityQueue, HasSizeOneAfterOneInsertion) {
  prio_queue.insert(0, 1.1);
  ASSERT_THAT(prio_queue.size(), Eq(1));
}

TEST_F(APriorityQueue, ReturnsTheMaximumElement) {
  prio_queue.insert(0, 4.1);
  prio_queue.insert(1, 0.833333);
  ASSERT_THAT(prio_queue.max(), Eq(0));
}

TEST_F(APriorityQueue, ReturnsTheMaximumKey) {
  prio_queue.insert(0, 4.1);
  prio_queue.insert(1, 0.833333);
  ASSERT_THAT(prio_queue.maxKey(), DoubleEq(4.1));
}

TEST_F(APriorityQueue, BehavesAsExpectedOnUpdate) {
  prio_queue.insert(0, 4.1);
  prio_queue.insert(1, 0.833333);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  prio_queue.update(0, 0.5);
  ASSERT_THAT(prio_queue.max(), Eq(1));
  ASSERT_THAT(prio_queue.maxKey(), DoubleEq(0.833333));
}

TEST_F(APriorityQueue, BehavesAsExpectedOnRemoval) {
  prio_queue.insert(0, 4.1);
  prio_queue.insert(1, 0.833333);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  prio_queue.remove(0);
  ASSERT_THAT(prio_queue.max(), Eq(1));
  ASSERT_THAT(prio_queue.maxKey(), DoubleEq(0.833333));
}

TEST_F(APriorityQueue, BehavesAsExpectedOnDeletionOfCurrentMaximum) {
  prio_queue.insert(2, 0.5);
  prio_queue.insert(0, 4.1);
  prio_queue.insert(1, 0.833333);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  prio_queue.deleteMax();
  ASSERT_THAT(prio_queue.max(), Eq(1));
  ASSERT_THAT(prio_queue.maxKey(), DoubleEq(0.833333));
}
