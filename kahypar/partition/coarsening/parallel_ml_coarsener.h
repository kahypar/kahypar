/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <limits>
#include <string>
#include <vector>
#include <queue>
#include <chrono>
#include <mutex>
#include <shared_mutex>


#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/datastructure/parallel_hypergraph.h"
#include "kahypar/datastructure/fast_hash_table.h"
#include "kahypar/datastructure/mutex_vector.h"
#include "kahypar/utils/thread_pool.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_partition_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/coarsening/vertex_pair_rater.h"
#include "kahypar/partition/coarsening/vertex_pair_coarsener_base.h"
#include "kahypar/partition/coarsening/lock_based_hypergraph_pruner.h"

namespace kahypar {

template <class ScorePolicy = HeavyEdgeScore,
          class HeavyNodePenaltyPolicy = NoWeightPenalty,
          class CommunityPolicy = UseCommunityStructure,
          class RatingPartitionPolicy = NormalPartitionPolicy,
          class AcceptancePolicy = BestRatingPreferringUnmatched<>,
          class FixedVertexPolicy = AllowFreeOnFixedFreeOnFreeFixedOnFixed,
          typename RatingType = RatingType>
class ParallelMLCoarsener final : public ICoarsener,
                                           protected VertexPairCoarsenerBase<>{
 private:
  static constexpr bool debug = false;
  static constexpr size_t BATCH_SIZE = 1000;
  static constexpr HypernodeID kInvalidTarget = std::numeric_limits<HypernodeID>::max();

  using Base = VertexPairCoarsenerBase;
  using Rater = VertexPairRater<ScorePolicy,
                                HeavyNodePenaltyPolicy,
                                CommunityPolicy,
                                RatingPartitionPolicy,
                                AcceptancePolicy,
                                FixedVertexPolicy,
                                RatingType>;
  using Rating = typename Rater::Rating;
  using ThreadPool = kahypar::parallel::ThreadPool;
  using Mutex = std::mutex;
  using Memento = typename Hypergraph::Memento;

 public:
  ParallelMLCoarsener(Hypergraph& hypergraph, const Context& context,
              const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _current_num_nodes(_hg.initialNumNodes()),
    _hn_mutex(_hg.initialNumNodes()),
    _he_mutex(_hg.initialNumEdges()),
    _current_batch_size(0),
    _batch(_context.shared_memory.num_threads),
    _batches_mutex(),
    _batches() { }

  ~ParallelMLCoarsener() override = default;

  ParallelMLCoarsener(const ParallelMLCoarsener&) = delete;
  ParallelMLCoarsener& operator= (const ParallelMLCoarsener&) = delete;

  ParallelMLCoarsener(ParallelMLCoarsener&&) = delete;
  ParallelMLCoarsener& operator= (ParallelMLCoarsener&&) = delete;

 private:
  void coarsenImpl(const HypernodeID limit) override final {
    ASSERT(_context.shared_memory.pool, "Thread pool not initialized");
    ThreadPool& pool = *_context.shared_memory.pool;

    for ( const HypernodeID& hn : _hg.nodes() ) {
      addHypernodeToQueue(0, hn);
    }
    enqueueBatch();

    for ( size_t thread = 0; thread < _context.shared_memory.num_threads; ++thread ) {
      pool.enqueue([this, thread, limit]() {
        mlCoarsening(thread, limit);
      });
    }
    pool.loop_until_empty();

    restoreHypergraphStats(pool);
  }

  void mlCoarsening(const PartitionID thread_id, const HypernodeID limit) {
    LockBasedHypergraphPruner pruner(_hn_mutex, _he_mutex);

    int pass_nr = 0;
    while( _current_num_nodes > limit && !_batches.empty() ) {
      DBG << V(thread_id) << V(pass_nr) << V(_current_num_nodes) 
          << V(_batches.size()) << V(limit);

      std::vector<HypernodeID> batch = popBatch();
      Randomize::instance().shuffleVector(batch, batch.size());

      for ( const HypernodeID& hn : batch ) {
        std::unique_lock<Mutex> hn_lock(_hn_mutex[hn]);
        if ( _hg.nodeIsEnabled(hn) ) {

          HypernodeID target = kInvalidTarget;
          for ( const HyperedgeID& he : _hg.incidentEdges(hn) ) {
            std::lock_guard<Mutex> he_lock(_he_mutex[he]);
            for ( const HypernodeID& pin : _hg.pins(he) ) {
              if ( pin != hn && _hg.nodeIsEnabled(pin) ) {
                target = pin;
                break;
              }
            }
            if ( target != kInvalidTarget ) {
              break;
            }
          }

          if ( target != kInvalidTarget ) {
            hn_lock.unlock();
            CoarseningMemento memento(_hg.parallelContract(hn, target, _hn_mutex, _he_mutex));
            memento.thread_id = thread_id;

            if ( memento.contraction_memento.u == hn && 
                 memento.contraction_memento.v == target ) {
              hn_lock.lock();
              if ( _hg.nodeIsEnabled(hn) ) {
                pruner.removeSingleNodeHyperedges(_hg, memento);
                // pruner.removeParallelHyperedges(_hg, memento);
              }
              hn_lock.unlock();
              --_current_num_nodes;
            }
          }

          if ( _hg.nodeIsEnabled(hn) ) {
            addHypernodeToQueue(thread_id, hn);
          }

          if ( _current_num_nodes <= limit ) {
            break;
          }
        }
      }

      enqueueBatch();
      ++pass_nr;
    }
  }

  void restoreHypergraphStats(ThreadPool& pool) {
    _hg.setCurrentNumNodes(_current_num_nodes);
    std::vector<std::future<std::pair<HyperedgeID, HypernodeID>>> results =
      pool.parallel_for([this](const HyperedgeID start, const HyperedgeID end) {
        HyperedgeID num_hyperedges = 0;
        HypernodeID num_pins = 0;
        for ( HyperedgeID he = start; he < end; ++he ) {
          if ( this->_hg.edgeIsEnabled(he) ) {
            ++num_hyperedges;
            num_pins += this->_hg.edgeSize(he);
          }
        }
        return std::make_pair(num_hyperedges, num_pins);
      }, (HyperedgeID) 0, _hg.initialNumEdges());
    pool.loop_until_empty();

    HyperedgeID num_hyperedges = 0;
    HypernodeID num_pins = 0;
    for ( auto& fut : results ) {
      auto sizes = fut.get();
      num_hyperedges += sizes.first;
      num_pins += sizes.second;
    }
    _hg.setCurrentNumEdges(num_hyperedges);
    _hg.setCurrentNumPins(num_pins);
  }


  void addHypernodeToQueue(const size_t thread, const HypernodeID hn) {
    std::shared_lock<std::shared_timed_mutex> lock(_batches_mutex);
    _batch[thread].push_back(hn);
    ++_current_batch_size;
    if ( _current_batch_size >= BATCH_SIZE ) {
      lock.unlock();
      enqueueBatch();
    }
  }

  void enqueueBatch() {
    std::unique_lock<std::shared_timed_mutex> lock(_batches_mutex);
    std::vector<HypernodeID> new_batch;
    for ( size_t thread = 0; thread < _context.shared_memory.num_threads; ++thread ) {
      new_batch.insert(new_batch.end(), _batch[thread].begin(), _batch[thread].end());
      _batch[thread].clear();
    }
    if ( new_batch.size() > 0 ) {
      _batches.emplace(std::move(new_batch));
      _current_batch_size = 0;
    }
  }

  std::vector<HypernodeID> popBatch() {
    std::unique_lock<std::shared_timed_mutex> lock(_batches_mutex);
    if ( !_batches.empty() ) {
      std::vector<HypernodeID> batch = std::move(_batches.front());
      _batches.pop();
      return batch;
    } else {
      return std::vector<HypernodeID>();
    }
  }

  bool uncoarsenImpl(IRefiner& refiner) override final {
    return doUncoarsen(refiner);
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_context;
  using Base::_history;
  using Base::_max_hn_weights;

  std::atomic<HypernodeID> _current_num_nodes;
  kahypar::ds::MutexVector<HypernodeID, Mutex> _hn_mutex;
  kahypar::ds::MutexVector<HyperedgeID, Mutex> _he_mutex;

  std::atomic<HypernodeID> _current_batch_size;
  std::vector<std::vector<HypernodeID>> _batch;

  std::shared_timed_mutex _batches_mutex;
  std::queue<std::vector<HypernodeID>> _batches;
};
}  // namespace kahypar
