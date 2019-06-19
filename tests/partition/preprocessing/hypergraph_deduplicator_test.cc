/*******************************************************************************
 * This file is part of KaHyPar.
 *
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

#include "gmock/gmock.h"


#include "kahypar/definitions.h"

#include "kahypar/partition/preprocessing/hypergraph_deduplicator.h"


namespace kahypar {
TEST(TheHypergraphDeduplicator, DoesNothingWhenThereIsNothingToDeduplicate) {
  Hypergraph hypergraph(3, 2, HyperedgeIndexVector { 0, 2, 4 },
                        HyperedgeVector { 0, 1, 1, 2 });

  Context context;
  HypergraphDeduplicator deduplicator;

  deduplicator.deduplicate(hypergraph, context);

  ASSERT_EQ(hypergraph.currentNumEdges(), 2);
  ASSERT_EQ(hypergraph.currentNumNodes(), 3);
}

TEST(TheHypergraphDeduplicator, RemovesRedundancy) {
  Hypergraph hypergraph(5, 7, HyperedgeIndexVector { 0, 1, 4, 6, 10, 13, 14, 17 },
                        HyperedgeVector { 0, 1, 2, 3, 0, 1, 1, 2, 3, 4, 1, 2, 3, 0, 1, 2, 3 });

  Context context;
  HypergraphDeduplicator deduplicator;

  deduplicator.deduplicate(hypergraph, context);

  ASSERT_EQ(hypergraph.currentNumEdges(), 4);
  ASSERT_EQ(hypergraph.edgeWeight(0), 2);
  ASSERT_EQ(hypergraph.edgeWeight(1), 3);
  ASSERT_EQ(hypergraph.edgeWeight(2), 1);
  ASSERT_EQ(hypergraph.edgeWeight(3), 1);
  ASSERT_EQ(hypergraph.currentNumNodes(), 4);
  ASSERT_EQ(hypergraph.nodeWeight(2), 2);
}

TEST(TheHypergraphDeduplicator, RestoresRedundancy) {
  Hypergraph hypergraph(5, 7, HyperedgeIndexVector { 0, 1, 4, 6, 10, 13, 14, 17 },
                        HyperedgeVector { 0, 1, 2, 3, 0, 1, 1, 2, 3, 4, 1, 2, 3, 0, 1, 2, 3 });

  Context context;
  HypergraphDeduplicator deduplicator;

  deduplicator.deduplicate(hypergraph, context);

  // currently, we assume partitioning inbetween deduplication
  // redudancy restore
  hypergraph.setNodePart(0, 1);
  hypergraph.setNodePart(1, 1);
  hypergraph.setNodePart(2, 0);
  hypergraph.setNodePart(4, 1);
  hypergraph.initializeNumCutHyperedges();

  deduplicator.restoreRedundancy(hypergraph);

  ASSERT_EQ(hypergraph.currentNumEdges(), 7);
  ASSERT_EQ(hypergraph.edgeWeight(0), 1);
  ASSERT_EQ(hypergraph.edgeWeight(1), 1);
  ASSERT_EQ(hypergraph.edgeWeight(2), 1);
  ASSERT_EQ(hypergraph.edgeWeight(3), 1);
  ASSERT_EQ(hypergraph.currentNumNodes(), 5);
  ASSERT_EQ(hypergraph.nodeWeight(2), 1);
}
}  // namespace kahypar
