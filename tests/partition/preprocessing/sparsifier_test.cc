/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/preprocessing/min_hash_sparsifier.h"

using ::testing::Eq;

namespace kahypar {
TEST(TheLSHSparsifier, HandlesWeightedNodesAndEdges) {
  Hypergraph hypergraph = io::createHypergraphFromFile(
    std::string("test_instances/WeightedHypergraph.hgr"), 2);
  Context context;
  context.partition.k = 2;
  context.preprocessing.enable_min_hash_sparsifier = true;
  context.preprocessing.min_hash_sparsifier.min_median_he_size = 28;
  context.preprocessing.min_hash_sparsifier.max_hyperedge_size = 1200;
  context.preprocessing.min_hash_sparsifier.max_cluster_size = 50;
  context.preprocessing.min_hash_sparsifier.min_cluster_size = 2;
  context.preprocessing.min_hash_sparsifier.num_hash_functions = 5;
  context.preprocessing.min_hash_sparsifier.combined_num_hash_functions = 100;

  Hypergraph sparse_hypergraph = MinHashSparsifier().buildSparsifiedHypergraph(hypergraph,
                                                                               context);

  ASSERT_EQ(sparse_hypergraph.currentNumNodes(), 6);
  ASSERT_EQ(sparse_hypergraph.currentNumEdges(), 1);
  ASSERT_EQ(sparse_hypergraph.edgeWeight(0), 385);
  ASSERT_EQ(sparse_hypergraph.nodeWeight(0), 50);
  ASSERT_EQ(sparse_hypergraph.nodeWeight(1), 50);
  ASSERT_EQ(sparse_hypergraph.nodeWeight(2), 50);
  ASSERT_EQ(sparse_hypergraph.nodeWeight(3), 50);
  ASSERT_EQ(sparse_hypergraph.nodeWeight(4), 50);
  ASSERT_EQ(sparse_hypergraph.nodeWeight(5), 50);
}
}  // namespace kahypar
