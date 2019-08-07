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

#include "kahypar/definitions.h"
#include "kahypar/datastructure/community_subhypergraph.h"
#include "tests/datastructure/hypergraph_test_fixtures.h"

using ::testing::Eq;
using ::testing::ContainerEq;
using ::testing::Test;
using kahypar::parallel::ThreadPool;

namespace kahypar {
namespace ds {

void assignCommunities(Hypergraph& hg,
                       const std::vector<std::vector<HypernodeID>>& communities) {
  PartitionID current_community = 0;
  for ( const auto& community : communities ) {
    for ( const HypernodeID& hn : community ) {
      hg.setNodeCommunity(hn, current_community);
    } 
    current_community++;
  }
}

TEST_F(AHypergraph, ExtractsCommunityZeroAsSectionHypergraph) {
  assignCommunities(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});

  Hypergraph expected_hg(7, 3, HyperedgeIndexVector { 0, 2, 6, 9 },
                        HyperedgeVector { 0, 2, 0, 1, 3, 4, 2, 5, 6 });
  assignCommunities(expected_hg, {{0, 1, 2}, {3, 4}, {5, 6}});
  std::vector<HypernodeID> expected_hn_mapping = {0, 1, 2, 3, 4, 5, 6};
  std::vector<CommunityHyperedge<Hypergraph>> expected_he_mapping =
    { {0, 0, 2}, {1, 0, 2}, {3, 0, 1} };

  auto extr_comm0 = extractCommunityInducedSectionHypergraph(hypergraph, 0, {0, 1, 2}, true);
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(expected_hg, *extr_comm0.subhypergraph), Eq(true));
  ASSERT_THAT(extr_comm0.subhypergraph_to_hypergraph_hn, Eq(expected_hn_mapping));
  ASSERT_THAT(extr_comm0.subhypergraph_to_hypergraph_he, Eq(expected_he_mapping));
  ASSERT_THAT(extr_comm0.num_hn_not_in_community, Eq(4));
  ASSERT_THAT(extr_comm0.num_pins_not_in_community, Eq(4));
}

TEST_F(AHypergraph, ExtractsCommunityOneAsSectionHypergraph) {
  assignCommunities(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});

  Hypergraph expected_hg(5, 2, HyperedgeIndexVector { 0, 4, 7 },
                         HyperedgeVector { 0, 1, 2, 3, 2, 3, 4 });
  assignCommunities(expected_hg, {{0, 1}, {2, 3}, {4}});
  std::vector<HypernodeID> expected_hn_mapping = {0, 1, 3, 4, 6};
  std::vector<CommunityHyperedge<Hypergraph>> expected_he_mapping =
    { {1, 2, 4}, {2, 0, 2} };

  auto extr_comm1 = extractCommunityInducedSectionHypergraph(hypergraph, 1, {3, 4}, true); 
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(expected_hg, *extr_comm1.subhypergraph), Eq(true));
  ASSERT_THAT(extr_comm1.subhypergraph_to_hypergraph_hn, Eq(expected_hn_mapping));
  ASSERT_THAT(extr_comm1.subhypergraph_to_hypergraph_he, Eq(expected_he_mapping));
  ASSERT_THAT(extr_comm1.num_hn_not_in_community, Eq(3));
  ASSERT_THAT(extr_comm1.num_pins_not_in_community, Eq(3));
}

TEST_F(AHypergraph, ExtractsCommunityTwoAsSectionHypergraph) {
  assignCommunities(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});

  Hypergraph expected_hg(5, 2, HyperedgeIndexVector { 0, 3, 6 },
                         HyperedgeVector { 1, 2, 4, 0, 3, 4 });
  assignCommunities(expected_hg, {{0}, {1, 2}, {3, 4}});
  std::vector<HypernodeID> expected_hn_mapping = {2, 3, 4, 5, 6};
  std::vector<CommunityHyperedge<Hypergraph>> expected_he_mapping =
    { {2, 2, 3}, {3, 1, 3} };

  auto extr_comm2 = extractCommunityInducedSectionHypergraph(hypergraph, 2, {5, 6}, true); 

  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(expected_hg, *extr_comm2.subhypergraph), Eq(true));
  ASSERT_THAT(extr_comm2.subhypergraph_to_hypergraph_hn, Eq(expected_hn_mapping));
  ASSERT_THAT(extr_comm2.subhypergraph_to_hypergraph_he, Eq(expected_he_mapping));
  ASSERT_THAT(extr_comm2.num_hn_not_in_community, Eq(3));
  ASSERT_THAT(extr_comm2.num_pins_not_in_community, Eq(3));
}

using Memento = typename Hypergraph::ContractionMemento;

Memento contract(CommunitySubhypergraph<Hypergraph>& community,
                                        const HypernodeID u, const HypernodeID v) {
  Memento memento = community.subhypergraph->contract(u, v);
  return { community.subhypergraph_to_hypergraph_hn[memento.u], 
           community.subhypergraph_to_hypergraph_hn[memento.v] };
}

void assingRandomParition(Hypergraph& hg1, Hypergraph& hg2) {
  ASSERT(hg1.currentNumNodes() == hg2.currentNumNodes());
  for ( const HypernodeID& hn : hg1.nodes() ) {
    PartitionID part = hn % 2;
    hg1.setNodePart(hn, part);
    hg2.setNodePart(hn, part);
  }
  hg1.initializeNumCutHyperedges();
  hg2.initializeNumCutHyperedges();
}

struct CommunityContraction {
  const PartitionID community;
  const HypernodeID u;
  const HypernodeID v;
};

void verifyCommunityMergeStep(ThreadPool& pool,
                              Hypergraph& hypergraph,
                              const std::vector<std::vector<HypernodeID>>& community_ids,
                              const std::vector<CommunityContraction>& contractions = {},
                              const std::vector<std::pair<PartitionID, HyperedgeID>>& disabled_hes = {}) {
  using CommunitySubhypergraph = CommunitySubhypergraph<Hypergraph>;
  using Memento = typename Hypergraph::ContractionMemento;
  
  assignCommunities(hypergraph, community_ids);
  std::vector<CommunitySubhypergraph> communities;
  communities.emplace_back(extractCommunityInducedSectionHypergraph(hypergraph, 0, community_ids[0], true));
  communities.emplace_back(extractCommunityInducedSectionHypergraph(hypergraph, 1, community_ids[1], true));
  communities.emplace_back(extractCommunityInducedSectionHypergraph(hypergraph, 2, community_ids[2], true));

  std::vector<Memento> history;
  std::vector<Memento> original_hg_history;
  for ( const CommunityContraction& contraction : contractions ) {
    Memento memento = contract(communities[contraction.community], contraction.u, contraction.v);
    history.push_back(memento);
    original_hg_history.push_back(hypergraph.contract(memento.u, memento.v));
  }

  std::vector<HyperedgeID> original_disabled_hes;
  for ( const auto& disabled_he : disabled_hes ) {
    const PartitionID community_id = disabled_he.first;
    const HyperedgeID he = disabled_he.second;
    const HyperedgeID original_he = communities[community_id].subhypergraph_to_hypergraph_he[he].original_he;
    communities[community_id].subhypergraph->removeEdge(he);
    hypergraph.removeEdge(original_he);
    original_disabled_hes.push_back(original_he);
  }

  Hypergraph merged_hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                              HyperedgeVector { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
  assignCommunities(merged_hypergraph, community_ids);
  mergeCommunityInducedSectionHypergraphs(pool, merged_hypergraph, communities, history);
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(hypergraph, merged_hypergraph), Eq(true));

  for ( const HyperedgeID& he : original_disabled_hes ) {
    hypergraph.restoreEdge(he);
    merged_hypergraph.restoreEdge(he);
  }

  assingRandomParition(hypergraph, merged_hypergraph);
  for ( int i = history.size() - 1; i >= 0; --i ) {
    merged_hypergraph.uncontract(history[i]);
    hypergraph.uncontract(original_hg_history[i]);
    ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(hypergraph, merged_hypergraph), Eq(true));
  }
}

class AParallelHypergraph : public ::testing::TestWithParam<size_t> {
 public:
  AParallelHypergraph() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    reference(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    pool(GetParam()) { }
  Hypergraph hypergraph;
  Hypergraph reference;
  ThreadPool pool;
};

INSTANTIATE_TEST_CASE_P(NumThreads,
                        AParallelHypergraph,
                        ::testing::Values(1, 2, 3));

TEST_P(AParallelHypergraph, MergesThreeCommunitySubhypergraphs) {
  verifyCommunityMergeStep(pool, hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});
}

TEST_P(AParallelHypergraph, MergesThreeCommunitySubhypergraphsWithOneContraction) {
  verifyCommunityMergeStep(pool, hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}});
}

TEST_P(AParallelHypergraph, MergesThreeCommunitySubhypergraphsWithTwoContractions) {
  verifyCommunityMergeStep(pool, hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}, {2, 3, 4}});
}

TEST_P(AParallelHypergraph, MergesThreeCommunitySubhypergraphsWithThreeContractions) {
  verifyCommunityMergeStep(pool, hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}, {1, 2, 3}, {2, 3, 4}});
}

TEST_P(AParallelHypergraph, MergesThreeCommunitySubhypergraphsWithFourContractions) {
  verifyCommunityMergeStep(pool, hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}, {1, 2, 3}, {0, 1, 0}, {2, 3, 4}});
}

TEST_P(AParallelHypergraph, MergesThreeCommunitySubhypergraphsWithFourContractionsWithOneDisabledHyperedge) {
  verifyCommunityMergeStep(pool, hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}}, {{0, 0, 2}, {1, 2, 3}, {0, 1, 0}, {2, 3, 4}},
    {{0, 0}});
}

TEST_P(AParallelHypergraph, PreparesHypergraphForParallelContractionCorrectly) {
  using CommunityHyperedge = std::vector<std::set<HypernodeID>>;
  assignCommunities(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});

  prepareForParallelCommunityAwareCoarsening(pool, hypergraph);

  std::vector<CommunityHyperedge> expected_community_he = { { {0, 2}, {}, {} },
                                                            { {0, 1}, {3, 4}, {} },
                                                            { {}, {3, 4}, {6} },
                                                            { {2}, {}, {5, 6} } };
  for ( const HyperedgeID& he : hypergraph.edges() ) {
    size_t expected_edge_size = 0;
    size_t actual_edge_size = 0;
    for ( PartitionID community = 0; community < 3; ++community ) {
      expected_edge_size += expected_community_he[he][community].size();
      if ( expected_community_he[he][community].size() > 0 ) {
        for ( const HypernodeID& pin : hypergraph.pins(he, community) ) {
          ASSERT_THAT(hypergraph.communityID(pin), Eq(community));
          ASSERT_THAT(expected_community_he[he][community].count(pin), Eq(1));
          actual_edge_size++;
        }
      }
    }
    ASSERT_THAT(actual_edge_size, Eq(expected_edge_size));
    ASSERT_THAT(hypergraph.edgeSize(he), Eq(expected_edge_size));
  }
}

void verifyParallelContractionStep(ThreadPool& pool,
                                   Hypergraph& hypergraph,
                                   Hypergraph& reference,
                                   const std::vector<std::vector<HypernodeID>>& community_ids,
                                   const std::vector<std::vector<Memento>>& community_contractions,
                                   const std::vector<std::vector<HyperedgeID>> disabled_he) {
  PartitionID num_communities = community_contractions.size();
  ASSERT_THAT(community_ids.size(), Eq(num_communities));
  ASSERT_THAT(community_contractions.size(), Eq(num_communities));
  ASSERT_THAT(disabled_he.size(), Eq(num_communities));
  assignCommunities(hypergraph, community_ids);
  assignCommunities(reference, community_ids);
  prepareForParallelCommunityAwareCoarsening(pool, hypergraph);

  size_t num_threads = pool.size();
  std::atomic<size_t> active_threads(0);
  std::vector<Memento> history; 
  for ( PartitionID c = 0; c < num_communities; ++c ) {
    std::vector<Memento> contractions = community_contractions[c];
    std::vector<HyperedgeID> community_disabled_he = disabled_he[c];
    pool.enqueue([&hypergraph, &active_threads, &num_threads, c, contractions, community_disabled_he]() {
      active_threads++;
      while(active_threads < num_threads) { }

      for ( const Memento& memento : contractions ) {
        hypergraph.parallelContract(c, memento.u, memento.v);
      }
      for ( const HyperedgeID& he : community_disabled_he ) {
        hypergraph.removeEdge(he, c);
      }
    });
    for ( const Memento& memento : contractions ) {
      history.push_back(reference.contract(memento.u, memento.v));
    }
    for ( const HyperedgeID& he : community_disabled_he ) {
      reference.removeEdge(he);
    }
  }
  pool.loop_until_empty();

  undoPreparationForParallelCommunityAwareCoarsening(pool, hypergraph, history);
  reference.removeAllDisabledEdgesFromIncidentNets();
  ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(reference, hypergraph), Eq(true));

  for ( PartitionID c = 0; c < num_communities; ++c ) {
    for ( const HyperedgeID& he : disabled_he[c] ) {
      hypergraph.restoreEdgeAfterParallelCoarsening(he);
      reference.restoreEdgeAfterParallelCoarsening(he);
    }
  }

  assingRandomParition(reference, hypergraph);
  for ( int i = history.size() - 1; i >= 0; --i ) {
    reference.uncontract(history[i]);
    hypergraph.uncontract(history[i]);
    ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(reference, hypergraph), Eq(true));
  }
}

TEST_P(AParallelHypergraph, DoingParallelContractionOfTwoHypernodes1) {
  verifyParallelContractionStep(pool, hypergraph, reference,
                                { {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, {}, { {5, 6} } },
                                { {}, {}, {} });
}

TEST_P(AParallelHypergraph, DoingParallelContractionOfTwoHypernodes2) {
  verifyParallelContractionStep(pool, hypergraph, reference,
                                { {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, { {3, 4} }, {} },
                                { {}, {}, {} });
}

TEST_P(AParallelHypergraph, DoingParallelContractionOfTwoHypernodes3) {
  verifyParallelContractionStep(pool, hypergraph, reference,
                                { {0, 1, 2}, {3, 4}, {5, 6} },
                                { { }, { {3, 4} }, { {5, 6} } },
                                { {}, {}, {} });
}

TEST_P(AParallelHypergraph, DoingParallelContractionOfThreeHypernodes) {
  verifyParallelContractionStep(pool, hypergraph, reference,
                                { {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, { {3, 4} }, { {5, 6} } },
                                { {}, {}, {} });
}

TEST_P(AParallelHypergraph, DoingParallelContractionOfFourHypernodes) {
  verifyParallelContractionStep(pool, hypergraph, reference,
                                { {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2}, {1, 0} }, { {3, 4} }, { {5, 6} } },
                                { {}, {}, {} });
}

TEST_P(AParallelHypergraph, DoingParallelContractionOfTwoHypernodesWithRemovingOneEdge) {
  verifyParallelContractionStep(pool, hypergraph, reference,
                                { {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, { {3, 4} }, {} },
                                { {0}, {}, {} });
}

TEST_P(AParallelHypergraph, DoingParallelContractionOfTwoHypernodesWithRemovingTwoEdges) {
  verifyParallelContractionStep(pool, hypergraph, reference,
                                { {0, 1, 2}, {3, 4}, {5, 6} },
                                { { {0, 2} }, { {3, 4} }, {} },
                                { {0}, {}, {3} });
}

}  // namespace ds
}  // namespace kahypar
