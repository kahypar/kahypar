/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <iostream>

#include "gmock/gmock.h"

#include "lib/definitions.h"

#include "external/binary_heap/BinaryHeap.hpp"
#include "lib/datastructure/BucketQueue.h"
#include "lib/datastructure/PriorityQueue.h"

using::testing::Test;
using::testing::Eq;

using external::BinaryHeap;
using defs::HypernodeID;
using defs::HyperedgeWeight;
using datastructure::BucketPQ;
using datastructure::PriorityQueue;

typedef PriorityQueue<BinaryHeap<HypernodeID, HyperedgeWeight,
                                 std::numeric_limits<HyperedgeWeight> > > HeapBasedPQ;
typedef BucketPQ<HypernodeID, HyperedgeWeight, HyperedgeWeight> BucketBasedPQ;

TEST(BothQueues, AlwaysReturnMax) {
  HypernodeID num_nodes = 5;
  HyperedgeWeight max_gain = 10;

  HeapBasedPQ heap_pq(num_nodes);
  BucketBasedPQ bucket_pq(max_gain);

  heap_pq.reInsert(0, 4);
  heap_pq.reInsert(4, 3);
  heap_pq.reInsert(2, 10);
  heap_pq.reInsert(3, 7);
  heap_pq.reInsert(1, 5);

  bucket_pq.reInsert(0, 4);
  bucket_pq.reInsert(4, 3);
  bucket_pq.reInsert(2, 10);
  bucket_pq.reInsert(3, 7);
  bucket_pq.reInsert(1, 5);

  ASSERT_THAT(heap_pq.max(), Eq(2));
  ASSERT_THAT(heap_pq.maxKey(), Eq(10));
  ASSERT_THAT(bucket_pq.max(), Eq(2));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(10));

  heap_pq.deleteMax();
  bucket_pq.deleteMax();
  ASSERT_THAT(heap_pq.max(), Eq(3));
  ASSERT_THAT(heap_pq.maxKey(), Eq(7));
  ASSERT_THAT(bucket_pq.max(), Eq(3));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(7));
  heap_pq.deleteMax();
  bucket_pq.deleteMax();
  ASSERT_THAT(heap_pq.max(), Eq(1));
  ASSERT_THAT(heap_pq.maxKey(), Eq(5));
  ASSERT_THAT(bucket_pq.max(), Eq(1));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(5));
  heap_pq.deleteMax();
  bucket_pq.deleteMax();
  ASSERT_THAT(heap_pq.max(), Eq(0));
  ASSERT_THAT(heap_pq.maxKey(), Eq(4));
  ASSERT_THAT(bucket_pq.max(), Eq(0));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(4));
  heap_pq.deleteMax();
  bucket_pq.deleteMax();
  ASSERT_THAT(heap_pq.max(), Eq(4));
  ASSERT_THAT(heap_pq.maxKey(), Eq(3));
  ASSERT_THAT(bucket_pq.max(), Eq(4));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(3));
  heap_pq.deleteMax();
  bucket_pq.deleteMax();
  ASSERT_THAT(heap_pq.empty(), Eq(true));
  ASSERT_THAT(bucket_pq.empty(), Eq(true));
}

TEST(BothQueues, HandleDuplicateKeys) {
  HypernodeID num_nodes = 5;
  HyperedgeWeight max_gain = 10;
  HeapBasedPQ heap_pq(num_nodes);
  BucketBasedPQ bucket_pq(max_gain);

  heap_pq.reInsert(0, 4);
  heap_pq.reInsert(4, 3);
  heap_pq.reInsert(2, 3);
  heap_pq.reInsert(3, 3);
  heap_pq.reInsert(1, 4);
  bucket_pq.reInsert(0, 4);
  bucket_pq.reInsert(4, 3);
  bucket_pq.reInsert(2, 3);
  bucket_pq.reInsert(3, 3);
  bucket_pq.reInsert(1, 4);

  ASSERT_THAT(heap_pq.max(), Eq(0));
  ASSERT_THAT(heap_pq.maxKey(), Eq(4));
  // bucket queue should use LIFO
  ASSERT_THAT(bucket_pq.max(), Eq(1));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(4));

  heap_pq.deleteMax();
  bucket_pq.deleteMax();
  ASSERT_THAT(heap_pq.max(), Eq(1));
  ASSERT_THAT(heap_pq.maxKey(), Eq(4));
  // bucket queue should use LIFO
  ASSERT_THAT(bucket_pq.max(), Eq(0));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(4));

  heap_pq.deleteMax();
  bucket_pq.deleteMax();
  ASSERT_THAT(heap_pq.max(), Eq(3));
  ASSERT_THAT(heap_pq.maxKey(), Eq(3));
  ASSERT_THAT(bucket_pq.max(), Eq(3));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(3));

  heap_pq.deleteMax();
  bucket_pq.deleteMax();
  ASSERT_THAT(heap_pq.max(), Eq(2));
  ASSERT_THAT(heap_pq.maxKey(), Eq(3));
  ASSERT_THAT(bucket_pq.max(), Eq(2));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(3));
}

TEST(BucketPQ, ChangesPositionOfElementsOnZeroGainUpdate) {
  HyperedgeWeight max_gain = 10;
  BucketBasedPQ bucket_pq(max_gain);

  bucket_pq.reInsert(0, 10);
  bucket_pq.reInsert(2, 3);
  bucket_pq.reInsert(1, 10);

  ASSERT_THAT(bucket_pq.max(), Eq(1));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(10));

  bucket_pq.updateKey(0, 10);
  ASSERT_THAT(bucket_pq.max(), Eq(0));
  ASSERT_THAT(bucket_pq.maxKey(), Eq(10));
}

TEST(BucketPQ, IsEmptyAfterClearing) {
  HyperedgeWeight max_gain = 10;
  BucketBasedPQ bucket_pq(max_gain);

  bucket_pq.reInsert(0, 10);
  bucket_pq.reInsert(2, 3);
  bucket_pq.reInsert(1, 10);
  bucket_pq.clear();
  ASSERT_THAT(bucket_pq.contains(0), Eq(false));
  ASSERT_THAT(bucket_pq.contains(1), Eq(false));
  ASSERT_THAT(bucket_pq.contains(2), Eq(false));
  ASSERT_THAT(bucket_pq.empty(), Eq(true));
}
