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
#include <sched.h>
#include <numa.h>


#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/datastructure/lock_based_hypergraph.h"
#include "kahypar/datastructure/mutex_vector.h"
#include "kahypar/utils/thread_pool.h"
#include "kahypar/utils/timer.h"
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
#include "kahypar/partition/coarsening/hypergraph_pruner.h"

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
  using LockBasedHypergraph = kahypar::ds::LockBasedHypergraph<Hypergraph>;
  using LockBasedMemento = typename LockBasedHypergraph::LockBasedContractionMemento;
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
                             MutexVector<HyperedgeID>& he_mutex,
                             const std::vector<HypernodeID>& last_touched ) :
      thread_id(thread_id),
      history(),
      pruner(num_hypernodes, hn_mutex, he_mutex, last_touched),
      max_hn_weights(),
      start(std::chrono::high_resolution_clock::now()),
      end(),
      numa_node(-1),
      total_failed_contractions(0),
      total_tried_contractions(0),
      avg_failed_contractions_per_batch_in_percent(0.0),
      max_failed_contractions_per_batch_in_percent(0.0) { }
      

    PartitionID thread_id;
    History history;
    LockBasedHypergraphPruner pruner;
    std::vector<CurrentMaxNodeWeight> max_hn_weights;
    HighResClockTimepoint start;
    HighResClockTimepoint end;
    int numa_node;
    size_t total_failed_contractions;
    size_t total_tried_contractions;
    double avg_failed_contractions_per_batch_in_percent;
    double max_failed_contractions_per_batch_in_percent;
  };

 public:
  ParallelMLCoarsener(Hypergraph& hypergraph, const Context& context,
                      const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _weight_of_heaviest_node(weight_of_heaviest_node),
    _pruner(),
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
    HighResClockTimepoint global_start = std::chrono::high_resolution_clock::now();
    ThreadPool& pool = *_context.shared_memory.pool;
    LockBasedHypergraph lock_based_hypergraph(_hg, _hn_mutex, _he_mutex, _last_touched, _active);

    // Enqueue batches of hypernode in global queue
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    for ( const HypernodeID& hn : _hg.nodes() ) {
      addHypernodeToQueue(0, hn);
    }
    swapTmpBatches();
    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::hypergraph_preparation,
                          std::chrono::duration<double>(end - start).count());
    
    // Parallel Coarsening
    start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<ParallelCoarseningResult>> results;
    for ( size_t thread = 0; thread < _context.shared_memory.num_threads; ++thread ) {
      results.emplace_back(
        pool.enqueue([this, &lock_based_hypergraph, thread, limit]() {
          return parallelCoarsening(lock_based_hypergraph, thread, limit);
              }));
    }      
    pool.loop_until_empty();
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::parallel_coarsening,
                          std::chrono::duration<double>(end - start).count());

    // Create structures relevant for uncontractions
    start = std::chrono::high_resolution_clock::now();
    std::vector<ParallelCoarseningResult> coarsening_results;
    for ( std::future<ParallelCoarseningResult>& fut : results ) {
      coarsening_results.emplace_back(std::move(fut.get()));
    }
    createUncontractionHistory(coarsening_results, lock_based_hypergraph.numContractions());
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::merge_hierarchies,
                          std::chrono::duration<double>(end - start).count());

    for ( const ParallelCoarseningResult& result : coarsening_results ) {
      std::cout << "THREAD_RESULT graph=" << _context.partition.graph_filename.substr(
                   _context.partition.graph_filename.find_last_of('/') + 1)
                << " seed=" << _context.partition.seed
                << " num_threads=" << _context.shared_memory.num_threads
                << " thread=" << result.thread_id
                << " start=" << std::chrono::duration<double>(result.start - global_start).count()
                << " end=" << std::chrono::duration<double>(result.end - global_start).count()
                << " duration=" << std::chrono::duration<double>(result.end - result.start).count()
                << " numa_node=" << result.numa_node
                << " total_failed=" << result.total_failed_contractions
                << " total_tried=" << result.total_tried_contractions
                << " avg_failed=" << result.avg_failed_contractions_per_batch_in_percent
                << " max_failed=" << result.max_failed_contractions_per_batch_in_percent
                << std::endl;
    }

    // Compute current number of hypernodes, hyperedges and pins in parallel
    start = std::chrono::high_resolution_clock::now();
    lock_based_hypergraph.restoreHypergraphStats(pool);
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::undo_preparation,
                          std::chrono::duration<double>(end - start).count());
  }

  ParallelCoarseningResult parallelCoarsening(LockBasedHypergraph& hypergraph, 
                                              const PartitionID thread_id, 
                                              const HypernodeID limit) {
    ParallelCoarseningResult result(thread_id, _hg.initialNumNodes(), _hn_mutex, _he_mutex, _last_touched);
    Rater rater(_hg, _context, _hn_mutex, _he_mutex);
    result.numa_node = numa_node_of_cpu(sched_getcpu());
    result.max_hn_weights.emplace_back( 
      CurrentMaxNodeWeight { hypergraph.currentNumNodes(), _weight_of_heaviest_node } );

    size_t batch_pass = 0;
    while( hypergraph.currentNumNodes() > limit && !_batches.empty() ) {
      ++batch_pass;
      DBG << V(thread_id) << V(hypergraph.currentNumNodes()) 
          << V(_batches.size()) << V(limit);

      std::vector<HypernodeID> batch = popBatch();
      Randomize::instance().shuffleVector(batch, batch.size());

      size_t failed_contractions = 0;
      size_t tried_contractions = 0;
      for ( const HypernodeID& hn : batch ) {
        if ( _hg.nodeIsEnabled(hn) ) {
          Rating rating = rater.rate(hn, _already_matched);
          
          if ( rating.target != kInvalidTarget ) {
            ++tried_contractions;
            CoarseningMemento memento(thread_id, hypergraph.contract(hn, rating.target));
            
            // In case we are not able to contract two nodes, the contract function
            // returns {0, 0}. This happens, if e.g. another threads contracts one
            // of the two nodes or a lock for a hyperedge was hold by an other thread.
            if ( memento.contraction_memento.u == hn && 
                 memento.contraction_memento.v == rating.target ) {
              _already_matched.set(memento.contraction_memento.u, true);
              _already_matched.set(memento.contraction_memento.v, true);
              result.pruner.removeSingleNodeHyperedges(hypergraph, memento);
              result.pruner.removeParallelHyperedges(hypergraph, memento);
              result.history.emplace_back(std::move(memento));
              if ( result.max_hn_weights.back().max_weight < _hg.nodeWeight(hn) ) {
                result.max_hn_weights.emplace_back( 
                  CurrentMaxNodeWeight {memento.contraction_index, _hg.nodeWeight(hn)} );
              }

              // Hypernodes are marked by the hypergraph as active, if they are
              // part of a contractions. Once they are active, they are not allowed
              // to participate in an other contraction. Unmark the active state here
              // releases the two hypernodes and u can be now part of an other 
              // contraction.
              hypergraph.unmarkAsActive(memento.contraction_memento.u);
              hypergraph.unmarkAsActive(memento.contraction_memento.v);
            } else {
              ++failed_contractions;
            }
          }

          if ( _hg.nodeIsEnabled(hn) ) {
            addHypernodeToQueue(thread_id, hn);
          }

          if ( hypergraph.currentNumNodes() <= limit ) {
            break;
          }
        }
      }

      double failed_contractions_in_percentage = ((double) failed_contractions) / tried_contractions;
      result.max_failed_contractions_per_batch_in_percent =
        std::max(result.max_failed_contractions_per_batch_in_percent, failed_contractions_in_percentage);
      result.total_failed_contractions += failed_contractions;
      result.total_tried_contractions += tried_contractions;

      // In case batch queue is empty, we swap content from temporary
      // batch queue into the global batch queue. This is only allowed
      // to be executed by one thread, therefore we introduce this
      // synchronization barrier.
      std::unique_lock<Mutex> _batch_lock(_batches_mutex);
      std::unique_lock<Mutex> tmp_batch_lock(_tmp_batches_mutex);
      if ( _batches.empty() && _tmp_batches.size() > 0 ) {
        swapTmpBatches();
        _already_matched.reset();
        if ( hypergraph.currentNumNodes() == _num_nodes_before_pass ) {
          break;
        }
        _num_nodes_before_pass = hypergraph.currentNumNodes();
      }
    }

    result.avg_failed_contractions_per_batch_in_percent = 
      ( (double) result.total_failed_contractions ) / result.total_tried_contractions ;
    result.end = std::chrono::high_resolution_clock::now();
    return result;
  }

  void createUncontractionHistory(std::vector<ParallelCoarseningResult>& results,
                                  const HypernodeID number_of_contractions) {
    _history.resize(number_of_contractions);
    for ( ParallelCoarseningResult& result : results  ) {
      _pruner.emplace_back(std::move(result.pruner));
      for ( CoarseningMemento& memento : result.history ) {
        HypernodeID contraction_index = memento.contraction_index;
        _history[contraction_index] = std::move(memento);
      }

      // Compute Maximum Hypernode Weight during contraction hierarchy
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

  HypernodeID _num_nodes_before_pass;

  // Vectors to lock hypernodes an hyperedges
  MutexVector<HypernodeID> _hn_mutex;
  MutexVector<HyperedgeID> _he_mutex;

  // Each thread contains its own batch queues
  // Once the batch size threshold is reached all
  // hypernodes from all batch queues are inserted 
  // into _tmp_batches
  std::atomic<HypernodeID> _current_batch_size;
  std::vector<std::vector<HypernodeID>> _batch;

  // Batch Queue - Contains nodes for current contraction round
  std::shared_timed_mutex _batches_mutex;
  std::queue<std::vector<HypernodeID>> _batches;

  // Batch Queue - Contains nodes for next contraction round
  std::shared_timed_mutex _tmp_batches_mutex;
  std::queue<std::vector<HypernodeID>> _tmp_batches;

  kahypar::ds::FastResetFlagArray<> _already_matched;
  std::vector<bool> _active;
  std::vector<HypernodeID> _last_touched;
};
}  // namespace kahypar
