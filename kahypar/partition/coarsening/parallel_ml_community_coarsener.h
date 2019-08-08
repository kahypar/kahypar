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


#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/datastructure/community_subhypergraph.h"
#include "kahypar/datastructure/fast_hash_table.h"
#include "kahypar/partition/coarsening/policies/fixed_vertex_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_acceptance_policy.h"
#include "kahypar/partition/coarsening/policies/rating_community_policy.h"
#include "kahypar/partition/coarsening/policies/rating_heavy_node_penalty_policy.h"
#include "kahypar/partition/coarsening/policies/rating_partition_policy.h"
#include "kahypar/partition/coarsening/policies/rating_score_policy.h"
#include "kahypar/partition/coarsening/policies/rating_tie_breaking_policy.h"
#include "kahypar/partition/coarsening/vertex_pair_rater.h"
#include "kahypar/partition/coarsening/parallel_hypergraph_pruner.h"

namespace kahypar {

template <class ScorePolicy = HeavyEdgeScore,
          class HeavyNodePenaltyPolicy = NoWeightPenalty,
          class CommunityPolicy = UseCommunityStructure,
          class RatingPartitionPolicy = NormalPartitionPolicy,
          class AcceptancePolicy = BestRatingPreferringUnmatched<>,
          class FixedVertexPolicy = AllowFreeOnFixedFreeOnFreeFixedOnFixed,
          typename RatingType = RatingType>
class ParallelMLCommunityCoarsener final : public ICoarsener,
                                           private VertexPairCoarsenerBase<>{
 private:
  static constexpr bool debug = false;
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

  using CommunitySize = std::pair<PartitionID, size_t>;
  using ThreadPool = kahypar::parallel::ThreadPool;
  using History = std::vector<CoarseningMemento>;
  using Hierarchy = std::vector<typename Hypergraph::ContractionMemento>;
  using HypernodeMapping = std::shared_ptr<std::vector<HypernodeID>>;  
  //using HashTable = std::unordered_map<HypernodeID, HypernodeID>;
  using HashTable = kahypar::ds::FastHashTable<>;
  using ReverseHypernodeMapping = std::shared_ptr<HashTable>;
  using ParallelHyperedge = typename ParallelHypergraphPruner::ParallelHE;

  struct ParallelCoarseningResult {

    ParallelCoarseningResult(const PartitionID community_id,
                             const size_t num_nodes) :
      community_id(community_id),
      history(),
      pruner(community_id, num_nodes) { }

    PartitionID community_id;
    History history;
    ParallelHypergraphPruner pruner;
  };

 public:
  ParallelMLCommunityCoarsener(Hypergraph& hypergraph, const Context& context,
              const HypernodeWeight weight_of_heaviest_node) :
    Base(hypergraph, context, weight_of_heaviest_node),
    _history(),
    _pruner(),
    _community_to_pruner(),
    _weight_of_heaviest_node(weight_of_heaviest_node) { }

  ~ParallelMLCommunityCoarsener() override = default;

  ParallelMLCommunityCoarsener(const ParallelMLCommunityCoarsener&) = delete;
  ParallelMLCommunityCoarsener& operator= (const ParallelMLCommunityCoarsener&) = delete;

  ParallelMLCommunityCoarsener(ParallelMLCommunityCoarsener&&) = delete;
  ParallelMLCommunityCoarsener& operator= (ParallelMLCommunityCoarsener&&) = delete;

 private:
  void coarsenImpl(const HypernodeID limit) override final {
    ASSERT(_context.shared_memory.pool, "Thread pool not initialized");
    ThreadPool& pool = *_context.shared_memory.pool;

    DBG << "Compute Community Sizes";
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
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
    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    LOG << "Compute Community Sizes = " << std::chrono::duration<double>(end - start).count() << " s";

    DBG << "Prepare Hypergraph Data Structure";
    start = std::chrono::high_resolution_clock::now();
    prepareForParallelCommunityAwareCoarsening(pool, _hg);
    end = std::chrono::high_resolution_clock::now();
    LOG << "Prepare Hypergraph Data Structure = " << std::chrono::duration<double>(end - start).count() << " s";


    DBG << "Parallel Community Coarsening";
    start = std::chrono::high_resolution_clock::now();
    std::vector<std::future<ParallelCoarseningResult>> results;
    for ( const CommunitySize& community_size : community_sizes ) {
      const PartitionID community_id = community_size.first;
      size_t size = community_size.second;
      HypernodeMapping community_hns = community_to_hns[community_id];
      DBG << V(community_id) << V(size);
      results.push_back(pool.enqueue([this, community_id, limit, size, community_hns]() {
        Context community_context(this->_context);
        HypernodeID contraction_limit = 
          std::ceil((((double) size) / this->_hg.totalWeight()) * limit);
        community_context.coarsening.contraction_limit = contraction_limit;
        community_context.coarsening.community_contraction_target = community_id;

        return this->coarsenCommunity(community_context, community_id, community_hns);
      }));
    }
    pool.loop_until_empty();
    end = std::chrono::high_resolution_clock::now();
    LOG << "Parallel Community Coarsening = " << std::chrono::duration<double>(end - start).count() << " s";


    DBG << "Move Contraction Results";
    start = std::chrono::high_resolution_clock::now();
    std::vector<ParallelCoarseningResult> coarsening_results;
    for ( std::future<ParallelCoarseningResult>& fut : results ) {
      coarsening_results.emplace_back(std::move(fut.get()));
    }
    end = std::chrono::high_resolution_clock::now();
    LOG << "Move Contraction Results = " << std::chrono::duration<double>(end - start).count() << " s";

    DBG << "Create Contraction Hierarchy";
    start = std::chrono::high_resolution_clock::now();
    Hierarchy hierarchy = createUncontractionHistory(coarsening_results);
    end = std::chrono::high_resolution_clock::now();
    LOG << "Create Contraction Hierarchy = " << std::chrono::duration<double>(end - start).count() << " s";

    DBG << "Undo Parallel Community Coarsening Preparation";
    start = std::chrono::high_resolution_clock::now();
    kahypar::ds::undoPreparationForParallelCommunityAwareCoarsening(pool, _hg, hierarchy);
    end = std::chrono::high_resolution_clock::now();
    LOG << "Undo Parallel Community Coarsening Preparation = " << std::chrono::duration<double>(end - start).count() << " s";
  }

  ParallelCoarseningResult coarsenCommunity(const Context& community_context, 
                                            const PartitionID community_id,
                                            const HypernodeMapping community_hns) {
    size_t current_num_nodes = community_hns->size();
    ReverseHypernodeMapping reverse_mapping =
      std::make_shared<HashTable>(current_num_nodes);
    for ( HypernodeID hn = 0; hn < community_hns->size(); ++hn ) {
      (*reverse_mapping).insert((*community_hns)[hn], hn);
      //(*reverse_mapping)[(*community_hns)[hn]] = hn;
    }

    ParallelCoarseningResult result(community_id, community_hns->size());
    Rater rater(_hg, community_context, community_hns, reverse_mapping);

    int pass_nr = 0;
    std::vector<HypernodeID> current_hns;
    while( current_num_nodes > community_context.coarsening.contraction_limit ) {
      DBG << V(pass_nr);
      DBG << V(current_num_nodes);

      rater.resetMatches();
      current_hns.clear();
      size_t num_hns_before_pass = current_num_nodes;
      for (const HypernodeID& hn : *community_hns) {
        if ( _hg.nodeIsEnabled(hn) ) {
          current_hns.push_back(hn);
        }
      }
      Randomize::instance().shuffleVector(current_hns, current_hns.size());


      for ( const HypernodeID& hn : current_hns ) {
        if ( _hg.nodeIsEnabled(hn) ) {
          ASSERT(_hg.communityID(hn) == community_id, "Hypernode " << hn << " is not in community " 
                                                                    << community_id);
          const Rating rating = rater.rate(hn);

          if ( rating.target != kInvalidTarget ) {
            ASSERT(_hg.communityID(rating.target) == community_id, "Contraction target " << rating.target 
              << " is not in same community");
            rater.markAsMatched(reverse_mapping->find(hn)->second);
            rater.markAsMatched(reverse_mapping->find(rating.target)->second);

            result.history.emplace_back(community_id, 
              _hg.parallelContract(community_id, hn, rating.target));
            result.pruner.removeSingleNodeHyperedges(_hg, result.history.back());
            // result.pruner.removeParallelHyperedges(_hg, result.history.back(), reverse_mapping);
            current_num_nodes--;
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

    return result;
  }

  Hierarchy createUncontractionHistory(std::vector<ParallelCoarseningResult>& results) {
    size_t history_size = 0;
    for ( ParallelCoarseningResult& result : results ) {
      PartitionID community_id = result.pruner.communityID();
      _community_to_pruner[community_id] = _pruner.size();
      _pruner.emplace_back(std::move(result.pruner));
      history_size += result.history.size();
    }

    Hierarchy hierarchy;
    std::vector<size_t> indicies(results.size(), 0);
    bool contraction_left = true;
    while( contraction_left ) {
      bool found = false;
      size_t idx = 0;
      for ( ParallelCoarseningResult& result : results ) {
        if ( indicies[idx] < result.history.size() ) {
          hierarchy.push_back(result.history[indicies[idx]].contraction_memento);
          _history.emplace_back(std::move(result.history[indicies[idx]++]));
          found = true;
        }
        idx++;
      }
      contraction_left = found;
    }
    return hierarchy;
  }

  bool uncoarsenImpl(IRefiner& refiner) override final {
    Metrics current_metrics = { metrics::hyperedgeCut(_hg),
                                metrics::km1(_hg),
                                metrics::imbalance(_hg, _context) };
    HyperedgeWeight initial_objective = std::numeric_limits<HyperedgeWeight>::min();

    switch (_context.partition.objective) {
      case Objective::cut:
        initial_objective = current_metrics.cut;
        break;
      case Objective::km1:
        initial_objective = current_metrics.km1;
        break;
      default:
        LOG << "Unknown Objective";
        exit(-1);
    }

    if (_context.type == ContextType::main) {
      _context.stats.set(StatTag::InitialPartitioning, "inititalCut", current_metrics.cut);
      _context.stats.set(StatTag::InitialPartitioning, "inititalKm1", current_metrics.km1);
      _context.stats.set(StatTag::InitialPartitioning, "initialImbalance",
                         current_metrics.imbalance);
    }

    initializeRefiner(refiner);
    std::vector<HypernodeID> refinement_nodes(2, 0);
    UncontractionGainChanges changes;
    changes.representative.push_back(0);
    changes.contraction_partner.push_back(0);
    size_t uncontraction_nr = 0;
    while (!_history.empty()) {
      uncontraction_nr++;
      PartitionID community_id = _history.back().community_id;
      ParallelHypergraphPruner& pruner = _pruner[_community_to_pruner[community_id]];
      // pruner.restoreParallelHyperedges(_hg, _history.back());
      // pruner.restoreSingleNodeHyperedges(_hg, _history.back());
      LOG << V(uncontraction_nr);
      DBG << "Uncontracting: (" << _history.back().contraction_memento.u << ","
          << _history.back().contraction_memento.v << ")";

      refinement_nodes.clear();
      refinement_nodes.push_back(_history.back().contraction_memento.u);
      refinement_nodes.push_back(_history.back().contraction_memento.v);

      if (_context.local_search.algorithm == RefinementAlgorithm::twoway_fm ||
          _context.local_search.algorithm == RefinementAlgorithm::twoway_fm_flow) {
        _hg.uncontract(_history.back().contraction_memento, changes,
                       meta::Int2Type<static_cast<int>(RefinementAlgorithm::twoway_fm)>());
      } else {
        _hg.uncontract(_history.back().contraction_memento);
      }

      performLocalSearch(refiner, refinement_nodes, current_metrics, changes);
      changes.representative[0] = 0;
      changes.contraction_partner[0] = 0;
      _history.pop_back();
    }

    bool improvement_found = false;
    switch (_context.partition.objective) {
      case Objective::cut:
        // _context.stats.set(StatTag::LocalSearch, "finalCut", current_metrics.cut);
        improvement_found = current_metrics.cut < initial_objective;
        break;
      case Objective::km1:
        if (_context.partition.mode == Mode::recursive_bisection) {
          // In recursive bisection-based (initial) partitioning, km1
          // is optimized using TwoWayFM and cut-net splitting. Since
          // TwoWayFM optimizes cut, current_metrics.km1 is not updated
          // during local search (it is currently only updated/maintained
          // during k-way k-1 refinement). In order to provide correct outputs,
          // we explicitly calculated the metric after uncoarsening.
          current_metrics.km1 = metrics::km1(_hg);
        }
        // _context.stats.set(StatTag::LocalSearch, "finalKm1", current_metrics.km1);
        improvement_found = current_metrics.km1 < initial_objective;
        break;
      default:
        LOG << "Unknown Objective";
        exit(-1);
    }

    return improvement_found;
  }

  using Base::_pq;
  using Base::_hg;
  using Base::_context;
  std::vector<CoarseningMemento> _history;
  std::vector<ParallelHypergraphPruner> _pruner;
  std::unordered_map<PartitionID, size_t> _community_to_pruner;
  const HypernodeWeight _weight_of_heaviest_node;

};
}  // namespace kahypar
