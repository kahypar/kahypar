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

#pragma once

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/datastructure/community_hypergraph.h"
#include "kahypar/utils/thread_pool.h"

using ::testing::Test;
using ::testing::Eq;

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
  hg.setNumCommunities(communities.size());
}

void assingRandomPartition(Hypergraph& hg1, Hypergraph& hg2) {
  ASSERT(hg1.currentNumNodes() == hg2.currentNumNodes());
  for ( const HypernodeID& hn : hg1.nodes() ) {
    PartitionID part = hn % 2;
    hg1.setNodePart(hn, part);
    hg2.setNodePart(hn, part);
  }
  hg1.initializeNumCutHyperedges();
  hg2.initializeNumCutHyperedges();
}

class ACommunityHypergraph : public ::testing::TestWithParam<size_t> {
 private:
  using ThreadPool = kahypar::parallel::ThreadPool;
  using CommunityHypergraph = kahypar::ds::CommunityHypergraph<Hypergraph>;
  using Memento = typename Hypergraph::ContractionMemento;

 public:
  ACommunityHypergraph() :
    context(),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    community_hypergraph(nullptr),
    reference(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    pool(nullptr) { 
    assignCommunities(hypergraph, {{0, 1, 2}, {3, 4}, {5, 6}});
    assignCommunities(reference, {{0, 1, 2}, {3, 4}, {5, 6}});
    context.shared_memory.num_threads = GetParam();
    context.shared_memory.working_packages = 3;
    pool = std::make_unique<ThreadPool>(context);
    community_hypergraph = std::make_shared<CommunityHypergraph>(hypergraph, context, *pool);
  }

  void verifyParallelContractionStep(const std::vector<std::vector<HypernodeID>>& community_ids,
                                     const std::vector<std::vector<Memento>>& community_contractions,
                                     const std::vector<std::vector<HyperedgeID>> disabled_he) {
    PartitionID num_communities = community_contractions.size();
    ASSERT_THAT(community_ids.size(), Eq(num_communities));
    ASSERT_THAT(community_contractions.size(), Eq(num_communities));
    ASSERT_THAT(disabled_he.size(), Eq(num_communities));

    community_hypergraph->initialize();

    size_t num_threads = pool->size();
    std::atomic<size_t> active_threads(0);
    std::vector<Memento> history;
    std::unordered_map<HyperedgeID, size_t> disabled_he_to_size;
    for ( PartitionID c = 0; c < num_communities; ++c ) {
      std::vector<Memento> contractions = community_contractions[c];
      std::vector<HyperedgeID> community_disabled_he = disabled_he[c];
      pool->enqueue([this, &active_threads, &num_threads, 
                     c, contractions, community_disabled_he]() {
        active_threads++;
        while(active_threads < num_threads) { }

        for ( const Memento& memento : contractions ) {
          this->community_hypergraph->contract(memento.u, memento.v);
        }
        for ( const HyperedgeID& he : community_disabled_he ) {
          this->community_hypergraph->removeEdge(he, c);
        }
      });
      for ( const Memento& memento : contractions ) {
        history.push_back(reference.contract(memento.u, memento.v));
      }
      for ( const HyperedgeID& he : community_disabled_he ) {
        disabled_he_to_size[he] = reference.edgeSize(he);
        reference.removeEdge(he);
      }
    }
    
    pool->loop_until_empty();

    community_hypergraph->undo(history);
    ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(reference, hypergraph), Eq(true));

    for ( PartitionID c = 0; c < num_communities; ++c ) {
      for ( const HyperedgeID& he : disabled_he[c] ) {
        hypergraph.restoreEdge(he, disabled_he_to_size[he]);
        reference.restoreEdge(he, disabled_he_to_size[he]);
      }
    }

    assingRandomPartition(reference, hypergraph);
    for ( int i = history.size() - 1; i >= 0; --i ) {
      reference.uncontract(history[i]);
      hypergraph.uncontract(history[i]);
      ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(reference, hypergraph), Eq(true));
    }
  }

  Context context;
  Hypergraph hypergraph;
  std::shared_ptr<CommunityHypergraph> community_hypergraph;
  Hypergraph reference;
  std::unique_ptr<ThreadPool> pool;
};

INSTANTIATE_TEST_CASE_P(NumThreads,
                        ACommunityHypergraph,
                        ::testing::Values(1, 2, 3));


}  // namespace ds
}  // namespace kahypar
