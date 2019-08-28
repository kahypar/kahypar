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
          state(CommunityState::WAITING),
          threads_in_community(0),
          contraction_limit(0),
          round(1) { }

        Mutex mutex;
        CommunityState state;
        size_t threads_in_community;
        HypernodeID contraction_limit;
        size_t round;
      };

      struct ThreadData {
        explicit ThreadData() :
          state(ThreadState::IDLE),
          current_community(-1) { }

        ThreadState state;
        PartitionID current_community;
      };

    public:
      WorkStealing(const PartitionID num_communities, const size_t num_threads) :
        _num_communities(num_communities),
        _num_threads(num_threads),
        _assign_mutex(),
        _community_data(num_communities),
        _thread_data(num_threads),
        _community_queues() { 
        for ( PartitionID i = 0; i < num_communities; ++i ) {
          _community_queues.emplace_back(num_threads, 1000);
        }
      }

      WorkStealing(const WorkStealing&) = delete;
      WorkStealing& operator= (const WorkStealing&) = delete;

      WorkStealing(WorkStealing&&) = delete;
      WorkStealing& operator= (WorkStealing&&) = delete;

      PartitionID assignWork(const size_t thread_id) {        
        std::unique_lock<Mutex> assign_lock(_assign_mutex);

        if ( thread(thread_id).state == ThreadState::MAIN ) {
          return thread(thread_id).current_community;
        }

        PartitionID assigned_community = thread(thread_id).current_community;
        size_t max_num_nodes = 0;
        CommunityState max_state = CommunityState::SINGLE_THREADED;
        for ( PartitionID community_id = 0; community_id < _num_communities; ++community_id ) {
          CommunityState state = community(community_id).state;
          if ( ( max_state == CommunityState::WAITING && state != max_state ) ||
                 state == CommunityState::COMPLETED ||
               ( state != CommunityState::WAITING && _community_queues[community_id].unsafe_size() < 500 ) ) {
            continue;
          } else if ( state == CommunityState::WAITING && state != max_state ) {
            assigned_community = community_id;
            max_num_nodes = _community_queues[community_id].unsafe_size();;
            max_state = state;
          } else {
            size_t num_nodes = _community_queues[community_id].unsafe_size();
            if ( max_num_nodes < num_nodes ) {
              assigned_community = community_id;
              max_num_nodes = num_nodes;
              max_state = state;
            }
          }
        }

        max_state = assigned_community != -1 ? community(assigned_community).state : CommunityState::COMPLETED;
        if ( max_num_nodes == 0 || max_state == CommunityState::COMPLETED ) {
          assigned_community = -1;
        }

        if ( assigned_community != -1 ) {
          if ( max_state == CommunityState::WAITING ) {
            community(assigned_community).state = CommunityState::SINGLE_THREADED;
            thread(thread_id).state = ThreadState::MAIN;
          } else if ( max_state == CommunityState::SINGLE_THREADED ) {
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

      void completeWork(const size_t thread_id, const PartitionID community_id) {
        ASSERT(community(community_id).threads_in_community > 0);
        std::unique_lock<Mutex> assign_lock(_assign_mutex);
        if ( thread(thread_id).state == ThreadState::MAIN ) {
          DBG << "Main thread completed community" << community_id;
          community(community_id).state = CommunityState::COMPLETED;
        }
        thread(thread_id).state = ThreadState::IDLE;
        thread(thread_id).current_community = -1;
        --community(community_id).threads_in_community;
      }

      void initializeCommunitySizesAndContractionLimits(CommunityHypergraph& community_hypergraph,
                                                        const HypernodeID limit) {
        for ( PartitionID community_id = 0; community_id < _num_communities; ++community_id ) {
          HypernodeID community_num_nodes = _community_queues[community_id].unsafe_size();
          community_hypergraph.setCommunityNumNodes(community_id, community_num_nodes);
          community(community_id).contraction_limit = 
            std::ceil((((double) community_num_nodes) / community_hypergraph.totalWeight()) * limit);
        }
      }

      Mutex& communityMutex(const PartitionID community_id) {
        return community(community_id).mutex;
      }

      CommunityState communityState(const PartitionID community_id) {
        return community(community_id).state;
      }

      ThreadState threadState(const size_t thread_id) {
        return thread(thread_id).state;
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
    std::vector<HypernodeID> hypernodes;
    for ( const HypernodeID& hn : _hg.nodes() ) {
      hypernodes.push_back(hn);
    }
    Randomize::instance().shuffleVector(hypernodes, hypernodes.size());
    for ( const HypernodeID& hn : hypernodes ) {
      PartitionID community = _hg.communityID(hn);
      _work_stealing.push(community, 0, hn);
    }
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
    PartitionID community_id = _work_stealing.assignWork(thread_id);
    while ( community_id != -1 ) {

      switch ( _work_stealing.communityState(community_id) ) {
        case CommunityState::SINGLE_THREADED:
          singleThreadedCommunityCoarsening(community_hypergraph, lock_based_hypergraph,
            result, rater, community_id, thread_id, _work_stealing.contractionLimit(community_id));
          break;
        case CommunityState::MULTI_THREADED:
          multiThreadedCommunityCoarsening(lock_based_hypergraph, result, rater,
            community_id, thread_id, _work_stealing.contractionLimit(community_id));
          break;
        default:
          break;
      }

      _work_stealing.completeWork(thread_id, community_id);
      community_id = _work_stealing.assignWork(thread_id);
    }
    return result;
  }

  void singleThreadedCommunityCoarsening(CommunityHypergraph& community_hypergraph,
                                         LockBasedCommunityHypergraph& lock_based_hypergraph,
                                         ParallelCoarseningResult& result,
                                         Rater& rater,
                                         const PartitionID community_id,
                                         const size_t thread_id,
                                         const HypernodeID limit) {
    std::unique_lock<Mutex> community_lock(_work_stealing.communityMutex(community_id), std::defer_lock);
    if ( !community_lock.try_lock() ) {
      ASSERT(_work_stealing.communityState(community_id) == CommunityState::MULTI_THREADED);
      multiThreadedCommunityCoarsening(lock_based_hypergraph, result, rater, community_id, thread_id, limit);
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
            LockBasedMemento lock_based_memento(
              community_hypergraph.contract(hn, target), 
              lock_based_hypergraph.drawContractionId());
            result.history.emplace_back(community_id, thread_id, std::move(lock_based_memento)); 
            _already_matched[community_id].set(community_hypergraph.communityNodeID(hn), true);
            _already_matched[community_id].set(community_hypergraph.communityNodeID(target), true);
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
          multiThreadedCommunityCoarsening(lock_based_hypergraph, result, rater, community_id, thread_id, limit);
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
        round = _work_stealing.round(community_id);
        _work_stealing.updateRound(community_id);
        num_contractions = 0;
        _already_matched[community_id].reset();
      }
    }
  }

  void multiThreadedCommunityCoarsening(LockBasedCommunityHypergraph& lock_based_hypergraph,
                                        ParallelCoarseningResult& result,
                                        Rater& rater,
                                        const PartitionID community_id,
                                        const size_t thread_id,
                                        const HypernodeID limit) {
    std::shared_lock<Mutex> community_lock(_work_stealing.communityMutex(community_id));
    LOG << "Thread" << thread_id << "process community" << community_id 
        << "in multi-threaded mode (Contraction Limit =" << limit << ", Thread State ="
        << threadState(_work_stealing.threadState(thread_id)) << ", Community State ="
        << communityState(_work_stealing.communityState(community_id)) << ")";

    size_t round = _work_stealing.round(community_id);
    size_t num_contractions = 0;
    _pruner[thread_id].setCommunity(community_id);
    rater.setCommunity(community_id);
    while ( lock_based_hypergraph.currentCommunityNumNodes(community_id) > limit && !_work_stealing.empty(community_id) ) {

      std::vector<HypernodeID> batch = _work_stealing.pop_batch(community_id);
      Randomize::instance().shuffleVector(batch, batch.size());

      size_t batch_pos = 0;
      for ( const HypernodeID& hn : batch ) {
        ++batch_pos;
        if ( lock_based_hypergraph.nodeIsEnabled(hn) ) {
          Rating rating = rater.rateLockBased(hn, _already_matched[community_id]);
          HypernodeID target = rating.target;

          if ( target != std::numeric_limits<HypernodeID>::max() ) {
            LockBasedMemento memento = lock_based_hypergraph.contract(hn, target);
            if ( memento.memento.u == hn && 
                 memento.memento.v == target ) {
              result.history.emplace_back(community_id, thread_id, std::move(memento));
              _pruner[thread_id].removeLockBasedSingleNodeHyperedges(lock_based_hypergraph, result.history.back());
              _pruner[thread_id].removeLockBasedParallelHyperedges(lock_based_hypergraph, result.history.back());
              _already_matched[community_id].set(lock_based_hypergraph.communityNodeID(hn), true);
              _already_matched[community_id].set(lock_based_hypergraph.communityNodeID(target), true);
              lock_based_hypergraph.unmarkAsActive(hn);
              lock_based_hypergraph.unmarkAsActive(target);
              num_contractions++;
            }
          }

          if ( lock_based_hypergraph.nodeIsEnabled(hn) && lock_based_hypergraph.nodeDegree(hn) > 0 ) {
            _work_stealing.push(community_id, thread_id, hn);
          }
        }

        if ( lock_based_hypergraph.currentCommunityNumNodes(community_id) <= limit ) {
          break;
        }
      }

      if ( lock_based_hypergraph.currentCommunityNumNodes(community_id) <= limit ) {
        break;
      }

      if ( round < _work_stealing.round(community_id) ) {
        if ( num_contractions == 0 ||
             _work_stealing.threadState(thread_id) != ThreadState::MAIN ) {
          break;
        }

        round = _work_stealing.round(community_id);
        num_contractions = 0;
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
