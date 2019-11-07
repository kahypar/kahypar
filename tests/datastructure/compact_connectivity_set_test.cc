/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <atomic>

#include "gmock/gmock.h"

#include "kahypar/datastructure/compact_connectivity_set.h"

using ::testing::Test;

namespace kahypar {
namespace ds {
using PartitionID = int32_t;

void add(CompactConnectivitySet& conn_set, const std::set<PartitionID>& ids) {
  for (const PartitionID& id : ids) {
    conn_set.add(id);
  }
}

void remove(CompactConnectivitySet& conn_set, const std::set<PartitionID>& ids) {
  for (const PartitionID& id : ids) {
    conn_set.remove(id);
  }
}

void verify(const CompactConnectivitySet& conn_set,
            const PartitionID k,
            const std::set<PartitionID>& contained) {
  // Verify bitset in connectivity set
  ASSERT_EQ(contained.size(), conn_set.size());
  for (PartitionID i = 0; i < k; ++i) {
    if (contained.find(i) != contained.end()) {
      ASSERT_TRUE(conn_set.contains(i)) << V(i);
    } else {
      ASSERT_FALSE(conn_set.contains(i)) << V(i);
    }
  }

  // Verify iterator
  size_t connectivity = 0;
  for (const PartitionID id : conn_set) {
    ASSERT_TRUE(contained.find(id) != contained.end()) << V(id);
    ++connectivity;
  }
  ASSERT_EQ(contained.size(), connectivity);
}

TEST(ACompactConnectivitySet, IsCorrectInitialized) {
  CompactConnectivitySet conn_set(32);
  verify(conn_set, 32, { });
}

TEST(ACompactConnectivitySet, AddOnePartition1) {
  CompactConnectivitySet conn_set(32);
  conn_set.add(2);
  verify(conn_set, 32, { 2 });
}

TEST(ACompactConnectivitySet, AddOnePartition2) {
  CompactConnectivitySet conn_set(32);
  conn_set.add(14);
  verify(conn_set, 32, { 14 });
}

TEST(ACompactConnectivitySet, AddOnePartition3) {
  CompactConnectivitySet conn_set(32);
  conn_set.add(23);
  verify(conn_set, 32, { 23 });
}

TEST(ACompactConnectivitySet, AddOnePartition4) {
  CompactConnectivitySet conn_set(32);
  conn_set.add(30);
  verify(conn_set, 32, { 30 });
}

TEST(ACompactConnectivitySet, AddTwoPartitions1) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 5, 31 };
  add(conn_set, added);
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddTwoPartitions2) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 14, 24 };
  add(conn_set, added);
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddTwoPartitions3) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 7, 16 };
  add(conn_set, added);
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddSeveralPartitions1) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 0, 1, 5, 14, 24, 27, 31 };
  add(conn_set, added);
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddSeveralPartitions2) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 5, 6, 7, 11, 13, 15, 24, 28, 30 };
  add(conn_set, added);
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddSeveralPartitions3) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };
  add(conn_set, added);
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddTwoPartitionsAndRemoveOne1) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 5, 31 };
  std::set<PartitionID> removed = { 31 };
  add(conn_set, added);
  remove(conn_set, removed);
  for (const PartitionID id : removed) {
    added.erase(id);
  }
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddTwoPartitionsAndRemoveOne2) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 16, 17 };
  std::set<PartitionID> removed = { 16 };
  add(conn_set, added);
  remove(conn_set, removed);
  for (const PartitionID id : removed) {
    added.erase(id);
  }
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddTwoPartitionsAndRemoveOne3) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 7, 21 };
  std::set<PartitionID> removed = { 7 };
  add(conn_set, added);
  remove(conn_set, removed);
  for (const PartitionID id : removed) {
    added.erase(id);
  }
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddTwoPartitionsAndRemoveOne4) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 25, 27 };
  std::set<PartitionID> removed = { 27 };
  add(conn_set, added);
  remove(conn_set, removed);
  for (const PartitionID id : removed) {
    added.erase(id);
  }
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddSeveralPartitionsAndRemoveSeveral1) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 1, 13, 15, 23, 24, 30 };
  std::set<PartitionID> removed = { 13, 15, 23 };
  add(conn_set, added);
  remove(conn_set, removed);
  for (const PartitionID id : removed) {
    added.erase(id);
  }
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddSeveralPartitionsAndRemoveSeveral2) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 2, 5, 6, 14, 15, 21, 23, 29 };
  std::set<PartitionID> removed = { 5, 14, 21, 29 };
  add(conn_set, added);
  remove(conn_set, removed);
  for (const PartitionID id : removed) {
    added.erase(id);
  }
  verify(conn_set, 32, added);
}

TEST(ACompactConnectivitySet, AddSeveralPartitionsAndRemoveSeveral3) {
  CompactConnectivitySet conn_set(32);
  std::set<PartitionID> added = { 0, 1, 2, 3, 4, 5, 6, 7, 24, 25, 26, 27, 28, 29 };
  std::set<PartitionID> removed = { 5, 6, 7, 24, 25, 26, 27 };
  add(conn_set, added);
  remove(conn_set, removed);
  for (const PartitionID id : removed) {
    added.erase(id);
  }
  verify(conn_set, 32, added);
}
}  // namespace ds
}  // namespace kahypar
