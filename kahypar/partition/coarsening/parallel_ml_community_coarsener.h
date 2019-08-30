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
#include <unordered_map>
#include <chrono>
#include <sched.h>
#include <numa.h>


#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/datastructure/community_hypergraph.h"
#include "kahypar/datastructure/numa_aware_community_hypergraph.h"
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
class ParallelMLCommunityCoarsener final : public ICoarsener,
                                           protected VertexPairCoarsenerBase<>{
 private:
  static constexpr bool debug = false;
  static constexpr HypernodeID kInvalidTarget = std::numeric_limits<HypernodeID>::max();

  using Base = VertexPairCoarsenerBase;
  using CommunityHypergraph = kahypar::ds::CommunityHypergraph<Hypergraph>;
  using NumaAwareCommunityHypergraph = kahypar::ds::NumaAwareCommunityHypergraph<Hypergraph>;
  using Rater = VertexPairRater<ScorePolicy,
                                HeavyNodePenaltyPolicy,
                                CommunityPolicy,
                                RatingPartitionPolicy,
                                AcceptancePolicy,
                                FixedVertexPolicy,
                                RatingType,
                                NumaAwareCommunityHypergraph>;
  using Rating = typename Rater::Rating;
  using CurrentMaxNodeWeight = typename CoarsenerBase::CurrentMaxNodeWeight;

  using CommunitySize = std::pair<PartitionID, size_t>;
  using ThreadPool = kahypar::parallel::ThreadPool;
  using History = std::vector<CoarseningMemento>;
  using Hierarchy = std::vector<typename Hypergraph::ContractionMemento>;
  using HypernodeMapping = std::shared_ptr<std::vector<HypernodeID>>;

  struct ParallelCoarseningResult {

    ParallelCoarseningResult(const PartitionID community_id,
                             const size_t num_nodes) :
      community_id(community_id),
      history(),
      pruner(num_nodes, community_id),
      max_hn_weights(),
      start(std::chrono::high_resolution_clock::now()),
      end(),
      numa_node(-1) { }

    PartitionID community_id;
    History history;
    NumaAwareCommunityHypergraphPruner pruner;
    std::vector<CurrentMaxNodeWeight> max_hn_weights;
    HighResClockTimepoint start;
    HighResClockTimepoint end;
    int numa_node;
  };

 public:
  ParallelMLCommunityCoarsener(Hypergraph& hypergraph, const Context& context,
              const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _pruner(),
    _community_to_pruner(),
    _weight_of_heaviest_node(weight_of_heaviest_node),
    _enableRandomization(true) { }

  ~ParallelMLCommunityCoarsener() override = default;

  ParallelMLCommunityCoarsener(const ParallelMLCommunityCoarsener&) = delete;
  ParallelMLCommunityCoarsener& operator= (const ParallelMLCommunityCoarsener&) = delete;

  ParallelMLCommunityCoarsener(ParallelMLCommunityCoarsener&&) = delete;
  ParallelMLCommunityCoarsener& operator= (ParallelMLCommunityCoarsener&&) = delete;

  void disableRandomization() {
    _enableRandomization = false;
  }

 private:
  void coarsenImpl(const HypernodeID limit) override final {
    ASSERT(_context.shared_memory.pool, "Thread pool not initialized");
    HighResClockTimepoint global_start = std::chrono::high_resolution_clock::now();
    ThreadPool& pool = *_context.shared_memory.pool;
    CommunityHypergraph community_hypergraph(_hg, _context, pool);

    // Setup internal structures in hypergraph such that
    // parallel contractions are possible
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    community_hypergraph.initialize();
    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::hypergraph_preparation,
                          std::chrono::duration<double>(end - start).count());


    start = std::chrono::high_resolution_clock::now();
    std::vector<NumaAwareCommunityHypergraph> numa_hypergraphs;
    for ( int node = 0; node < numa_num_configured_nodes(); ++node ) {
      numa_hypergraphs.emplace_back(_hg, _context, node);
    }
    end = std::chrono::high_resolution_clock::now();
    LOG << "Initialize Numa Hypergraphs = " << std::chrono::duration<double>(end - start).count() << "s";

    // Compute hypernodes per community and community sizes. Community sizes
    // are used to sort communities in decreasing order and hypernodes per
    // community are used to iterate more efficient over hypernodes of
    // a community during coarsening.
    start = std::chrono::high_resolution_clock::now();
    std::unordered_map<PartitionID, HypernodeMapping> community_to_hns;
    for ( const HypernodeID& hn : _hg.nodes() ) {
      PartitionID community = _hg.communityID(hn);
      if ( community_to_hns.find(community) == community_to_hns.end() ) {
        community_to_hns[community] = std::make_shared<std::vector<HypernodeID>>();
      }
      community_to_hns[community]->push_back(hn);
    }

    std::vector<CommunitySize> community_sizes;
    for ( const auto& community : community_to_hns ) {
      community_sizes.push_back(std::make_pair(community.first, community.second->size()));
    }
    std::sort(community_sizes.begin(), community_sizes.end(),
      [](const CommunitySize& lhs, const CommunitySize& rhs) {
        return lhs.second > rhs.second || (lhs.second == rhs.second && lhs.first < rhs.first);
    });
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::compute_community_sizes,
                          std::chrono::duration<double>(end - start).count());


    // Enqueue a coarsening job for each community in thread pool
    start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<ParallelCoarseningResult>> results;
    std::string desc_community_sizes = "";
    for ( const CommunitySize& community_size : community_sizes ) {
      const PartitionID community_id = community_size.first;
      size_t size = community_size.second;
      //desc_community_sizes += std::to_string(size) + ",";
      HypernodeMapping community_hns = community_to_hns[community_id];
      DBG << "Enqueue parallel contraction job of community" << community_id
          << "of size" << size;
      results.push_back(pool.enqueue([this, &community_hypergraph, 
        &numa_hypergraphs, community_id, limit, size, community_hns]() {
        // Since we coarsen only on a subhypergraph of the original hypergraph
        // we have to recalculate the contraction limit for coarsening
        DBG << "Start coarsening of community" << community_id
            << "with" << community_hns->size() << "hypernodes";
        Context community_context(this->_context);
        HypernodeID contraction_limit =
          std::ceil((((double) size) / this->_hg.totalWeight()) * limit);
        community_context.coarsening.contraction_limit = contraction_limit;
        community_context.coarsening.community_contraction_target = community_id;

        // Initialize incident nets on numa node
        int node = numa_node_of_cpu(sched_getcpu());
        NumaAwareCommunityHypergraph& numa_hypergraph = numa_hypergraphs[node];
        numa_hypergraph.initializeIncidentNets(*community_hns, community_id);

        return this->parallelCoarsening(numa_hypergraph,
          community_context, community_id, community_hns);
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
    Hierarchy hierarchy = createUncontractionHistory(coarsening_results);
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::merge_hierarchies,
                          std::chrono::duration<double>(end - start).count());

    for ( const ParallelCoarseningResult& result : coarsening_results ) {
      std::cout << "COMMUNITY_RESULT graph=" << _context.partition.graph_filename.substr(
                   _context.partition.graph_filename.find_last_of('/') + 1)
                << " seed=" << _context.partition.seed
                << " num_threads=" << _context.shared_memory.num_threads
                << " community=" << result.community_id
                << " start=" << std::chrono::duration<double>(result.start - global_start).count()
                << " end=" << std::chrono::duration<double>(result.end - global_start).count()
                << " duration=" << std::chrono::duration<double>(result.end - result.start).count()
                << " numa_node=" << result.numa_node
                << std::endl;
    }

    // Undo changes on internal hypergraph data structure done by
    // prepareForParallelCommunityAwareCoarsening(...) such that
    // hypergraph can be again used in a sequential setting
    start = std::chrono::high_resolution_clock::now();
    community_hypergraph.undo(hierarchy);
    end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(_context, Timepoint::undo_preparation,
                          std::chrono::duration<double>(end - start).count());
  }

  /**
   * Implements ml_style coarsening within one community (ml_coarsener.h).
   */
  ParallelCoarseningResult parallelCoarsening(NumaAwareCommunityHypergraph& numa_hypergraph,
                                              const Context& community_context,
                                              const PartitionID community_id,
                                              const HypernodeMapping community_hns) {
    ParallelCoarseningResult result(community_id, community_hns->size());
    result.numa_node = numa_node_of_cpu(sched_getcpu());
    result.max_hn_weights.emplace_back(
      CurrentMaxNodeWeight { 0, _weight_of_heaviest_node } );

    // Since internal coarsening structures sometimes allocate for some
    // internal data structures memory that is proportional to the intial
    // number of nodes, we create a mapping from the original hypergraph
    // to a remapped consecutive range of hypernodes, such that memory allocated
    // by those structures is proportional to the number of nodes in the community
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    size_t current_num_nodes = community_hns->size();
    Rater rater(numa_hypergraph, community_context, community_hns);

    int pass_nr = 0;
    HypernodeID contraction_idx = 0;
    std::vector<HypernodeID> current_hns;
    while( current_num_nodes > community_context.coarsening.contraction_limit ) {
      DBG << V(pass_nr);
      DBG << V(current_num_nodes);

      rater.resetMatches();
      current_hns.clear();
      size_t num_hns_before_pass = current_num_nodes;
      for (const HypernodeID& hn : *community_hns) {
        if ( numa_hypergraph.nodeIsEnabled(hn) && numa_hypergraph.nodeDegree(hn) > 0 ) {
          current_hns.push_back(hn);
        }
      }

      if ( _enableRandomization ) {
        Randomize::instance().shuffleVector(current_hns, current_hns.size());
      }

      for ( const HypernodeID& hn : current_hns ) {
        if ( numa_hypergraph.nodeIsEnabled(hn) ) {
          ASSERT(numa_hypergraph.communityID(hn) == community_id, 
            "Hypernode " << hn << " is not in community " << community_id);
          const Rating rating = rater.rate(hn);

          if ( rating.target != kInvalidTarget ) {
            ASSERT(numa_hypergraph.communityID(rating.target) == community_id, "Contraction target " << rating.target
              << " is not in same community");
            rater.markAsMatched(numa_hypergraph.communityNodeID(hn));
            rater.markAsMatched(numa_hypergraph.communityNodeID(rating.target));

            DBG << "Contract (" << hn << "," << rating.target << ")";
            result.history.emplace_back(community_id,
              numa_hypergraph.contract(hn, rating.target));
            result.pruner.removeSingleNodeHyperedges(numa_hypergraph, result.history.back());
            result.pruner.removeParallelHyperedges(numa_hypergraph, result.history.back());
            if ( result.max_hn_weights.back().max_weight < _hg.nodeWeight(hn) ) {
              result.max_hn_weights.emplace_back(
                CurrentMaxNodeWeight { contraction_idx, _hg.nodeWeight(hn) } );
            }
            current_num_nodes--;
            contraction_idx++;
          }

          if ( current_num_nodes <= community_context.coarsening.contraction_limit ) {
            break;
          }
        }
      }

      if ( num_hns_before_pass == current_num_nodes ) {
        break;
      }
      ++pass_nr;
    }

    numa_hypergraph.copyBack(*community_hns, community_id);

    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    LOG << "Finish coarsening of community" << community_id
        << "( initial_num_nodes = " << community_hns->size()
        << V(current_num_nodes) << ","
        << V(community_context.coarsening.contraction_limit)
        << "time =" << std::chrono::duration<double>(end - start).count() << "s)";
    result.end = std::chrono::high_resolution_clock::now();
    return result;
  }

  Hierarchy createUncontractionHistory(std::vector<ParallelCoarseningResult>& results) {
    for ( ParallelCoarseningResult& result : results ) {
      PartitionID community_id = result.pruner.communityID();
      _community_to_pruner[community_id] = _pruner.size();
      _pruner.emplace_back(std::move(result.pruner));
    }
    HypernodeID current_num_nodes = _hg.initialNumNodes() - 1;

    // Merge coarsening mementos into global history
    // Currently implemented in round-robin fashion
    Hierarchy hierarchy;
    std::vector<size_t> indicies(results.size(), 0);
    std::vector<size_t> max_hn_weight_indicies(results.size(), 0);
    bool contraction_left = true;
    while( contraction_left ) {
      bool found = false;
      size_t idx = 0;
      for ( ParallelCoarseningResult& result : results ) {
        size_t i = indicies[idx];
        if ( i < result.history.size() ) {
          hierarchy.push_back(result.history[i].contraction_memento);
          _history.emplace_back(std::move(result.history[i]));

          // Compute max node weight for current number of nodes
          // from coarsening results
          size_t max_hn_weight_i = max_hn_weight_indicies[idx];
          ASSERT(max_hn_weight_i < result.max_hn_weights.size(), "Error");
          if ( i == result.max_hn_weights[max_hn_weight_i].num_nodes ) {
            if ( _max_hn_weights.back().max_weight <
                  result.max_hn_weights[max_hn_weight_i].max_weight ) {
              _max_hn_weights.emplace_back(
                CurrentMaxNodeWeight { current_num_nodes,
                  result.max_hn_weights[max_hn_weight_i].max_weight } );
            }
            ++max_hn_weight_indicies[idx];
          }
          --current_num_nodes;
          ++indicies[idx];
          found = true;
        }
        idx++;
      }
      contraction_left = found;
    }
    return hierarchy;
  }

  bool uncoarsenImpl(IRefiner& refiner) override final {
    return doUncoarsen(refiner);
  }

  void restoreParallelHyperedges() override {
    PartitionID community_id = _history.back().community_id;
    NumaAwareCommunityHypergraphPruner& pruner = _pruner[_community_to_pruner[community_id]];
    pruner.restoreParallelHyperedges(_hg, _history.back());
  }

  void restoreSingleNodeHyperedges() override {
    PartitionID community_id = _history.back().community_id;
    NumaAwareCommunityHypergraphPruner& pruner = _pruner[_community_to_pruner[community_id]];
    pruner.restoreSingleNodeHyperedges(_hg, _history.back());
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_context;
  using Base::_history;
  using Base::_max_hn_weights;
  std::vector<NumaAwareCommunityHypergraphPruner> _pruner;
  std::unordered_map<PartitionID, size_t> _community_to_pruner;
  const HypernodeWeight _weight_of_heaviest_node;
  bool _enableRandomization;

};
}  // namespace kahypar
