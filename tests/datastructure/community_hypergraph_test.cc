/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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

#include <iostream>

#include "gmock/gmock.h"

#include "tests/datastructure/community_hypergraph_test_fixtures.h"

using ::testing::Eq;
using ::testing::ContainerEq;
using ::testing::Test;

namespace kahypar {
namespace ds {

TEST_P(ACommunityHypergraph, InitializeCommunityHypergraphWithSinglePinEdges) {
  using CommunityHyperedge = std::vector<std::set<HypernodeID>>;
  
  community_hypergraph->initialize();

  std::vector<CommunityHyperedge> expected_community_he = { { {0, 2}, {}, {} },
                                                            { {0, 1}, {3, 4}, {} },
                                                            { {}, {3, 4}, {6} },
                                                            { {2}, {}, {5, 6} } };
                                                            
  for ( const HyperedgeID& he : community_hypergraph->edges() ) {
    size_t expected_edge_size = 0;
    size_t actual_edge_size = 0;
    for ( PartitionID community = 0; community < 3; ++community ) {
      expected_edge_size += expected_community_he[he][community].size();
      if ( expected_community_he[he][community].size() > 0 ) {
        for ( const HypernodeID& pin : community_hypergraph->pins(he, community) ) {
          ASSERT_THAT(community_hypergraph->communityID(pin), Eq(community));
          ASSERT_THAT(expected_community_he[he][community].count(pin), Eq(1));
          actual_edge_size++;
        }
      }
    }
    ASSERT_THAT(actual_edge_size, Eq(expected_edge_size));
    ASSERT_THAT(hypergraph.edgeSize(he), Eq(expected_edge_size));
  }
}

TEST_P(ACommunityHypergraph, InitializeCommunityHypergraphWithoutSinglePinEdges) {
  using CommunityHyperedge = std::vector<std::set<HypernodeID>>;
  
  context.shared_memory.remove_single_pin_community_hyperedges = true;
  community_hypergraph->initialize();

  std::vector<CommunityHyperedge> expected_community_he = { { {0, 2}, {}, {} },
                                                            { {0, 1}, {3, 4}, {} },
                                                            { {}, {3, 4}, {} },
                                                            { {}, {}, {5, 6} } };
                                                            
  for ( const HyperedgeID& he : community_hypergraph->edges() ) {
    size_t expected_edge_size = 0;
    size_t actual_edge_size = 0;
    for ( PartitionID community = 0; community < 3; ++community ) {
      expected_edge_size += expected_community_he[he][community].size();
      if ( expected_community_he[he][community].size() > 0 ) {
        for ( const HypernodeID& pin : community_hypergraph->pins(he, community) ) {
          ASSERT_THAT(community_hypergraph->communityID(pin), Eq(community));
          ASSERT_THAT(expected_community_he[he][community].count(pin), Eq(1));
          actual_edge_size++;
        }
      }
    }
    ASSERT_THAT(actual_edge_size, Eq(expected_edge_size));
  }
}

TEST_P(ACommunityHypergraph, CommunityNodeIdsFormDisjointContinousRanges) {
  community_hypergraph->initialize();
  std::vector<HypernodeID> expected_community_node_ids = { 0, 1, 2, 0, 1, 0, 1 };
  for ( const HypernodeID& hn : community_hypergraph->nodes() ) {
    ASSERT_THAT(community_hypergraph->communityNodeID(hn), Eq(expected_community_node_ids[hn]));
  }
}

TEST_P(ACommunityHypergraph, DoingParallelContractionOfTwoHypernodes1) {
  verifyParallelContractionStep({ {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, {}, { {5, 6} } },
                                { {}, {}, {} });
}

TEST_P(ACommunityHypergraph, DoingParallelContractionOfTwoHypernodes2) {
  verifyParallelContractionStep({ {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, { {3, 4} }, {} },
                                { {}, {}, {} });
}

TEST_P(ACommunityHypergraph, DoingParallelContractionOfTwoHypernodes3) {
  verifyParallelContractionStep({ {0, 1, 2}, {3, 4}, {5, 6} },
                                { { }, { {3, 4} }, { {5, 6} } },
                                { {}, {}, {} });
}

TEST_P(ACommunityHypergraph, DoingParallelContractionOfThreeHypernodes) {
  verifyParallelContractionStep({ {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, { {3, 4} }, { {5, 6} } },
                                { {}, {}, {} });
}

TEST_P(ACommunityHypergraph, DoingParallelContractionOfFourHypernodes) {
  verifyParallelContractionStep({ {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2}, {1, 0} }, { {3, 4} }, { {5, 6} } },
                                { {}, {}, {} });
}

TEST_P(ACommunityHypergraph, DoingParallelContractionOfTwoHypernodesWithRemovingOneEdge) {
  verifyParallelContractionStep({ {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, { {3, 4} }, {} },
                                { {0}, {}, {} });
}

} // namespace ds
} // namespace kahypar