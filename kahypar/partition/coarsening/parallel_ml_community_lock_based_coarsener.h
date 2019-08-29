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

#include <limits>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <condition_variable>
#include <sched.h>
#include <numa.h>


#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/datastructure/community_hypergraph.h"
#include "kahypar/datastructure/lock_based_hypergraph.h"
#include "kahypar/datastructure/concurrent_batch_queue.h"
#include "kahypar/datastructure/mutex_vector.h"
#include "kahypar/utils/timer.h"
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
#include "kahypar/partition/coarsening/hypergraph_pruner.h"

namespace kahypar {

template <class ScorePolicy = HeavyEdgeScore,
          class HeavyNodePenaltyPolicy = NoWeightPenalty,
          class CommunityPolicy = UseCommunityStructure,
          class RatingPartitionPolicy = NormalPartitionPolicy,
          class AcceptancePolicy = BestRatingPreferringUnmatched<>,
          class FixedVertexPolicy = AllowFreeOnFixedFreeOnFreeFixedOnFixed,
          typename RatingType = RatingType>
class ParallelMLCommunityLockBasedCoarsener final : public ICoarsener,
                                           protected VertexPairCoarsenerBase<>{
 private:
  static constexpr bool debug = false;
  static constexpr HypernodeID kInvalidTarget = std::numeric_limits<HypernodeID>::max();
  static constexpr HypernodeID BASE_CASE_NODE_LIMIT = 5000;
  static constexpr HypernodeID BATCH_SIZE_FACTOR = 50;

  using Base = VertexPairCoarsenerBase;
  using CommunityHypergraph = kahypar::ds::CommunityHypergraph<Hypergraph>;
  using LockBasedCommunityHypergraph = kahypar::ds::LockBasedHypergraph<CommunityHypergraph>;
  using LockBasedMemento = typename LockBasedCommunityHypergraph::LockBasedContractionMemento;
  using Memento = typename Hypergraph::ContractionMemento;

  using Rater = LockBasedVertexPairRater<ScorePolicy,
                                        HeavyNodePenaltyPolicy,
                                        CommunityPolicy,
                                        RatingPartitionPolicy,
                                        AcceptancePolicy,
                                        FixedVertexPolicy,
                                        RatingType,
                                        LockBasedCommunityHypergraph>;
  using Rating = typename Rater::Rating;

  using ThreadPool = kahypar::parallel::ThreadPool;
  using ConcurrentBatchQueue = kahypar::ds::ConcurrentBatchQueue<HypernodeID>;
  using Mutex = std::shared_timed_mutex;
  template<typename Key>
  using MutexVector = kahypar::ds::MutexVector<Key, Mutex>;
  using History = std::vector<CoarseningMemento>;
  using Hierarchy = std::vector<Memento>;

  struct ParallelCoarseningResult {

    ParallelCoarseningResult(const PartitionID thread_id) :
      thread_id(thread_id),
      history() { }
      
    PartitionID thread_id;
    History history;
  };

  enum class CommunityState : uint8_t {
    WAITING = 0,
    SINGLE_THREADED = 1,
    MULTI_THREADED = 2,
    COMPLETED = 3
  };

  enum class ThreadState : uint8_t {
    IDLE = 0,
    MAIN = 1,
    JOINED = 2
  };

  class WorkStealing {
    private:
      struct CommunityData {
        explicit CommunityData() :
          mutex(),
          cv(),
          finished_contractions(false),
          state(CommunityState::WAITING),
          threads_in_community(0),
          contraction_limit(0),
          round(1) { }

        Mutex mutex;
        std::condition_variable_any cv;
        bool finished_contractions;
        CommunityState state;
        size_t threads_in_community;
        HypernodeID contraction_limit;
        size_t round;
      };

      struct ThreadData {
        explicit ThreadData() :
          state(ThreadState::IDLE),
          current_community(-1),
          tmp_contractions() { }

        ThreadState state;
        PartitionID current_community;
        std::vector<Memento> tmp_contractions;
      };

    public:
      WorkStealing(const PartitionID num_communities, const size_t num_threads) :
        _num_communities(num_communities),
        _num_threads(num_threads),
        _assign_mutex(),
        _community_data(num_communities),
        _thread_data(num_threads),
        _community_queues(),
        _running(),
        _waiting() { 
        for ( PartitionID i = 0; i < num_communities; ++i ) {
          _community_queues.emplace_back(num_threads, 5000);
        }
      }

      WorkStealing(const WorkStealing&) = delete;
      WorkStealing& operator= (const WorkStealing&) = delete;

      WorkStealing(WorkStealing&&) = delete;
      WorkStealing& operator= (WorkStealing&&) = delete;

      PartitionID assignWork(const CommunityHypergraph& community_hypergraph,
                             const size_t thread_id) {        
        std::unique_lock<Mutex> assign_lock(_assign_mutex);
        ASSERT(thread(thread_id).current_community == -1, "Thread" << thread_id << "is already assigned to community"
          << thread(thread_id).current_community);

        PartitionID assigned_community = -1;
        CommunityState state = CommunityState::COMPLETED;
        if ( !_waiting.empty() ) {
          assigned_community = _waiting.front();
          state = community(assigned_community).state;
          _waiting.pop();
        } else if ( _running.size() > 0 ) {
          size_t min_num_threads_in_community = std::numeric_limits<size_t>::max();
          HypernodeID max_num_nodes = 0;
          for ( const PartitionID& community_id : _running ) {
            HypernodeID community_num_nodes = community_hypergraph.currentCommunityNumNodes(community_id);
            if (  community_num_nodes >= BASE_CASE_NODE_LIMIT &&
                 _community_queues[community_id].unsafe_size() >= 5000 ) {
              size_t num_threads_in_community = community(community_id).threads_in_community;
              bool has_more_nodes = ( 1.1 * max_num_nodes < community_num_nodes );
              bool has_less_threads = ( 1.1 * community_num_nodes > max_num_nodes && num_threads_in_community < min_num_threads_in_community );
              if ( has_more_nodes || has_less_threads ) {
                min_num_threads_in_community = num_threads_in_community;
                max_num_nodes = community_num_nodes;
                assigned_community = community_id;
                state = community(community_id).state;
              }
            }
          }
        }

        if ( assigned_community != -1 ) {
          ASSERT(community(assigned_community).state != CommunityState::COMPLETED);
          if ( state == CommunityState::WAITING ) {
            community(assigned_community).state = CommunityState::SINGLE_THREADED;
            thread(thread_id).state = ThreadState::MAIN;
            _running.push_back(assigned_community);
          } else if ( state == CommunityState::SINGLE_THREADED ) {
            community(assigned_community).state = CommunityState::MULTI_THREADED;
            thread(thread_id).state = ThreadState::JOINED;
          } else {
            thread(thread_id).state = ThreadState::JOINED;
          }
          thread(thread_id).current_community = assigned_community;
          ++community(assigned_community).threads_in_community;
        }

        return assigned_community;
      }

      void completeWork(const CommunityHypergraph& community_hypergraph,
                        const size_t thread_id, 
                        const PartitionID community_id) {
        ASSERT(community(community_id).threads_in_community > 0);
        std::unique_lock<Mutex> assign_lock(_assign_mutex);
        if ( thread(thread_id).state == ThreadState::MAIN ) {
          DBG << "Main thread completed community" << community_id;
          community(community_id).state = CommunityState::COMPLETED;
          size_t community_pos = std::numeric_limits<size_t>::max();
          for ( size_t pos = 0; pos < _running.size(); ++pos) {
            if ( _running[pos] == community_id ) {
              community_pos = pos;
              break;
            }
          }
          ASSERT(community_pos != std::numeric_limits<size_t>::max());
          std::swap(_running[community_pos], _running.back());
          _running.pop_back();
        }
        thread(thread_id).state = ThreadState::IDLE;
        thread(thread_id).current_community = -1;
        --community(community_id).threads_in_community;
        if ( community(community_id).threads_in_community == 1 &&
             community_hypergraph.currentCommunityNumNodes(community_id) < BASE_CASE_NODE_LIMIT ) {
          community(community_id).state = CommunityState::SINGLE_THREADED;
        }
      }

      bool eligibleReturnToSingleThreaded(CommunityHypergraph& community_hypergraph,
                                          const size_t thread_id, 
                                          const PartitionID community_id) {
        std::unique_lock<Mutex> assign_lock(_assign_mutex);
        if ( thread(thread_id).state == ThreadState::MAIN &&
             community(community_id).threads_in_community == 1 &&
             community_hypergraph.currentCommunityNumNodes(community_id) < BASE_CASE_NODE_LIMIT ) {
          community(community_id).state = CommunityState::SINGLE_THREADED;
          return true;
        } else {
          return false;
        }
      }

      void initializeCommunitySizesAndContractionLimits(CommunityHypergraph& community_hypergraph,
                                                        const HypernodeID limit) {
        std::vector<HypernodeID> community_sizes(_num_communities, 0);    std::vector<HypernodeID> hypernodes;
        for ( const HypernodeID& hn : community_hypergraph.nodes() ) {
          hypernodes.push_back(hn);
          ++community_sizes[community_hypergraph.communityID(hn)];
        }
        Randomize::instance().shuffleVector(hypernodes, hypernodes.size());

        std::vector<PartitionID> communities;
        for ( PartitionID community_id = 0; community_id < _num_communities; ++community_id ) {
          communities.push_back(community_id);
          HypernodeID community_num_nodes = community_sizes[community_id];
          community_hypergraph.setCommunityNumNodes(community_id, community_num_nodes);
          community(community_id).contraction_limit = 
            std::ceil((((double) community_num_nodes) / community_hypergraph.totalWeight()) * limit);
          updateQueueBatchSize(community_hypergraph, community_id);
        }

        for ( const HypernodeID& hn : hypernodes ) {
          push(community_hypergraph.communityID(hn), 0, hn);
        }

        std::sort(communities.begin(), communities.end(), 
          [&](const PartitionID& lhs, const PartitionID& rhs) {
            return community_hypergraph.currentCommunityNumNodes(lhs) > community_hypergraph.currentCommunityNumNodes(rhs);
        });
        for ( const PartitionID community_id : communities ) {
          _waiting.push(community_id);
        }
      }

      void updateQueueBatchSize(CommunityHypergraph& community_hypergraph, const PartitionID community_id) {
        ASSERT(community_id < _num_communities);
        HypernodeID community_num_nodes = community_hypergraph.currentCommunityNumNodes(community_id);
        _community_queues[community_id].update_batch_size(std::max( community_num_nodes / BATCH_SIZE_FACTOR , 100U ));
      }

      Mutex& communityMutex(const PartitionID community_id) {
        return community(community_id).mutex;
      }


      std::condition_variable_any& communityConditionVariable(const PartitionID community_id) {
        return community(community_id).cv;
      }

      bool& communityFinishedContractions(const PartitionID community_id) {
        return community(community_id).finished_contractions;
      }

      CommunityState communityState(const PartitionID community_id) {
        return community(community_id).state;
      }

      std::vector<size_t> allThreadsAssignedToCommunity(const PartitionID community_id) {
        std::vector<size_t> threads;
        for ( size_t thread_id = 0; thread_id < _thread_data.size(); ++thread_id ) {
          if ( thread(thread_id).current_community == community_id ) {
            threads.push_back(thread_id);
          }
        }
        return threads;
      }

      ThreadState threadState(const size_t thread_id) {
        return thread(thread_id).state;
      }

      std::vector<Memento> & contractions(const size_t thread_id) {
        return thread(thread_id).tmp_contractions;
      }

      void push( const PartitionID community_id, const size_t thread_id, const HypernodeID hn ) {
        ASSERT(community_id < _num_communities);
        ASSERT(thread_id < _num_threads);
        _community_queues[community_id].push(thread_id, hn);
      }

      std::vector<HypernodeID> pop_batch(const PartitionID community_id) {
        ASSERT(community_id < _num_communities);
        return _community_queues[community_id].pop_batch();
      }

      bool empty(const PartitionID community_id) const {
        ASSERT(community_id < _num_communities);
        return _community_queues[community_id].empty();
      }

      size_t round(const PartitionID community_id) const {
        ASSERT(community_id < _num_communities);
        return _community_queues[community_id].round();
      }

      size_t communityRound(const PartitionID community_id) {
        return community(community_id).round;
      }

      void updateRound(const PartitionID community_id) {
        community(community_id).round = round(community_id);
      }

      size_t unsafe_size(const PartitionID community_id) const {
        ASSERT(community_id < _num_communities);
        return _community_queues[community_id].unsafe_size();
      }

      HypernodeID contractionLimit(const PartitionID community_id) {
        return community(community_id).contraction_limit;
      }


    private:
      ThreadData& thread(const size_t thread_id) {
        ASSERT(thread_id < _num_threads);
        return _thread_data[thread_id];
      }

      CommunityData& community(const PartitionID community_id) {
        ASSERT(community_id < _num_communities);
        return _community_data[community_id];
      }

      const PartitionID _num_communities;
      const size_t _num_threads;
      Mutex _assign_mutex;
      std::vector<CommunityData> _community_data;
      std::vector<ThreadData> _thread_data;
      std::vector<ConcurrentBatchQueue> _community_queues;
      std::vector<PartitionID> _running;
      std::queue<PartitionID> _waiting;
  };

 public:
  ParallelMLCommunityLockBasedCoarsener(Hypergraph& hypergraph, const Context& context,
              const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _weight_of_heaviest_node(weight_of_heaviest_node),
    _work_stealing(hypergraph.numCommunities(), context.shared_memory.num_threads),
    _pruner(),
    _hn_mutex(_hg.initialNumNodes()),
    _he_mutex(_hg.initialNumEdges()),
    _already_matched(),
    _active(_hg.initialNumNodes()),
    _last_touched(_hg.initialNumEdges(), 0) { 
    
    for ( size_t i = 0; i < context.shared_memory.num_threads; ++i ) {
      _pruner.emplace_back(_hg.initialNumNodes(), _hn_mutex, _he_mutex, _last_touched);
    }

  }

  ~ParallelMLCommunityLockBasedCoarsener() override = default;

  ParallelMLCommunityLockBasedCoarsener(const ParallelMLCommunityLockBasedCoarsener&) = delete;
  ParallelMLCommunityLockBasedCoarsener& operator= (const ParallelMLCommunityLockBasedCoarsener&) = delete;

  ParallelMLCommunityLockBasedCoarsener(ParallelMLCommunityLockBasedCoarsener&&) = delete;
  ParallelMLCommunityLockBasedCoarsener& operator= (ParallelMLCommunityLockBasedCoarsener&&) = delete;

 private:
  void coarsenImpl(const HypernodeID limit) override final {
    ASSERT(_context.shared_memory.pool, "Thread pool not initialized");
    //HighResClockTimepoint global_start = std::chrono::high_resolution_clock::now();
    ThreadPool& pool = *_context.shared_memory.pool;
    CommunityHypergraph community_hypergraph(_hg, _context, pool);
    LockBasedCommunityHypergraph lock_based_hypergraph(community_hypergraph, 
      _hn_mutex, _he_mutex, _last_touched, _active, true);

    // Setup internal structures in hypergraph such that
    // parallel contractions are possible
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    community_hypergraph.initialize();
    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::hypergraph_preparation,
                          std::chrono::duration<double>(end - start).count());

    
    start = std::chrono::high_resolution_clock::now();
    _work_stealing.initializeCommunitySizesAndContractionLimits(community_hypergraph, limit);
    for ( PartitionID community = 0; community < _hg.numCommunities(); ++community ) {
      _already_matched.emplace_back(_work_stealing.unsafe_size(community));
    }
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::compute_community_sizes,
                          std::chrono::duration<double>(end - start).count());


    // Parallel Coarsening
    start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<ParallelCoarseningResult>> results;
    for ( size_t thread = 0; thread < _context.shared_memory.num_threads; ++thread ) {
      results.emplace_back(
        pool.enqueue([this, &community_hypergraph, &lock_based_hypergraph, thread, limit]() {
          return parallelCoarsening(community_hypergraph, lock_based_hypergraph, thread);
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
    Hierarchy hierarchy = createUncontractionHistory(coarsening_results, lock_based_hypergraph.numContractions());
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::merge_hierarchies,
                          std::chrono::duration<double>(end - start).count());

    // Undo changes on internal hypergraph data structure done by
    // prepareForParallelCommunityAwareCoarsening(...) such that
    // hypergraph can be again used in a sequential setting
    start = std::chrono::high_resolution_clock::now();
    community_hypergraph.undo(hierarchy);
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::undo_preparation,
                          std::chrono::duration<double>(end - start).count());
  }

  ParallelCoarseningResult parallelCoarsening(CommunityHypergraph& community_hypergraph,
                                              LockBasedCommunityHypergraph& lock_based_hypergraph,
                                              const size_t thread_id) {
    ParallelCoarseningResult result(thread_id);
    Rater rater(lock_based_hypergraph, _context, _hn_mutex, _he_mutex);
    PartitionID community_id = _work_stealing.assignWork(community_hypergraph, thread_id);
    ds::FastResetFlagArray<> tmp_disabled_hypernodes(community_hypergraph.initialNumNodes());
    while ( community_id != -1 ) {

      switch ( _work_stealing.communityState(community_id) ) {
        case CommunityState::SINGLE_THREADED:
          singleThreadedCommunityCoarsening(community_hypergraph, lock_based_hypergraph,
            result, rater, community_id, thread_id, _work_stealing.contractionLimit(community_id), tmp_disabled_hypernodes);
          break;
        case CommunityState::MULTI_THREADED:
          multiThreadedCommunityCoarsening(community_hypergraph, lock_based_hypergraph, result, rater,
            community_id, thread_id, _work_stealing.contractionLimit(community_id), tmp_disabled_hypernodes);
          break;
        default:
          break;
      }

      _work_stealing.completeWork(community_hypergraph, thread_id, community_id);
      community_id = _work_stealing.assignWork(community_hypergraph, thread_id);
    }
    return result;
  }

  void singleThreadedCommunityCoarsening(CommunityHypergraph& community_hypergraph,
                                         LockBasedCommunityHypergraph& lock_based_hypergraph,
                                         ParallelCoarseningResult& result,
                                         Rater& rater,
                                         const PartitionID community_id,
                                         const size_t thread_id,
                                         const HypernodeID limit,
                                         ds::FastResetFlagArray<>& tmp_disabled_hypernodes) {
    std::unique_lock<Mutex> community_lock(_work_stealing.communityMutex(community_id), std::defer_lock);
    if ( !community_lock.try_lock() ) {
      ASSERT(_work_stealing.communityState(community_id) == CommunityState::MULTI_THREADED);
      multiThreadedCommunityCoarsening(community_hypergraph, lock_based_hypergraph, 
        result, rater, community_id, thread_id, limit, tmp_disabled_hypernodes);
      return;
    }

    LOG << "Thread" << thread_id << "process community" << community_id 
        << "in single-threaded mode (Contraction Limit =" << limit << ", Thread State ="
        << threadState(_work_stealing.threadState(thread_id)) << ", Community State ="
        << communityState(_work_stealing.communityState(community_id)) << ")";

    _pruner[thread_id].setCommunity(community_id);
    rater.setCommunity(community_id);
    size_t round = 1;
    size_t num_contractions = 0;
    while ( community_hypergraph.currentCommunityNumNodes(community_id) > limit && !_work_stealing.empty(community_id) ) {

      std::vector<HypernodeID> batch = _work_stealing.pop_batch(community_id);
      Randomize::instance().shuffleVector(batch, batch.size());

      size_t batch_pos = 0;
      for ( const HypernodeID& hn : batch ) {
        ++batch_pos;
        if ( community_hypergraph.nodeIsEnabled(hn) ) {
          Rating rating = rater.rateSingleThreaded(hn, _already_matched[community_id]);
          HypernodeID target = rating.target;

          if ( target != std::numeric_limits<HypernodeID>::max() ) {
            HypernodeID community_target = community_hypergraph.communityNodeID(target);
            LockBasedMemento lock_based_memento(
              community_hypergraph.contract(hn, target), 
              lock_based_hypergraph.drawContractionId());
            result.history.emplace_back(community_id, thread_id, std::move(lock_based_memento)); 
            _already_matched[community_id].set(community_hypergraph.communityNodeID(hn), true);
            _already_matched[community_id].set(community_target, true);
            _pruner[thread_id].removeSingleNodeHyperedges(community_hypergraph, result.history.back());
            _pruner[thread_id].removeParallelHyperedges(community_hypergraph, result.history.back());
            num_contractions++;
          }

          ASSERT(community_hypergraph.nodeIsEnabled(hn));
          if ( community_hypergraph.nodeDegree(hn) > 0 ) {
            _work_stealing.push(community_id, thread_id, hn);
          }
        }

        if ( community_hypergraph.currentCommunityNumNodes(community_id) <= limit ) {
          break;
        }

        if ( _work_stealing.communityState(community_id) == CommunityState::MULTI_THREADED ) {
          for ( size_t i = batch_pos; i < batch.size(); ++i ) {
            HypernodeID hn = batch[i];
            if ( community_hypergraph.nodeIsEnabled(hn) ) {
              _work_stealing.push(community_id, thread_id, hn);
            }
          }
          community_lock.unlock();
          multiThreadedCommunityCoarsening(community_hypergraph, lock_based_hypergraph, 
            result, rater, community_id, thread_id, limit, tmp_disabled_hypernodes);
          return;
        }
      }


      if ( community_hypergraph.currentCommunityNumNodes(community_id) <= limit ) {
        break;
      }

      if ( round < _work_stealing.round(community_id) ) {
        if ( num_contractions == 0 ) {
          break;
        }

        _work_stealing.updateQueueBatchSize(community_hypergraph, community_id);
        round = _work_stealing.round(community_id);
        _work_stealing.updateRound(community_id);
        num_contractions = 0;
        _already_matched[community_id].reset();
      }
    }
  }

  void multiThreadedCommunityCoarsening(CommunityHypergraph& community_hypergraph,
                                        LockBasedCommunityHypergraph& lock_based_hypergraph,
                                        ParallelCoarseningResult& result,
                                        Rater& rater,
                                        const PartitionID community_id,
                                        const size_t thread_id,
                                        const HypernodeID limit,
                                        ds::FastResetFlagArray<>& tmp_disabled_hypernodes) {
    std::shared_lock<Mutex> community_lock(_work_stealing.communityMutex(community_id));
    LOG << "Thread" << thread_id << "process community" << community_id 
        << "in multi-threaded mode (Contraction Limit =" << limit << ", Thread State ="
        << threadState(_work_stealing.threadState(thread_id)) << ", Community State ="
        << communityState(_work_stealing.communityState(community_id)) << ", Num Nodes =" 
        << community_hypergraph.currentCommunityNumNodes(community_id) << ")";

    size_t round = _work_stealing.round(community_id);
    size_t num_contractions = 0;
    size_t tried_contractions = 0;
    _pruner[thread_id].setCommunity(community_id);
    rater.setCommunity(community_id);
    while ( lock_based_hypergraph.currentCommunityNumNodes(community_id) > limit && !_work_stealing.empty(community_id) ) {

      std::vector<HypernodeID> batch = _work_stealing.pop_batch(community_id);
      Randomize::instance().shuffleVector(batch, batch.size());

      std::vector<Memento>& contractions = _work_stealing.contractions(thread_id);
      ASSERT(contractions.empty(), "Contractions of thread" << thread_id << "are not empty");
      for ( const HypernodeID& hn : batch ) {
        if ( community_hypergraph.nodeIsEnabled(hn) &&
             !tmp_disabled_hypernodes[hn] ) {
          Rating rating = rater.rateSingleThreaded(hn, _already_matched[community_id]);
        
          if ( rating.target != std::numeric_limits<HypernodeID>::max() ) {
            ASSERT(community_hypergraph.nodeIsEnabled(rating.target), "Hypernode" << rating.target << "is disabled");
            contractions.emplace_back(Memento {hn, rating.target});
            tmp_disabled_hypernodes.set(rating.target, true);
          }
        }
      }
      tmp_disabled_hypernodes.reset();

      _work_stealing.communityFinishedContractions(community_id) = false;
      if ( _work_stealing.threadState(thread_id) == ThreadState::MAIN ) {
        community_lock.unlock();
        std::unique_lock<Mutex> lock(_work_stealing.communityMutex(community_id));
        std::vector<size_t> threads = _work_stealing.allThreadsAssignedToCommunity(community_id);
        for ( const size_t thread : threads ) {
          std::vector<Memento>& thread_contractions = _work_stealing.contractions(thread);
          while ( !thread_contractions.empty() ) {
            HypernodeID representative = thread_contractions.back().u;
            HypernodeID target = thread_contractions.back().v;
            thread_contractions.pop_back();
            ++tried_contractions;
            if ( community_hypergraph.nodeIsEnabled(representative) &&
                 community_hypergraph.nodeIsEnabled(target) ) {
              HypernodeID community_target = community_hypergraph.communityNodeID(target);
              LockBasedMemento lock_based_memento(
                community_hypergraph.contract(representative, target), 
                lock_based_hypergraph.drawContractionId());
              result.history.emplace_back(community_id, thread_id, std::move(lock_based_memento)); 
              _already_matched[community_id].set(community_hypergraph.communityNodeID(representative), true);
              _already_matched[community_id].set(community_target, true);
              _pruner[thread_id].removeSingleNodeHyperedges(community_hypergraph, result.history.back());
              _pruner[thread_id].removeParallelHyperedges(community_hypergraph, result.history.back());
              num_contractions++;
            }

            if ( community_hypergraph.currentCommunityNumNodes(community_id) <= limit ) {
              break;
            }

            if ( community_hypergraph.nodeIsEnabled(representative) ) {
              _work_stealing.push(community_id, thread_id, representative);
            }
          }
        }
        
        lock.unlock();
        community_lock.lock();
        _work_stealing.communityFinishedContractions(community_id) = true;
        _work_stealing.communityConditionVariable(community_id).notify_all();
      } else if ( !_work_stealing.communityFinishedContractions(community_id) ) {
        _work_stealing.communityConditionVariable(community_id).wait(community_lock, 
          [this, &community_id]() { return this->_work_stealing.communityFinishedContractions(community_id); });
      }

      if ( lock_based_hypergraph.currentCommunityNumNodes(community_id) <= limit ) {
        break;
      }

      if ( round < _work_stealing.round(community_id) ) {
        if ( num_contractions == 0 ||
             _work_stealing.threadState(thread_id) != ThreadState::MAIN ) {
          break;
        }

        if ( _work_stealing.eligibleReturnToSingleThreaded(community_hypergraph, thread_id, community_id) ) {
          community_lock.unlock();
          singleThreadedCommunityCoarsening(community_hypergraph, lock_based_hypergraph, 
            result, rater, community_id, thread_id, limit, tmp_disabled_hypernodes);
          return;
        }

        _work_stealing.updateQueueBatchSize(community_hypergraph, community_id);
        round = _work_stealing.round(community_id);
        num_contractions = 0;
        tried_contractions = 0;
        if ( _work_stealing.communityRound(community_id) < _work_stealing.round(community_id) ) {
          _work_stealing.updateRound(community_id);
          _already_matched[community_id].reset();
        }
      }
    }
  }

  std::vector<Memento> createUncontractionHistory(std::vector<ParallelCoarseningResult>& results,
                                                  const HypernodeID number_of_contractions) {
    std::vector<Memento> hierarchy(number_of_contractions);
    _history.resize(number_of_contractions);
    for ( ParallelCoarseningResult& result : results  ) {
      for ( CoarseningMemento& memento : result.history ) {
        HypernodeID contraction_index = memento.contraction_index;
        ASSERT(contraction_index < number_of_contractions);
        _history[contraction_index] = std::move(memento);
        hierarchy[contraction_index] = _history[contraction_index].contraction_memento;
      }
      // TODO: Compute Maximum Hypernode Weight during contraction hierarchy
    }
    return hierarchy;
  }
  

  bool uncoarsenImpl(IRefiner& refiner) override final {
    return doUncoarsen(refiner);
  }

  void restoreParallelHyperedges() override {
    PartitionID thread_id = _history.back().thread_id;
    CommunityLockBasedHypergraphPruner& pruner = _pruner[thread_id];
    pruner.restoreParallelHyperedges(_hg, _history.back());
  }

  void restoreSingleNodeHyperedges() override {
    PartitionID thread_id = _history.back().thread_id;
    CommunityLockBasedHypergraphPruner& pruner = _pruner[thread_id];
    pruner.restoreSingleNodeHyperedges(_hg, _history.back());
  }

  std::string threadState(const ThreadState& state) {
    switch(state) {
      case ThreadState::IDLE: return "IDLE";
      case ThreadState::MAIN: return "MAIN";
      case ThreadState::JOINED: return "JOINED";
    }
    return "UNDEFINED";
  }

  std::string communityState(const CommunityState& state) {
    switch(state) {
      case CommunityState::WAITING: return "WAITING";
      case CommunityState::SINGLE_THREADED: return "SINGLE_THREADED";
      case CommunityState::MULTI_THREADED: return "MULTI_THREADED";
      case CommunityState::COMPLETED: return "COMPLETED";
    }
    return "UNDEFINED";
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_context;
  using Base::_history;
  using Base::_max_hn_weights;
  const HypernodeWeight _weight_of_heaviest_node;

  WorkStealing _work_stealing;
  std::vector<CommunityLockBasedHypergraphPruner> _pruner;

  // Vectors to lock hypernodes an hyperedges
  MutexVector<HypernodeID> _hn_mutex;
  MutexVector<HyperedgeID> _he_mutex;

  std::vector<kahypar::ds::FastResetFlagArray<>> _already_matched;
  std::vector<bool> _active;
  std::vector<HypernodeID> _last_touched;

};
}  // namespace kahypar
