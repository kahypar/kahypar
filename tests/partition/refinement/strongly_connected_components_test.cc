/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "kahypar/datastructure/graph.h"
#include "kahypar/partition/refinement/flow/strongly_connected_components.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {

TEST(SCCs, AreComputedCorrectly) {

  std::vector<NodeID> adj_array;
  std::vector<Edge> edges;

  edges.emplace_back(Edge{1,0});
  edges.emplace_back(Edge{2,0});
  edges.emplace_back(Edge{3,0});
  edges.emplace_back(Edge{0,0});
  edges.emplace_back(Edge{4,0});

  adj_array.push_back(0);
  adj_array.push_back(1);
  adj_array.push_back(3);
  adj_array.push_back(4);
  adj_array.push_back(5);
  adj_array.push_back(5);

  Graph graph(adj_array, edges);

  // for (const auto u : graph.nodes()){
  //   for (const auto& edge : graph.incidentEdges(u)) {
  //     LOG << "(" << u << "," << edge.target_node << ")";
  //   }
  // }

  StronglyConnectedComponents sccs(graph.numNodes());

  sccs.compute(graph);

  ASSERT_TRUE(graph.clusterID(0) == graph.clusterID(1));
  ASSERT_TRUE(graph.clusterID(0) == graph.clusterID(2));

  ASSERT_TRUE(graph.clusterID(0) != graph.clusterID(3));
  ASSERT_TRUE(graph.clusterID(0) != graph.clusterID(4));

  ASSERT_TRUE(graph.clusterID(3) != graph.clusterID(4));

}


} // namespace kahypar
