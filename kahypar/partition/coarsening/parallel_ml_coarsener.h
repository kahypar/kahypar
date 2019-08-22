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
#include "kahypar/partition/coarsening/lock_based_vertex_pair_rater.h"
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
  using Rater = LockBasedVertexPairRater<ScorePolicy,
                                         HeavyNodePenaltyPolicy,
                                         CommunityPolicy,
                                         RatingPartitionPolicy,
                                         AcceptancePolicy,
                                         FixedVertexPolicy,
                                         RatingType>;
  using Rating = typename Rater::Rating;
  using ThreadPool = kahypar::parallel::ThreadPool;
  using Mutex = std::shared_timed_mutex;
  template<typename Key>
  using MutexVector = kahypar::ds::MutexVector<Key, Mutex>;
  using Memento = typename Hypergraph::Memento;
  using History = std::vector<CoarseningMemento>;
  using CurrentMaxNodeWeight = typename CoarsenerBase::CurrentMaxNodeWeight;

  struct ParallelCoarseningResult {

    ParallelCoarseningResult(const PartitionID thread_id,
                             const HypernodeID num_hypernodes,
                             MutexVector<HypernodeID>& hn_mutex,
                             MutexVector<HyperedgeID>& he_mutex ) :
      thread_id(thread_id),
      history(),
      pruner(num_hypernodes, hn_mutex, he_mutex),
      max_hn_weights() { }

    PartitionID thread_id;
    History history;
    LockBasedHypergraphPruner pruner;
    std::vector<CurrentMaxNodeWeight> max_hn_weights;
  };

 public:
  ParallelMLCoarsener(Hypergraph& hypergraph, const Context& context,
              const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _weight_of_heaviest_node(weight_of_heaviest_node),
    _pruner(),
    _contraction_index(0),
    _current_num_nodes(_hg.initialNumNodes()),
    _num_nodes_before_pass(_hg.initialNumNodes()),
    _hn_mutex(_hg.initialNumNodes()),
    _he_mutex(_hg.initialNumEdges()),
    _current_batch_size(0),
    _batch(_context.shared_memory.num_threads),
    _batches_mutex(),
    _batches(),
    _tmp_batches_mutex(),
    _tmp_batches(),
    _already_matched(_hg.initialNumNodes()),
    _active(_hg.initialNumNodes()),
    _last_touched(_hg.initialNumEdges(), 0) { }

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
    swapTmpBatches();
    
    std::vector<std::future<ParallelCoarseningResult>> results;
    for ( size_t thread = 0; thread < _context.shared_memory.num_threads; ++thread ) {
      results.push_back(
        pool.enqueue([this, thread, limit]() {
          return mlCoarsening(thread, limit);
        }));
    }
    pool.loop_until_empty();

    _history.resize(_contraction_index);
    for ( auto& fut : results ) {
      ParallelCoarseningResult result = std::move(fut.get());
      _pruner.emplace_back(std::move(result.pruner));
      for ( CoarseningMemento& memento : result.history ) {
        HypernodeID contraction_index = memento.contraction_index;
        _history[contraction_index] = std::move(memento);
      }

      size_t pos = 0;
      for ( CurrentMaxNodeWeight& max_node_weight : result.max_hn_weights ) {
        HypernodeID current_num_nodes = _hg.initialNumNodes() - (max_node_weight.num_nodes + 1);
        max_node_weight.num_nodes = current_num_nodes;
        while ( pos < _max_hn_weights.size() && current_num_nodes < _max_hn_weights[pos].num_nodes ) {
          ++pos;
        }
        if ( pos < _max_hn_weights.size() ) {
          if ( current_num_nodes > _max_hn_weights[pos].num_nodes &&
               max_node_weight.max_weight > _max_hn_weights[pos].max_weight ) {
            _max_hn_weights[pos++] = std::move( max_node_weight );
          }
        } else {
          _max_hn_weights.emplace_back(std::move(max_node_weight));
        }
      }
    }

    restoreHypergraphStats(pool);
  }

  ParallelCoarseningResult mlCoarsening(const PartitionID thread_id, const HypernodeID limit) {
    ParallelCoarseningResult result(thread_id, _hg.initialNumNodes(), _hn_mutex, _he_mutex);
    result.max_hn_weights.emplace_back( CurrentMaxNodeWeight { _current_num_nodes, _weight_of_heaviest_node } );
    Rater rater(_hg, _context, _hn_mutex, _he_mutex);

    while( _current_num_nodes > limit && !_batches.empty() ) {
      DBG << V(thread_id) << V(_current_num_nodes) 
          << V(_batches.size()) << V(limit);

      std::vector<HypernodeID> batch = popBatch();
      Randomize::instance().shuffleVector(batch, batch.size());

      for ( const HypernodeID& hn : batch ) {
        if ( _hg.nodeIsEnabled(hn) ) {
          Rating rating = rater.rate(hn, _already_matched);
          
          if ( rating.target != kInvalidTarget ) {
            HypernodeID contraction_index = 0;
            CoarseningMemento memento(_hg.parallelContract(
              hn, rating.target, _hn_mutex, _he_mutex, 
              _contraction_index, contraction_index, _active, _last_touched));
            memento.thread_id = thread_id;

            if ( memento.contraction_memento.u == hn && 
                memento.contraction_memento.v == rating.target ) {
              _already_matched.set(memento.contraction_memento.u, true);
              _already_matched.set(memento.contraction_memento.v, true);
              memento.contraction_index = contraction_index;
              result.pruner.removeSingleNodeHyperedges(_hg, memento);
              result.pruner.removeParallelHyperedges(_hg, memento, _last_touched);
              result.history.emplace_back(std::move(memento));
              if ( result.max_hn_weights.back().max_weight < _hg.nodeWeight(hn) ) {
                result.max_hn_weights.emplace_back( CurrentMaxNodeWeight {contraction_index, _hg.nodeWeight(hn)} );
              }
              _active[memento.contraction_memento.u] = false;
              _active[memento.contraction_memento.v] = false;
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

      std::unique_lock<Mutex> _batch_lock(_batches_mutex);
      std::unique_lock<Mutex> tmp_batch_lock(_tmp_batches_mutex);
      if ( _batches.empty() && _tmp_batches.size() > 0 ) {
        swapTmpBatches();
        _already_matched.reset();
        if ( _current_num_nodes == _num_nodes_before_pass ) {
          break;
        }
        _num_nodes_before_pass = _current_num_nodes;
      }
    }

    return result;
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
      lockBasedEnqueueBatch();
    }
  }

  void lockBasedEnqueueBatch() {
    std::unique_lock<std::shared_timed_mutex> lock(_batches_mutex);
    enqueueBatch();
  }

  void enqueueBatch() {
    std::vector<HypernodeID> new_batch;
    for ( size_t thread = 0; thread < _context.shared_memory.num_threads; ++thread ) {
      new_batch.insert(new_batch.end(), _batch[thread].begin(), _batch[thread].end());
      _batch[thread].clear();
    }
    if ( new_batch.size() > 0 ) {
      _tmp_batches.emplace(std::move(new_batch));
      _current_batch_size = 0;
    }
  }

  void swapTmpBatches() {
    enqueueBatch();
    ASSERT(_batches.size() == 0);
    std::swap(_batches, _tmp_batches);
    _tmp_batches = std::queue<std::vector<HypernodeID>>();
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

  void restoreParallelHyperedges() override {
    PartitionID thread_id = _history.back().thread_id;
    LockBasedHypergraphPruner& pruner = _pruner[thread_id];
    pruner.restoreParallelHyperedges(_hg, _history.back());
  }

  void restoreSingleNodeHyperedges() override {
    PartitionID thread_id = _history.back().thread_id;
    LockBasedHypergraphPruner& pruner = _pruner[thread_id];
    pruner.restoreSingleNodeHyperedges(_hg, _history.back());
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_context;
  using Base::_history;
  using Base::_max_hn_weights;
  
  const HypernodeWeight _weight_of_heaviest_node;

  std::vector<LockBasedHypergraphPruner> _pruner;

  std::atomic<HypernodeID> _contraction_index;
  std::atomic<HypernodeID> _current_num_nodes;
  HypernodeID _num_nodes_before_pass;
  MutexVector<HypernodeID> _hn_mutex;
  MutexVector<HyperedgeID> _he_mutex;

  std::atomic<HypernodeID> _current_batch_size;
  std::vector<std::vector<HypernodeID>> _batch;

  std::shared_timed_mutex _batches_mutex;
  std::queue<std::vector<HypernodeID>> _batches;
  std::shared_timed_mutex _tmp_batches_mutex;
  std::queue<std::vector<HypernodeID>> _tmp_batches;

  kahypar::ds::FastResetFlagArray<> _already_matched;
  std::vector<bool> _active;
  std::vector<HypernodeID> _last_touched;
};
}  // namespace kahypar
