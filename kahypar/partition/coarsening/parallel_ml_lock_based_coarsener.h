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
#include "kahypar/datastructure/concurrent_batch_queue.h"
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

/**
 * TODOS:
 *  - Prepare Mutex Vector for Conflicts
 *  - Refactor Lock Based Vertex Pair Rater
 *  - Refactor Probabilistic Sparse Map
 */


template <class ScorePolicy = HeavyEdgeScore,
          class HeavyNodePenaltyPolicy = NoWeightPenalty,
          class CommunityPolicy = UseCommunityStructure,
          class RatingPartitionPolicy = NormalPartitionPolicy,
          class AcceptancePolicy = BestRatingPreferringUnmatched<>,
          class FixedVertexPolicy = AllowFreeOnFixedFreeOnFreeFixedOnFixed,
          typename RatingType = RatingType>
class ParallelMLLockBasedCoarsener final : public ICoarsener,
                                  protected VertexPairCoarsenerBase<>{
 private:
  static constexpr bool debug = false;
  static constexpr size_t BATCH_SIZE = 1000;
  static constexpr double REDUCTION_THRESHOLD = 1.05;
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
  using ConcurrentBatchQueue = kahypar::ds::ConcurrentBatchQueue<HypernodeID>;
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
  ParallelMLLockBasedCoarsener(Hypergraph& hypergraph, const Context& context,
                      const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _weight_of_heaviest_node(weight_of_heaviest_node),
    _pruner(),
    _sync(),
    _global_round(1),
    _hn_mutex(_hg.initialNumNodes()),
    _he_mutex(_hg.initialNumEdges()),
    _queue(context.shared_memory.num_threads, BATCH_SIZE),
    _already_matched(_hg.initialNumNodes()),
    _active(_hg.initialNumNodes()),
    _last_touched(_hg.initialNumEdges(), 0) { }

  ~ParallelMLLockBasedCoarsener() override = default;

  ParallelMLLockBasedCoarsener(const ParallelMLLockBasedCoarsener&) = delete;
  ParallelMLLockBasedCoarsener& operator= (const ParallelMLLockBasedCoarsener&) = delete;

  ParallelMLLockBasedCoarsener(ParallelMLLockBasedCoarsener&&) = delete;
  ParallelMLLockBasedCoarsener& operator= (ParallelMLLockBasedCoarsener&&) = delete;

 private:
  void coarsenImpl(const HypernodeID limit) override final {
    ASSERT(_context.shared_memory.pool, "Thread pool not initialized");
    HighResClockTimepoint global_start = std::chrono::high_resolution_clock::now();
    ThreadPool& pool = *_context.shared_memory.pool;
    LockBasedHypergraph lock_based_hypergraph(_hg, _hn_mutex, _he_mutex, _last_touched, _active);

    // Enqueue batches of hypernode in global queue
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    std::vector<HypernodeID> hypernodes;
    for ( const HypernodeID& hn : _hg.nodes() ) {
      hypernodes.push_back(hn);
    }
    Randomize::instance().shuffleVector(hypernodes, hypernodes.size());
    for ( const HypernodeID& hn : hypernodes ) {
      _queue.push(0, hn);
    }
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

    size_t round = 1;
    size_t num_nodes_before_pass = hypergraph.currentNumNodes();
    while( hypergraph.currentNumNodes() > limit && !_queue.empty() ) {
      DBG << V(thread_id) << V(hypergraph.currentNumNodes()) 
          << V(_queue.unsafe_size()) << V(round) << V(limit);

      std::vector<HypernodeID> batch = _queue.pop_batch();
      Randomize::instance().shuffleVector(batch, batch.size());

      size_t failed_contractions = 0;
      size_t tried_contractions = 0;
      for ( const HypernodeID& hn : batch ) {
        if ( _hg.nodeIsEnabled(hn) ) {
          Rating rating = rater.rateLockBased(hn, _already_matched);
          
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
            _queue.push(thread_id, hn);
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

      if ( round < _queue.round() ) {
        if ( REDUCTION_THRESHOLD * hypergraph.currentNumNodes() >= num_nodes_before_pass ) {
          break;
        }
        round = _queue.round();
        num_nodes_before_pass = hypergraph.currentNumNodes();
        std::unique_lock<Mutex> sync_lock(_sync);
        if ( _global_round < _queue.round() ) {
          _global_round = _queue.round();
          _already_matched.reset();
        }
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

  Mutex _sync;
  size_t _global_round = 0;

  // Vectors to lock hypernodes an hyperedges
  MutexVector<HypernodeID> _hn_mutex;
  MutexVector<HyperedgeID> _he_mutex;

  // Queue contains batches of hypernodes for coarsening
  ConcurrentBatchQueue _queue;

  kahypar::ds::FastResetFlagArray<> _already_matched;
  std::vector<bool> _active;
  std::vector<HypernodeID> _last_touched;
};
}  // namespace kahypar
