/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@live.com>
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

#include <set>
#include <vector>

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/flow/quotient_graph_block_scheduler.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
class AQuotientGraphBlockScheduler : public Test {
 public:
  AQuotientGraphBlockScheduler() :
    context(),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }, 4),
    scheduler(nullptr) {
    hypergraph.setNodePart(0, 0);
    hypergraph.setNodePart(1, 1);
    hypergraph.setNodePart(2, 3);
    hypergraph.setNodePart(3, 2);
    hypergraph.setNodePart(4, 2);
    hypergraph.setNodePart(5, 3);
    hypergraph.setNodePart(6, 3);

    context.partition.k = 4;
    scheduler = new QuotientGraphBlockScheduler(hypergraph, context);
  }

  Context context;
  Hypergraph hypergraph;
  QuotientGraphBlockScheduler* scheduler;
};


TEST_F(AQuotientGraphBlockScheduler, HasCorrectQuotientGraphEdges) {
  scheduler->buildQuotientGraph();
  std::vector<std::pair<PartitionID, PartitionID> > adjacentBlocks = { std::make_pair(0, 1),
                                                                       std::make_pair(0, 2),
                                                                       std::make_pair(0, 3),
                                                                       std::make_pair(1, 2),
                                                                       std::make_pair(2, 3) };
  size_t idx = 0;
  for (const auto& e : scheduler->qoutientGraphEdges()) {
    ASSERT_EQ(adjacentBlocks[idx].first, e.first);
    ASSERT_EQ(adjacentBlocks[idx].second, e.second);
    idx++;
  }
}

TEST_F(AQuotientGraphBlockScheduler, HasCorrectCutHyperedges) {
  scheduler->buildQuotientGraph();

  for (const auto& e : scheduler->blockPairCutHyperedges(0, 1)) {
    ASSERT_EQ(e, 1);
  }
  for (const auto& e : scheduler->blockPairCutHyperedges(0, 2)) {
    ASSERT_EQ(e, 1);
  }
  for (const auto& e : scheduler->blockPairCutHyperedges(0, 3)) {
    ASSERT_EQ(e, 0);
  }
  for (const auto& e : scheduler->blockPairCutHyperedges(1, 2)) {
    ASSERT_EQ(e, 1);
  }
  for (const auto& e : scheduler->blockPairCutHyperedges(2, 3)) {
    ASSERT_EQ(e, 2);
  }
}

TEST_F(AQuotientGraphBlockScheduler, HasCorrectCutHyperedgesAfterMove) {
  scheduler->buildQuotientGraph();

  scheduler->changeNodePart(1, 1, 0);

  size_t idx = 0;
  for (const auto& e : scheduler->blockPairCutHyperedges(0, 1)) {
    unused(e);
    idx++;
  }
  ASSERT_EQ(idx, 0);
  for (const auto& e : scheduler->blockPairCutHyperedges(0, 2)) {
    ASSERT_EQ(e, 1);
  }
  for (const auto& e : scheduler->blockPairCutHyperedges(0, 3)) {
    ASSERT_EQ(e, 0);
  }
  idx = 0;
  for (const auto& e : scheduler->blockPairCutHyperedges(1, 2)) {
    unused(e);
    idx++;
  }
  ASSERT_EQ(idx, 0);
  for (const auto& e : scheduler->blockPairCutHyperedges(2, 3)) {
    ASSERT_EQ(e, 2);
  }
}
}  // namespace kahypar
