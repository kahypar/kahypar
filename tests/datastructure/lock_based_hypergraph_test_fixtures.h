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
#include "kahypar/datastructure/lock_based_hypergraph.h"
#include "kahypar/utils/thread_pool.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
namespace ds {

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

class ALockBasedHypergraph : public ::testing::TestWithParam<size_t> {
 private:
  using LockBasedHypergraph = kahypar::ds::LockBasedHypergraph<Hypergraph>;
  using ThreadPool = kahypar::parallel::ThreadPool;
  using Memento = typename Hypergraph::ContractionMemento;
  using LockBasedMemento = typename LockBasedHypergraph::LockBasedContractionMemento;
  using Mutex = std::shared_timed_mutex;
  template<typename Key>
  using MutexVector = kahypar::ds::MutexVector<Key, Mutex>;


 public:
  ALockBasedHypergraph() :
    context(),
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    hn_mutex(7),
    he_mutex(4),
    last_touched(4),
    active(7),
    lock_based_hypergraph(hypergraph, hn_mutex, he_mutex, last_touched, active),
    reference(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    pool(nullptr) { 
    context.shared_memory.num_threads = GetParam();
    context.shared_memory.working_packages = GetParam();
    lock_based_hypergraph.unmarkActiveAfterContractions();
    pool = std::make_unique<ThreadPool>(context);
  }

  void verifyParallelContractionStep(const std::vector<Memento>& contractions) {
    size_t num_threads = std::min(pool->size(), contractions.size());
    pool->set_working_packages(num_threads);
    std::atomic<size_t> active_threads(0);
    
    std::vector<std::future<std::vector<LockBasedMemento>>> results =
      pool->parallel_for(
        [this, &active_threads, &num_threads, &contractions](const size_t start, const size_t end) {
        active_threads++;
        while(active_threads < num_threads) { }

        std::vector<LockBasedMemento> history;
        for ( size_t i = start; i < end; ++i ) {
          LockBasedMemento memento = this->lock_based_hypergraph.contract(contractions[i].u, contractions[i].v);
          if ( memento.contraction_id != std::numeric_limits<HypernodeID>::max() ) {
            history.push_back(memento);
          }
        }
        return history;
      }, 0UL, contractions.size() );
    pool->loop_until_empty();

    std::vector<LockBasedMemento> parallel_history;
    for ( auto& fut : results ) {
      std::vector<LockBasedMemento> tmp_history = std::move(fut.get());
      parallel_history.insert(parallel_history.end(), tmp_history.begin(), tmp_history.end());
    }
    std::sort(parallel_history.begin(), parallel_history.end(), 
              [&](const LockBasedMemento& lhs, const LockBasedMemento& rhs) {
                return lhs.contraction_id < rhs.contraction_id;
    });
    lock_based_hypergraph.restoreHypergraphStats(*pool);

    std::vector<Memento> history;
    for ( const Memento& memento : contractions ) {
      history.emplace_back(reference.contract(memento.u, memento.v));
    }

    ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(reference, hypergraph), Eq(true));

    assingRandomPartition(hypergraph, reference);
    ASSERT_THAT(parallel_history.size(), Eq(history.size()));
    for ( int i = history.size() - 1; i >= 0; --i ) {
      reference.uncontract(history[i]);
      hypergraph.uncontract(parallel_history[i].memento);
    }
    ASSERT_THAT(verifyEquivalenceWithoutPartitionInfo(reference, hypergraph), Eq(true));
  }

  Context context;
  Hypergraph hypergraph;
  MutexVector<HypernodeID> hn_mutex;
  MutexVector<HyperedgeID> he_mutex;
  std::vector<HyperedgeID> last_touched;
  std::vector<bool> active;
  LockBasedHypergraph lock_based_hypergraph;
  Hypergraph reference;
  std::unique_ptr<ThreadPool> pool;
};

INSTANTIATE_TEST_CASE_P(NumThreads,
                        ALockBasedHypergraph,
                        ::testing::Values(1, 2, 3));


}  // namespace ds
}  // namespace kahypar
