/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"

using::testing::Eq;
using::testing::Test;

using defs::HypernodeID;
using defs::PartitionID;
using defs::HyperedgeWeight;

namespace datastructure {
class AKWayPriorityQueue : public Test {
  public:
  AKWayPriorityQueue() :
    prio_queue(5, 4) { }

  KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                    std::numeric_limits<HyperedgeWeight> > prio_queue;
};

typedef AKWayPriorityQueue AKWayPriorityQueueDeathTest;

TEST_F(AKWayPriorityQueue, IsEmptyIfItContainsNoElements) {
  ASSERT_THAT(prio_queue.empty(), Eq(true));
}

TEST_F(AKWayPriorityQueue, DISABLED_IsEmptyIfNoInternalHeapsAreEnabled) {
  prio_queue.insert(1, 2, 25);
  ASSERT_THAT(prio_queue.empty(), Eq(true));
}

TEST_F(AKWayPriorityQueue, IsNotEmptyIfThereExistsANonemptyAndEnabledInternalHeap) {
  prio_queue.insert(1, 2, 25);
  prio_queue.enablePart(2);
  ASSERT_THAT(prio_queue.empty(), Eq(false));
}

TEST_F(AKWayPriorityQueue, MustEnableAnInternalHeapInOrderToConsiderItDuringDeleteMax) {
  prio_queue.insert(1, 2, 25);
  prio_queue.enablePart(2);
  HypernodeID max_id = -1;
  HyperedgeWeight max_gain = -1;
  PartitionID max_part = -1;

  prio_queue.deleteMax(max_id, max_gain, max_part);

  ASSERT_THAT(max_id, Eq(1));
  ASSERT_THAT(max_gain, Eq(25));
  ASSERT_THAT(max_part, Eq(2));
}

TEST_F(AKWayPriorityQueue, IsEmptyIfLastEnabledInternalHeapBecomesEmpty) {
  prio_queue.insert(1, 2, 25);
  prio_queue.enablePart(2);
  HypernodeID max_id = -1;
  HyperedgeWeight max_gain = -1;
  PartitionID max_part = -1;

  prio_queue.deleteMax(max_id, max_gain, max_part);

  ASSERT_THAT(prio_queue.empty(), Eq(true));
}

TEST_F(AKWayPriorityQueue, IsEmptyIfLastEnabledInternalHeapBecomesDisabled) {
  prio_queue.insert(1, 2, 25);
  prio_queue.enablePart(2);

  prio_queue.disablePart(2);

  ASSERT_THAT(prio_queue.empty(), Eq(true));
}

TEST_F(AKWayPriorityQueue, DISABLED_ChoosesMaxKeyAmongAllEnabledInternalHeaps) {
  prio_queue.insert(1, 2, 25);
  prio_queue.insert(4, 1, 6);
  prio_queue.insert(0, 3, 42);
  prio_queue.insert(3, 0, 23);

  prio_queue.enablePart(1);
  ASSERT_THAT(prio_queue.max(), Eq(4));
  ASSERT_THAT(prio_queue.maxKey(), Eq(6));

  prio_queue.enablePart(2);
  ASSERT_THAT(prio_queue.max(), Eq(1));
  ASSERT_THAT(prio_queue.maxKey(), Eq(25));

  prio_queue.enablePart(3);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  ASSERT_THAT(prio_queue.maxKey(), Eq(42));

  prio_queue.enablePart(0);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  ASSERT_THAT(prio_queue.maxKey(), Eq(42));
}

TEST_F(AKWayPriorityQueue, DoesNotConsiderDisabledHeapForChoosingMax) {
  prio_queue.insert(1, 2, 25);
  prio_queue.insert(4, 1, 6);
  prio_queue.insert(0, 3, 42);
  prio_queue.insert(3, 0, 23);
  prio_queue.enablePart(0);
  prio_queue.enablePart(1);
  prio_queue.enablePart(2);
  prio_queue.enablePart(3);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  ASSERT_THAT(prio_queue.maxKey(), Eq(42));

  prio_queue.disablePart(3);
  ASSERT_THAT(prio_queue.isEnabled(3), Eq(false));
  ASSERT_THAT(prio_queue.max(), Eq(1));
  ASSERT_THAT(prio_queue.maxKey(), Eq(25));
}

TEST_F(AKWayPriorityQueue, ReconsidersDisabledHeapAgainAfterEnabling) {
  prio_queue.insert(1, 2, 25);
  prio_queue.insert(4, 1, 6);
  prio_queue.insert(0, 3, 42);
  prio_queue.insert(3, 0, 23);
  prio_queue.enablePart(0);
  prio_queue.enablePart(1);
  prio_queue.enablePart(2);
  prio_queue.enablePart(3);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  ASSERT_THAT(prio_queue.maxKey(), Eq(42));
  prio_queue.disablePart(3);
  ASSERT_THAT(prio_queue.max(), Eq(1));
  ASSERT_THAT(prio_queue.maxKey(), Eq(25));

  prio_queue.enablePart(3);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  ASSERT_THAT(prio_queue.maxKey(), Eq(42));
}

TEST_F(AKWayPriorityQueue, DisablesInternalHeapIfItBecomesEmptyDueToRemoval) {
  prio_queue.insert(1, 2, 25);
  prio_queue.insert(0, 3, 42);
  prio_queue.enablePart(2);
  prio_queue.enablePart(3);
  ASSERT_THAT(prio_queue.max(), Eq(0));
  ASSERT_THAT(prio_queue.maxKey(), Eq(42));

  prio_queue.remove(0, 3);
  ASSERT_THAT(prio_queue.isEnabled(3), Eq(false));
}

TEST_F(AKWayPriorityQueue, ReturnsFalseIfUnusedHeapIsEmpty) {
  prio_queue.insert(1, 2, 25);
  ASSERT_THAT(prio_queue.empty(0), Eq(true));
}

TEST_F(AKWayPriorityQueue, ReturnsSizeZeroForUnusedInternalHeaps) {
  prio_queue.insert(1, 2, 25);
  prio_queue.insert(0, 0, 42);
  ASSERT_THAT(prio_queue.size(3), Eq(0));
}

TEST_F(AKWayPriorityQueue, ReturnsCorrectSizeForUsedInternalHeaps) {
  prio_queue.insert(1, 2, 25);
  prio_queue.insert(2, 2, 23);
  prio_queue.insert(0, 0, 42);
  ASSERT_THAT(prio_queue.size(2), Eq(2));
  ASSERT_THAT(prio_queue.size(0), Eq(1));
}

TEST_F(AKWayPriorityQueue, AnswersContainmentChecksForAllNonEmptyPQs) {
  prio_queue.insert(1, 2, 25);
  // disabled part
  ASSERT_THAT(prio_queue.contains(1, 2), Eq(true));

  prio_queue.insert(0, 0, 1);
  prio_queue.enablePart(0);
  // enabled part
  ASSERT_THAT(prio_queue.contains(0, 0), Eq(true));

  // unused part
  ASSERT_THAT(prio_queue.contains(1, 1), Eq(false));
}

TEST_F(AKWayPriorityQueue, ReturnsNumberOfEnabledParts) {
  ASSERT_THAT(prio_queue.numEnabledParts(), Eq(0));
  prio_queue.insert(1, 2, 25);
  prio_queue.enablePart(2);
  ASSERT_THAT(prio_queue.numEnabledParts(), Eq(1));
}

TEST_F(AKWayPriorityQueue, EnsuresThatEmptyPartsCannotBeEnabled) {
  prio_queue.enablePart(2);
  ASSERT_THAT(prio_queue.numEnabledParts(), Eq(0));
}

TEST_F(AKWayPriorityQueue, ReturnsNumberOfNonEmptyParts) {
  ASSERT_THAT(prio_queue.numNonEmptyParts(), Eq(0));
  prio_queue.insert(1, 2, 25);
  ASSERT_THAT(prio_queue.numNonEmptyParts(), Eq(1));
}

TEST_F(AKWayPriorityQueue,
       DoesNotDecreaseTheNumberOfEnabledHeapsIfTheLastElementOfADisabledHeapIsRemoved) {
  prio_queue.insert(1, 2, 25);
  prio_queue.insert(0, 3, 23);
  prio_queue.enablePart(2);

  prio_queue.remove(0, 3);

  ASSERT_THAT(prio_queue.numEnabledParts(), Eq(1));
}
} // namespace datastructure
