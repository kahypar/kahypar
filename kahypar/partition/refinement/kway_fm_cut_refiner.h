/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <stack>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "kahypar/datastructure/fast_reset_array.h"
#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_map.h"
#include "kahypar/definitions.h"
#include "kahypar/meta/mandatory.h"
#include "kahypar/meta/template_parameter_to_string.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/fm_refiner_base.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/kway_fm_gain_cache.h"
#include "kahypar/partition/refinement/policies/fm_improvement_policy.h"
#include "kahypar/utils/float_compare.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
template <class StoppingPolicy = Mandatory,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class KWayFMRefiner final : public IRefiner,
                            private FMRefinerBase<RollbackInfo, KWayFMRefiner<StoppingPolicy,
                                                                              FMImprovementPolicy> >{
 private:
  static constexpr bool debug = false;
  static constexpr HypernodeID hn_to_debug = 4242;

  using GainCache = KwayGainCache<Gain>;
  using Base = FMRefinerBase<RollbackInfo, KWayFMRefiner<StoppingPolicy,
                                                         FMImprovementPolicy> >;

  friend class FMRefinerBase<RollbackInfo, KWayFMRefiner<StoppingPolicy,
                                                         FMImprovementPolicy> >;

  using HEState = typename Base::HEState;
  using Base::kInvalidGain;
  using Base::kInvalidHN;

 public:
  KWayFMRefiner(Hypergraph& hypergraph, const Context& context) :
    Base(hypergraph, context),
    _he_fully_active(_hg.initialNumEdges()),
    _tmp_gains(_context.partition.k, 0),
    _already_processed_part(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
    _locked_hes(_hg.initialNumEdges(), HEState::free),
    _gain_cache(_hg.initialNumNodes(), _context.partition.k),
    _stopping_policy() { }

  ~KWayFMRefiner() override = default;

  KWayFMRefiner(const KWayFMRefiner&) = delete;
  KWayFMRefiner& operator= (const KWayFMRefiner&) = delete;

  KWayFMRefiner(KWayFMRefiner&&) = delete;
  KWayFMRefiner& operator= (KWayFMRefiner&&) = delete;

 private:
  FRIEND_TEST(AKwayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInitialGainComputation);
  FRIEND_TEST(AKwayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInducedGainComputation);
  FRIEND_TEST(AKwayFMRefiner, KnowsIfAHyperedgeIsFullyActive);

  void initializeImpl(const HyperedgeWeight max_gain) override final {
    if (!_is_initialized) {
#ifdef USE_BUCKET_QUEUE
      _pq.initialize(_hg.initialNumNodes(), max_gain);
#else
      unused(max_gain);
      _pq.initialize(_hg.initialNumNodes());
#endif
      _is_initialized = true;
    }
    _gain_cache.clear();
    initializeGainCache();
  }

  void performMovesAndUpdateCacheImpl(const std::vector<Move>& moves,
                                      std::vector<HypernodeID>& refinement_nodes,
                                      const UncontractionGainChanges& changes) override final {
    _he_fully_active.reset();
    _locked_hes.resetUsedEntries();
    Base::performMovesAndUpdateCache(moves, refinement_nodes, changes);
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics& best_metrics) override final {
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg),
           V(best_metrics.cut) << V(metrics::hyperedgeCut(_hg)));
    ASSERT(FloatingPoint<double>(best_metrics.imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg, _context))),
           V(best_metrics.imbalance) << V(metrics::imbalance(_hg, _context)));

    Base::reset();
    _he_fully_active.reset();
    _locked_hes.resetUsedEntries();


    Randomize::instance().shuffleVector(refinement_nodes, refinement_nodes.size());
    for (const HypernodeID& hn : refinement_nodes) {
      activate<true>(hn);
    }

    Base::activateAdjacentFreeVertices(refinement_nodes);
    ASSERT_THAT_GAIN_CACHE_IS_VALID();

    const HyperedgeWeight initial_cut = best_metrics.cut;
    const double initial_imbalance = best_metrics.imbalance;
    HyperedgeWeight current_cut = best_metrics.cut;
    double current_imbalance = best_metrics.imbalance;

    int min_cut_index = -1;
    int touched_hns_since_last_improvement = 0;
    _stopping_policy.resetStatistics();

    const double beta = log(_hg.currentNumNodes());
    while (!_pq.empty() &&
           !_stopping_policy.searchShouldStop(touched_hns_since_last_improvement,
                                              _context, beta, best_metrics.cut, current_cut)) {
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      PartitionID from_part = _hg.partID(max_gain_node);

      DBG << V(current_cut) << V(max_gain_node) << V(max_gain)
          << V(_hg.partID(max_gain_node)) << V(to_part);

      ASSERT(!_hg.marked(max_gain_node), V(max_gain_node));
      ASSERT(max_gain == gainInducedByHypergraph(max_gain_node, to_part));
      ASSERT(_hg.isBorderNode(max_gain_node));
      ASSERT([&]() {
          _hg.changeNodePart(max_gain_node, from_part, to_part);
          ASSERT((current_cut - max_gain) == metrics::hyperedgeCut(_hg),
                 "cut=" << current_cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
          _hg.changeNodePart(max_gain_node, to_part, from_part);
          return true;
        } ());

      // Staleness assertion: The move should be to a part that is in the connectivity superset of
      // the max_gain_node.
      ASSERT(Base::hypernodeIsConnectedToPart(max_gain_node, to_part),
             V(max_gain_node) << V(from_part) << V(to_part));

      // remove all other possible moves of the current max_gain_node
      for (const PartitionID& part : _gain_cache.adjacentParts(max_gain_node)) {
        if (part == to_part) {
          continue;
        }
        _pq.remove(max_gain_node, part);
      }
      ASSERT([&]() {
          for (PartitionID part = 0; part < _context.partition.k; ++part) {
            if (_pq.contains(max_gain_node, part)) {
              return false;
            }
          }
          return true;
        } (), V(max_gain_node));

      _hg.mark(max_gain_node);
      ++touched_hns_since_last_improvement;

      if (Base::moveIsFeasible(max_gain_node, from_part, to_part)) {
        Base::moveHypernode(max_gain_node, from_part, to_part);

        if (_hg.partWeight(to_part) >= _context.partition.max_part_weights[0]) {
          _pq.disablePart(to_part);
        }
        if (_hg.partWeight(from_part) < _context.partition.max_part_weights[0]) {
          _pq.enablePart(from_part);
        }

        current_imbalance = metrics::imbalance(_hg, _context);

        current_cut -= max_gain;
        _stopping_policy.updateStatistics(max_gain);

        ASSERT(current_cut == metrics::hyperedgeCut(_hg),
               V(current_cut) << V(metrics::hyperedgeCut(_hg)));
        ASSERT(current_imbalance == metrics::imbalance(_hg, _context),
               V(current_imbalance) << V(metrics::imbalance(_hg, _context)));

        updateNeighbours(max_gain_node, from_part, to_part);

        // right now, we do not allow a decrease in cut in favor of an increase in balance
        const bool improved_cut_within_balance = (current_imbalance <= _context.partition.epsilon) &&
                                                 (current_cut < best_metrics.cut);
        const bool improved_balance_less_equal_cut = (current_imbalance < best_metrics.imbalance) &&
                                                     (current_cut <= best_metrics.cut);
        // if (current_cut < best_metrics.cut && current_imbalance > _context.partition.epsilon) {
        //   LOG << V(current_cut) << V(best_metrics.cut) << V(current_imbalance);
        // }

        if (improved_cut_within_balance || improved_balance_less_equal_cut) {
          DBGC(max_gain == 0) << "KWayFM improved balance between" << from_part << "and "
                              << to_part << "(max_gain=" << max_gain << ")";
          DBGC(current_cut < best_metrics.cut) << "KWayFM improved cut from "
                                               << best_metrics.cut << "to" << current_cut;
          best_metrics.cut = current_cut;
          best_metrics.imbalance = current_imbalance;
          _stopping_policy.resetStatistics();
          min_cut_index = _performed_moves.size();
          touched_hns_since_last_improvement = 0;
          _gain_cache.resetDelta();
        }
        _performed_moves.emplace_back(RollbackInfo { max_gain_node, from_part, to_part });
      }
    }
    DBG << "KWayFM performed" << _performed_moves.size()
        << "local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (_stopping_policy.searchShouldStop(touched_hns_since_last_improvement, _context, beta,
                                          best_metrics.cut, current_cut)
        == true ? "policy" : "empty queue");

    Base::rollback(_performed_moves.size() - 1, min_cut_index);
    _gain_cache.rollbackDelta();

    ASSERT_THAT_GAIN_CACHE_IS_VALID();
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg));
    ASSERT(best_metrics.cut <= initial_cut, V(initial_cut) << V(best_metrics.cut));

    return FMImprovementPolicy::improvementFound(best_metrics.cut, initial_cut,
                                                 best_metrics.imbalance,
                                                 initial_imbalance, _context.partition.epsilon);
  }

  bool moveAffectsGainOrConnectivityUpdate(const HypernodeID pin_count_target_part_before_move,
                                           const HypernodeID pin_count_source_part_after_move,
                                           const HypernodeID he_size)
  const {
    return (pin_count_source_part_after_move == 0 ||
            pin_count_target_part_before_move == 0 ||
            pin_count_target_part_before_move + 1 == he_size - 1 ||
            pin_count_source_part_after_move + 1 == he_size - 1);
  }

  void deltaGainUpdatesForCacheOnly(const HypernodeID pin, const PartitionID from_part,
                                    const PartitionID to_part, const HyperedgeID he,
                                    const HypernodeID he_size, const HyperedgeWeight he_weight,
                                    const HypernodeID pin_count_source_part_before_move,
                                    const HypernodeID pin_count_target_part_after_move)
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE {
    deltaGainUpdates<true>(pin, from_part, to_part, he, he_size,
                           he_weight, pin_count_source_part_before_move,
                           pin_count_target_part_after_move);
  }


  void deltaGainUpdatesForPQandCache(const HypernodeID pin, const PartitionID from_part,
                                     const PartitionID to_part, const HyperedgeID he,
                                     const HypernodeID he_size, const HyperedgeWeight he_weight,
                                     const HypernodeID pin_count_source_part_before_move,
                                     const HypernodeID pin_count_target_part_after_move)
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE {
    deltaGainUpdates<false>(pin, from_part, to_part, he, he_size,
                            he_weight, pin_count_source_part_before_move,
                            pin_count_target_part_after_move);
  }

  template <bool update_cache_only = false>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE
  void deltaGainUpdates(const HypernodeID pin, const PartitionID from_part,
                        const PartitionID to_part, const HyperedgeID he, const HypernodeID he_size,
                        const HyperedgeWeight he_weight,
                        const HypernodeID pin_count_source_part_before_move,
                        const HypernodeID pin_count_target_part_after_move) {
    if (pin_count_source_part_before_move == he_size) {
      ASSERT(_hg.connectivity(he) == 2, V(_hg.connectivity(he)));
      ASSERT(pin_count_target_part_after_move == 1, V(pin_count_target_part_after_move));
      DBG << "he" << he << "is not cut before applying move";
      // Update pin of a HE that is not cut before applying the move.
      for (const PartitionID& part : _gain_cache.adjacentParts(pin)) {
        if (part != from_part) {
          if (update_cache_only) {
            if (_already_processed_part.get(pin) != part) {
              _gain_cache.updateExistingEntry(pin, part, he_weight);
            }
          } else {
            updatePin(pin, part, he, he_weight);
          }
        }
      }
    }
    if (pin_count_target_part_after_move == he_size) {
      ASSERT(_hg.connectivity(he) == 1, V(_hg.connectivity(he)));
      ASSERT(pin_count_source_part_before_move == 1, V(pin_count_source_part_before_move));
      DBG << "he" << he << "is cut before applying move and uncut after";
      // Update pin of a HE that is removed from the cut.
      for (const PartitionID& part : _gain_cache.adjacentParts(pin)) {
        if (part != to_part) {
          if (update_cache_only) {
            _gain_cache.updateExistingEntry(pin, part, -he_weight);
          } else {
            updatePin(pin, part, he, -he_weight);
          }
        }
      }
    }
    if (pin_count_target_part_after_move == he_size - 1) {
      DBG << he << ": Only one vertex remains outside of to_part after applying the move";
      if (_hg.partID(pin) != to_part) {
        // Update single pin that remains outside of to_part after applying the move
        if (update_cache_only) {
          if (_already_processed_part.get(pin) != to_part) {
            _gain_cache.updateEntryIfItExists(pin, to_part, he_weight);
          }
        } else {
          updatePin(pin, to_part, he, he_weight);
        }
      }
    }

    if (pin_count_source_part_before_move == he_size - 1) {
      DBG << he << ": Only one vertex outside from_part before applying move";
      if (_hg.partID(pin) != from_part) {
        if (update_cache_only) {
          _gain_cache.updateEntryIfItExists(pin, from_part, -he_weight);
        } else {
          // Update single pin that was outside from_part before applying the move.
          updatePin(pin, from_part, he, -he_weight);
        }
      }
    }
  }

  template <bool only_update_cache = false>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void connectivityUpdate(const HypernodeID pin, const PartitionID from_part,
                                                          const PartitionID to_part, const HyperedgeID he,
                                                          const bool move_decreased_connectivity,
                                                          const bool move_increased_connectivity) {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _gain_cache.entryExists(pin, from_part) &&
        !Base::hypernodeIsConnectedToPart(pin, from_part)) {
      if (!only_update_cache && likely(!_hg.isFixedVertex(pin))) {
        _pq.remove(pin, from_part);
      }
      // LOG << "normal connectivity decrease for" << pin << V(from_part);
      _gain_cache.removeEntryDueToConnectivityDecrease(pin, from_part);
      // Now pq might actually not contain any moves for HN pin.
      // We do not need to set _active to false however, because in this case
      // the move not only decreased but also increased the connectivity and we
      // therefore add a new move to to_part in the next if-condition.
      // This resembled the case in which all but one incident HEs of HN pin are
      // internal and the "other" pin of the border HE (which has size 2) is
      // moved from one part to another.
    }
    if (move_increased_connectivity && !_gain_cache.entryExists(pin, to_part)) {
      ASSERT(_hg.connectivity(he) >= 2, V(_hg.connectivity(he)));
      ASSERT(_already_processed_part.get(pin) == Hypergraph::kInvalidPartition,
             V(_already_processed_part.get(pin)));
      Gain gain = GainCache::kNotCached;
      if (_gain_cache.entryExists(pin, to_part)) {
        gain = _gain_cache.entry(pin, to_part);
        ASSERT(gain == gainInducedByHypergraph(pin, to_part),
               V(pin) << V(gain) << V(gainInducedByHypergraph(pin, to_part)));
      } else {
        gain = gainInducedByHypergraph(pin, to_part);
        _gain_cache.addEntryDueToConnectivityIncrease(pin, to_part, gain);
      }
      // LOG << "normal connectivity increase for" << pin << V(to_part);
      if (!only_update_cache && likely(!_hg.isFixedVertex(pin))) {
        _pq.insert(pin, to_part, gain);
        if (_hg.partWeight(to_part) < _context.partition.max_part_weights[0]) {
          _pq.enablePart(to_part);
        }
      }
      _already_processed_part.set(pin, to_part);
    }
    ASSERT((_pq.contains(pin) && _hg.active(pin)) || _hg.isFixedVertex(pin), V(pin));
  }

  void connectivityUpdateForCache(const HypernodeID pin, const PartitionID from_part,
                                  const PartitionID to_part, const HyperedgeID he,
                                  const bool move_decreased_connectivity,
                                  const bool move_increased_connectivity)
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _gain_cache.entryExists(pin, from_part) &&
        !Base::hypernodeIsConnectedToPart(pin, from_part)) {
      DBGC(hn_to_debug == pin) << "removing cache entry for HN" << pin << "part=" << from_part;
      _gain_cache.removeEntryDueToConnectivityDecrease(pin, from_part);
    }
    if (move_increased_connectivity && !_gain_cache.entryExists(pin, to_part)) {
      ASSERT(_hg.connectivity(he) >= 2, V(_hg.connectivity(he)));
      DBGC(hn_to_debug == pin) << "adding cache entry for HN" << pin << "part="
                               << to_part << "gain=" <<
        V(gainInducedByHypergraph(pin, to_part));
      _gain_cache.addEntryDueToConnectivityIncrease(pin, to_part,
                                                    gainInducedByHypergraph(pin, to_part));
      _already_processed_part.set(pin, to_part);
    }
  }

  // Full update includes:
  // 1.) Activation of new border HNs
  // 2.) Removal of new non-border HNs
  // 3.) Connectivity update
  // 4.) Delta-Gain Update
  // This is used for the state transitions: free -> loose and loose -> locked
  template <bool only_update_cache = false>
  void fullUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                  const PartitionID to_part, const HyperedgeID he) {
    ONLYDEBUG(moved_hn);
    const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_target_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
    const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;

    if (!_he_fully_active[he] ||
        moveAffectsGainOrConnectivityUpdate(pin_count_target_part_before_move,
                                            pin_count_source_part_after_move,
                                            _hg.edgeSize(he))) {
      const HypernodeID pin_count_target_part_after_move = pin_count_target_part_before_move + 1;
      const bool move_decreased_connectivity = pin_count_source_part_after_move == 0 &&
                                               pin_count_source_part_before_move != 0;
      const bool move_increased_connectivity = pin_count_target_part_after_move == 1;

      const HypernodeID he_size = _hg.edgeSize(he);
      HypernodeID num_active_pins = 0;
      for (const HypernodeID& pin : _hg.pins(he)) {
        if (!only_update_cache && !_hg.marked(pin)) {
          ASSERT(pin != moved_hn, V(pin));
          if (!_hg.active(pin) && likely(!_hg.isFixedVertex(pin))) {
            _hns_to_activate.push_back(pin);
            ++num_active_pins;
          } else {
            if (!_hg.isBorderNode(pin)) {
              Base::removeHypernodeMovementsFromPQ(pin, _gain_cache);
            } else {
              connectivityUpdate(pin, from_part, to_part, he,
                                 move_decreased_connectivity,
                                 move_increased_connectivity);
              deltaGainUpdatesForPQandCache(pin, from_part, to_part, he, he_size, _hg.edgeWeight(he),
                                            pin_count_source_part_before_move,
                                            pin_count_target_part_after_move);
              num_active_pins += _hg.marked(pin) || _hg.active(pin);
              continue;
            }
          }
        }
        if (pin != moved_hn) {
          connectivityUpdateForCache(pin, from_part, to_part, he,
                                     move_decreased_connectivity,
                                     move_increased_connectivity);
          deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_size, _hg.edgeWeight(he),
                                       pin_count_source_part_before_move,
                                       pin_count_target_part_after_move);
        }
        num_active_pins += _hg.marked(pin) || _hg.active(pin);
      }
      _he_fully_active.set(he, (num_active_pins == he_size));
    }
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void updateNeighboursGainCacheOnly(const HypernodeID moved_hn,
                                                                     const PartitionID from_part,
                                                                     const PartitionID to_part) {
    updateNeighbours<true>(moved_hn, from_part, to_part);
  }


  // HEs remaining loose won't lead to new activations
  template <bool only_update_cache = false>
  void connectivityAndDeltaGainUpdateForHEsRemainingLoose(const HypernodeID moved_hn,
                                                          const PartitionID from_part,
                                                          const PartitionID to_part,
                                                          const HyperedgeID he) {
    ONLYDEBUG(moved_hn);
    const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;
    const HypernodeID pin_count_target_part_before_move = _hg.pinCountInPart(he, to_part) - 1;

    if (moveAffectsGainOrConnectivityUpdate(pin_count_target_part_before_move,
                                            pin_count_source_part_after_move,
                                            _hg.edgeSize(he))) {
      const HypernodeID pin_count_target_part_after_move = pin_count_target_part_before_move + 1;
      const bool move_decreased_connectivity = pin_count_source_part_after_move == 0;
      const bool move_increased_connectivity = pin_count_target_part_after_move == 1;

      for (const HypernodeID& pin : _hg.pins(he)) {
        if (!only_update_cache && !_hg.marked(pin)) {
          ASSERT(pin != moved_hn, V(pin));
          if (!_hg.isBorderNode(pin)) {
            Base::removeHypernodeMovementsFromPQ(pin, _gain_cache);
          } else {
            connectivityUpdate(pin, from_part, to_part, he,
                               move_decreased_connectivity,
                               move_increased_connectivity);
            deltaGainUpdatesForPQandCache(pin, from_part, to_part, he, _hg.edgeSize(he),
                                          _hg.edgeWeight(he),
                                          pin_count_source_part_before_move,
                                          pin_count_target_part_after_move);
            continue;
          }
        }
        if (pin != moved_hn) {
          connectivityUpdateForCache(pin, from_part, to_part, he,
                                     move_decreased_connectivity,
                                     move_increased_connectivity);
          deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, _hg.edgeSize(he),
                                       _hg.edgeWeight(he),
                                       pin_count_source_part_before_move,
                                       pin_count_target_part_after_move);
        }
      }
    }
  }

  template <bool only_update_cache = false>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void connectivityUpdate(const HypernodeID moved_hn,
                                                          const PartitionID from_part,
                                                          const PartitionID to_part,
                                                          const HyperedgeID he) {
    ONLYDEBUG(moved_hn);
    const HypernodeID he_size = _hg.edgeSize(he);
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);
    const HypernodeID pin_count_source_part_after_move = _hg.pinCountInPart(he, from_part);
    const HypernodeID pin_count_source_part_before_move = pin_count_source_part_after_move + 1;
    const HypernodeID pin_count_target_part_after_move = _hg.pinCountInPart(he, to_part);
    const HypernodeID pin_count_target_part_before_move = pin_count_target_part_after_move - 1;
    const bool move_decreased_connectivity = pin_count_source_part_after_move == 0;
    const bool move_increased_connectivity = pin_count_target_part_before_move == 0;

    if (move_decreased_connectivity || move_increased_connectivity) {
      for (const HypernodeID& pin : _hg.pins(he)) {
        if (!only_update_cache && !_hg.marked(pin)) {
          ASSERT(pin != moved_hn, V(pin));
          ASSERT(_hg.active(pin) || _hg.isFixedVertex(pin), V(pin));
          ASSERT(_hg.isBorderNode(pin), V(pin));
          connectivityUpdate(pin, from_part, to_part, he,
                             move_decreased_connectivity,
                             move_increased_connectivity);
          continue;
        }
        if (pin != moved_hn) {
          connectivityUpdateForCache(pin, from_part, to_part, he,
                                     move_decreased_connectivity,
                                     move_increased_connectivity);
          deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_size, he_weight,
                                       pin_count_source_part_before_move,
                                       pin_count_target_part_after_move);
        }
      }
    } else {
      for (const HypernodeID& pin : _hg.pins(he)) {
        if (pin != moved_hn) {
          deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_size, he_weight,
                                       pin_count_source_part_before_move,
                                       pin_count_target_part_after_move);
        }
      }
    }
  }


  template <bool only_update_cache = false>
  void updatePinsOfFreeHyperedgeBecomingLoose(const HypernodeID moved_hn,
                                              const PartitionID from_part,
                                              const PartitionID to_part,
                                              const HyperedgeID he) {
    //   ASSERT([&]() {
    //       // Only the moved_node is marked
    //       for (const HypernodeID& pin : _hg.pins(he)) {
    //         if (pin != moved_hn && _hg.marked(pin)) {
    //           return false;
    //         }
    //       }
    //       return true;
    //     } (), "Encountered a free HE with more than one marked pins.");

    fullUpdate<only_update_cache>(moved_hn, from_part, to_part, he);

    ASSERT([&]() {
        // all border nodes will be activate, except fixed vertices
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (!only_update_cache && !_hg.isFixedVertex(pin) &&
              (_hg.isBorderNode(pin) && !_hg.active(pin) && !_hg.marked(pin))) {
            if (std::find(_hns_to_activate.cbegin(), _hns_to_activate.cend(), pin) ==
                _hns_to_activate.cend()) {
              return false;
            }
          }
        }
        return true;
      } (), "Pins of HE" << he << "are not activated correctly");
  }

  template <bool only_update_cache = false>
  void updatePinsOfHyperedgeRemainingLoose(const HypernodeID moved_hn, const PartitionID from_part,
                                           const PartitionID to_part, const HyperedgeID he) {
    // ASSERT([&]() {
    //     // There is at least one marked pin whose partID = to_part and
    //     // no marked pin has a partID other than to_part
    //     bool valid = false;
    //     for (const HypernodeID& pin : _hg.pins(he)) {
    //       if (_hg.partID(pin) == to_part && _hg.marked(pin)) {
    //         valid = true;
    //       }
    //       if (_hg.partID(pin) != to_part && _hg.marked(pin)) {
    //         return false;
    //       }
    //     }
    //     return valid;
    //   } (), "");
    ASSERT([&]() {
        // Loose HEs remaining loose should have only active border HNs which are not fixed vertices
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (_hg.isBorderNode(pin) && !_hg.active(pin) && !_hg.marked(pin) && !_hg.isFixedVertex(pin) &&
              !only_update_cache) {
            return false;
          }
        }
        return true;
      } (), "");

    connectivityAndDeltaGainUpdateForHEsRemainingLoose<only_update_cache>(
      moved_hn, from_part, to_part, he);

    ASSERT([&]() {
        HypernodeID count = 0;
        for (const HypernodeID& pin : _hg.pins(he)) {
          // - All border HNs which are not fixed vertices are active
          // - At least two pins of the HE are marked
          // - No internal HNs have moves in PQ
          if (_hg.isBorderNode(pin) && !_hg.isFixedVertex(pin) &&
              !only_update_cache && !_hg.active(pin) && !_hg.marked(pin)) {
            return false;
          }
          if (_hg.marked(pin)) {
            ++count;
          }
          if (!_hg.isBorderNode(pin)) {
            for (PartitionID part = 0; part < _context.partition.k; ++part) {
              if (_pq.contains(pin, part)) {
                return false;
              }
            }
          }
        }
        return count >= 2;
      } (), " ");
  }


  template <bool only_update_cache = false>
  void updatePinsOfLooseHyperedgeBecomingLocked(const HypernodeID moved_hn,
                                                const PartitionID from_part,
                                                const PartitionID to_part,
                                                const HyperedgeID he) {
    fullUpdate<only_update_cache>(moved_hn, from_part, to_part, he);

    ASSERT([&]() {
        // If a HE becomes locked, the activation of its pins will definitely
        // happen because it not has to be a cut HE, except the pin is a fixed vertex
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (!only_update_cache &&
              !_hg.isFixedVertex(pin) &&
              (!_hg.active(pin) &&
               !_hg.marked(pin) &&
               std::find(_hns_to_activate.cbegin(), _hns_to_activate.cend(), pin) ==
               _hns_to_activate.cend())) {
            return false;
          }
        }
        return true;
      } (), "Loose HE" << he << "becomes locked, but not all pins are active");
  }

  template <bool only_update_cache = false>
  void updatePinsOfHyperedgeRemainingLocked(const HypernodeID moved_hn, const PartitionID from_part,
                                            const PartitionID to_part, const HyperedgeID he) {
    ASSERT([&]() {
        // All pins of a locked HE have to be active, except fixed vertices.
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (!_hg.active(pin) &&
              !_hg.marked(pin) &&
              !only_update_cache &&
              !_hg.isFixedVertex(pin)) {
            return false;
          }
        }
        return true;
      } (), "Loose HE" << he << "remains locked, but not all pins are active");

    connectivityUpdate<only_update_cache>(moved_hn, from_part, to_part, he);
  }


  template <bool only_update_cache = false>
  void updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part) {
    _already_processed_part.resetUsedEntries();

    bool moved_hn_remains_conntected_to_from_part = false;
    for (const HyperedgeID& he : _hg.incidentEdges(moved_hn)) {
      moved_hn_remains_conntected_to_from_part |= _hg.pinCountInPart(he, from_part) != 0;
      if (_locked_hes.get(he) != HEState::locked) {
        if (_locked_hes.get(he) == to_part) {
          // he is loose
          DBG << "HE" << he << "maintained state: loose";
          updatePinsOfHyperedgeRemainingLoose<only_update_cache>(moved_hn, from_part, to_part, he);
        } else if (_locked_hes.get(he) == HEState::free) {
          // he is free.
          DBG << "HE" << he << "changed state: free -> loose";
          updatePinsOfFreeHyperedgeBecomingLoose<only_update_cache>(moved_hn, from_part, to_part, he);
          _locked_hes.set(he, to_part);
        } else {
          // he is loose and becomes locked after the move
          DBG << "HE" << he << "changed state: loose -> locked";
          updatePinsOfLooseHyperedgeBecomingLocked<only_update_cache>(moved_hn, from_part, to_part, he);
          _locked_hes.uncheckedSet(he, HEState::locked);
        }
      } else {
        // he is locked
        DBG << he << "is locked";
        updatePinsOfHyperedgeRemainingLocked<only_update_cache>(moved_hn, from_part, to_part, he);
      }

      const HypernodeID pin_count_from_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
      const HypernodeID pin_count_to_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
      const HypernodeID pin_count_to_part_after_move = pin_count_to_part_before_move + 1;
      const HypernodeID he_size = _hg.edgeSize(he);
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      if (pin_count_from_part_before_move == he_size) {
        // Update pin of a HE that is not cut before applying the move.
        for (const PartitionID& part : _gain_cache.adjacentParts(moved_hn)) {
          if (part != from_part && part != to_part) {
            ASSERT(_already_processed_part.get(moved_hn) != part, "Argh");
            _gain_cache.updateExistingEntry(moved_hn, part, he_weight);
          }
        }
      }
      if (pin_count_to_part_after_move == he_size) {
        // Update pin of a HE that is removed from the cut.
        for (const PartitionID& part : _gain_cache.adjacentParts(moved_hn)) {
          if (part != to_part && part != from_part) {
            _gain_cache.updateExistingEntry(moved_hn, part, -he_weight);
          }
        }
      }
    }

    _gain_cache.updateFromAndToPartOfMovedHN(moved_hn, from_part, to_part,
                                             moved_hn_remains_conntected_to_from_part);

    // remove dups
    // TODO(schlag): fix this!!!
    for (const HypernodeID& hn : _hns_to_activate) {
      if (!_hg.active(hn) && likely(!_hg.isFixedVertex(hn))) {
        activate(hn);
      }
    }
    _hns_to_activate.clear();

    ASSERT([&]() {
        // This lambda checks verifies the internal state of KFM for all pins that could
        // have been touched during updateNeighbours.
        for (const HyperedgeID& he : _hg.incidentEdges(moved_hn)) {
          bool valid = true;
          for (const HypernodeID& pin : _hg.pins(he)) {
            ASSERT_THAT_CACHE_IS_VALID_FOR_HN(pin);

            if (!_hg.isBorderNode(pin)) {
              // The pin is an internal HN
              // there should not be any move of this HN in the PQ.
              for (PartitionID part = 0; part < _context.partition.k; ++part) {
                valid = (_pq.contains(pin, part) == false);
                if (!valid) {
                  LOG << "HN" << pin << "should not be contained in PQ";
                  return false;
                }
              }
            } else {
              // Pin is a border HN
              for (const PartitionID& part : _hg.connectivitySet(he)) {
                ASSERT(_hg.pinCountInPart(he, part) > 0, V(he) << "not connected to" << V(part));
                if (_pq.contains(pin, part)) {
                  // if the move to target.part is in the PQ, it has to have the correct gain
                  ASSERT(_hg.active(pin), "Pin is not active");
                  ASSERT(_hg.isBorderNode(pin), "BorderFail");
                  const Gain expected_gain = gainInducedByHypergraph(pin, part);
                  valid = (_pq.key(pin, part) == expected_gain);
                  if (!valid) {
                    LOG << "Incorrect maxGain for HN" << pin;
                    LOG << "expected key=" << expected_gain;
                    LOG << "actual key=" << _pq.key(pin, part);
                    LOG << "from_part=" << _hg.partID(pin);
                    LOG << "to part =" << part;
                    LOG << "_locked_hes[" << he << "]=" << _locked_hes.get(he);
                    return false;
                  }
                  if (_hg.partWeight(part) < _context.partition.max_part_weights[0] &&
                      !_pq.isEnabled(part)) {
                    LOG << V(pin);
                    LOG << "key=" << expected_gain;
                    LOG << "Part" << part << "should be enabled as target part";
                    return false;
                  }
                  if (_hg.partWeight(part) >= _context.partition.max_part_weights[0] &&
                      _pq.isEnabled(part)) {
                    LOG << V(pin);
                    LOG << "key=" << expected_gain;
                    LOG << "Part" << part << "should NOT be enabled as target part";
                    return false;
                  }
                } else {
                  // if it is not in the PQ then either the HN has already been marked as moved
                  // or we currently look at the source partition of pin or the pin is a fixed vertex
                  // or we are currently in cache-update-mode
                  valid = (_hg.marked(pin) == true) || (part == _hg.partID(pin) ||
                                                        unlikely(_hg.isFixedVertex(pin)) ||
                                                        only_update_cache);
                  if (!valid) {
                    LOG << "HN" << pin << "not in PQ but also not marked";
                    LOG << "gain=" << gainInducedByHypergraph(pin, part);
                    LOG << "from_part=" << _hg.partID(pin);
                    LOG << "to_part=" << part;
                    LOG << "would be feasible=" << Base::moveIsFeasible(pin, _hg.partID(pin), part);
                    LOG << "_locked_hes[" << he << "]=" << _locked_hes.get(he);
                    return false;
                  }
                  if (_hg.marked(pin)) {
                    // If the pin is already marked as moved, then all moves concerning this pin
                    // should have been removed from the PQ.
                    for (PartitionID part = 0; part < _context.partition.k; ++part) {
                      if (_pq.contains(pin, part)) {
                        LOG << "HN" << pin << "should not be contained in PQ,"
                            << "because it is already marked";
                        return false;
                      }
                    }
                  }
                }
              }
            }
            // Staleness check. If the PQ contains a move of pin to part, there
            // has to be at least one HE that connects to that part. Otherwise the
            // move is stale and should have been removed from the PQ.
            for (PartitionID part = 0; part < _context.partition.k; ++part) {
              bool connected = false;
              for (const HyperedgeID& incident_he : _hg.incidentEdges(pin)) {
                if (_hg.pinCountInPart(incident_he, part) > 0) {
                  connected = true;
                  break;
                }
              }
              if (!connected && _pq.contains(pin, part)) {
                LOG << "PQ contains stale move of HN" << pin << ":";
                LOG << "calculated gain=" << gainInducedByHypergraph(pin, part);
                LOG << "gain in PQ=" << _pq.key(pin, part);
                LOG << "from_part=" << _hg.partID(pin);
                LOG << "to_part=" << part;
                LOG << "would be feasible=" << Base::moveIsFeasible(pin, _hg.partID(pin), part);
                LOG << "current HN" << moved_hn << "was moved from" << from_part
                    << "to" << to_part;
                return false;
              }
            }
          }
        }
        return true;
      } (), V(moved_hn));
    ASSERT([&]() {
        for (const HypernodeID& hn : _hg.nodes()) {
          if (_hg.active(hn)) {
            bool valid = _hg.marked(hn) || !_hg.isBorderNode(hn);
            for (PartitionID part = 0; part < _context.partition.k; ++part) {
              if (_pq.contains(hn, part)) {
                valid = true;
                break;
              }
            }
            if (!valid) {
              LOG << V(hn) << "is active but neither marked nor in one of the PQs";
              return false;
            }
          }
        }
        return true;
      } (), V(moved_hn));
  }

  void updatePin(const HypernodeID pin, const PartitionID part, const HyperedgeID he,
                 const Gain delta)
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE {
    ONLYDEBUG(he);
    if (delta != 0 && _gain_cache.entryExists(pin, part) &&
        _already_processed_part.get(pin) != part) {
      if (likely(!_hg.isFixedVertex(pin))) {
        ASSERT(!_hg.marked(pin));
        ASSERT(_hg.active(pin));
        ASSERT(_hg.isBorderNode(pin));
        ASSERT((_hg.partWeight(part) < _context.partition.max_part_weights[0] ?
                _pq.isEnabled(part) : !_pq.isEnabled(part)), V(part));
        // Assert that we only perform delta-gain updates on moves that are not stale!
        ASSERT([&]() {
            for (const HyperedgeID& he : _hg.incidentEdges(pin)) {
              if (_hg.pinCountInPart(he, part) > 0) {
                return true;
              }
            }
            return false;
          } (), V(pin));

        DBG << "updating gain of HN" << pin
            << "from gain" << _pq.key(pin, part) << "to" << _pq.key(pin, part) + delta
            << "(to_part=" << part << ")";
        _pq.updateKeyBy(pin, part, delta);
      }
      _gain_cache.updateExistingEntry(pin, part, delta);
    }
  }

  template <bool invalidate_hn = false>
  void activate(const HypernodeID hn) {
    ASSERT(!_hg.active(hn), V(hn));
    ASSERT([&]() {
        for (PartitionID part = 0; part < _context.partition.k; ++part) {
          if (_pq.contains(hn, part)) {
            return false;
          }
        }
        return true;
      } (),
           "HN" << hn << "is already contained in PQ ");

    // Currently we cannot infer the gain changes of the two initial refinement nodes
    // from the uncontraction itself (this is still a todo). Therefore, these activations
    // have to invalidate and recalculate the gains.
    if (invalidate_hn) {
      _gain_cache.clear(hn);
      initializeGainCacheFor(hn);
    }

    if (_hg.isBorderNode(hn) && likely(!_hg.isFixedVertex(hn))) {
      insertHNintoPQ(hn);
      // mark HN as active for this round.
      _hg.activate(hn);
    }
  }

  Gain gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const {
    const PartitionID source_part = _hg.partID(hn);
    Gain gain = 0;
    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      if (_hg.connectivity(he) == 1) {
        gain -= _hg.edgeWeight(he);
      } else {
        const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, source_part);
        const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, target_part);
        if (pins_in_source_part == 1 && pins_in_target_part == _hg.edgeSize(he) - 1) {
          gain += _hg.edgeWeight(he);
        }
      }
    }
    return gain;
  }

  void initializeGainCache() {
    for (const HypernodeID& hn : _hg.nodes()) {
      initializeGainCacheFor(hn);
    }
  }

  void initializeGainCacheFor(const HypernodeID hn) {
    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal_weight = 0;

    _tmp_gains.clear();

    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      switch (_hg.connectivity(he)) {
        case 1:
          ASSERT(_hg.edgeSize(he) > 1, V(he));
          internal_weight += he_weight;
          break;
        case 2:
          for (const PartitionID& part : _hg.connectivitySet(he)) {
            _tmp_gains.add(part, 0);
            if (_hg.pinCountInPart(he, part) == _hg.edgeSize(he) - 1) {
              _tmp_gains[part] += he_weight;
            }
          }
          break;
        default:
          for (const PartitionID& part : _hg.connectivitySet(he)) {
            _tmp_gains.add(part, 0);
          }
          break;
      }
    }

    for (const auto& target_part : _tmp_gains) {
      if (target_part.key == source_part) {
        continue;
      }
      ASSERT(target_part.value - internal_weight == gainInducedByHypergraph(hn, target_part.key),
             V(gainInducedByHypergraph(hn, target_part.key))
             << V(target_part.value - internal_weight));
      _gain_cache.initializeEntry(hn, target_part.key, target_part.value - internal_weight);
    }
  }


  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE
  void insertHNintoPQ(const HypernodeID hn) {
    ASSERT(!_hg.marked(hn));
    ASSERT(_hg.isBorderNode(hn));

    if (likely(!_hg.isFixedVertex(hn))) {
      for (const PartitionID& part : _gain_cache.adjacentParts(hn)) {
        ASSERT(part != _hg.partID(hn), V(hn) << V(part) << V(_gain_cache.entry(hn, part)));
        ASSERT(_gain_cache.entry(hn, part) == gainInducedByHypergraph(hn, part),
               V(hn) << V(part) << V(_gain_cache.entry(hn, part)) <<
               V(gainInducedByHypergraph(hn, part)));
        ASSERT(Base::hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
        DBGC(hn == hn_to_debug) << "inserting" << V(hn) << V(part)
                                << V(_gain_cache.entry(hn, part));
        _pq.insert(hn, part, _gain_cache.entry(hn, part));
        if (_hg.partWeight(part) < _context.partition.max_part_weights[0]) {
          _pq.enablePart(part);
        }
      }
    }
  }

  void ASSERT_THAT_GAIN_CACHE_IS_VALID() {
    ASSERT([&]() {
        for (const HypernodeID& hn : _hg.nodes()) {
          ASSERT_THAT_CACHE_IS_VALID_FOR_HN(hn);
        }
        return true;
      } (), "Gain Cache inconsistent");
  }

  // TODO(schlag): Some of these assertions could easily be converted
  // into unit tests.
  void ASSERT_THAT_CACHE_IS_VALID_FOR_HN(const HypernodeID hn) const {
    if (_gain_cache.entryExists(hn)) {
      std::vector<bool> adjacent_parts(_context.partition.k, false);
      for (PartitionID part = 0; part < _context.partition.k; ++part) {
        if (Base::hypernodeIsConnectedToPart(hn, part)) {
          adjacent_parts[part] = true;
        }
        if (_gain_cache.entry(hn, part) != GainCache::kNotCached) {
          ASSERT(_gain_cache.entryExists(hn, part), V(hn) << V(part));
          ASSERT(_gain_cache.entry(hn, part) == gainInducedByHypergraph(hn, part),
                 V(hn) << V(part) << V(_gain_cache.entry(hn, part)) <<
                 V(gainInducedByHypergraph(hn, part)));
          ASSERT(Base::hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
        } else if (_hg.partID(hn) != part && !Base::hypernodeIsConnectedToPart(hn, part)) {
          ASSERT(!_gain_cache.entryExists(hn, part), V(hn) << V(part));
          ASSERT(_gain_cache.entry(hn, part) == GainCache::kNotCached, V(hn) << V(part));
        }
        if (_hg.partID(hn) == part) {
          ASSERT(!_gain_cache.entryExists(hn, part), V(hn) << V(part));
          ASSERT(_gain_cache.entry(hn, part) == GainCache::kNotCached,
                 V(hn) << V(part));
        }
      }
      for (const PartitionID& part : _gain_cache.adjacentParts(hn)) {
        ASSERT(adjacent_parts[part], V(part));
      }
    }
  }

  using Base::_hg;
  using Base::_context;
  using Base::_pq;
  using Base::_performed_moves;
  using Base::_hns_to_activate;

  ds::FastResetFlagArray<> _he_fully_active;
  ds::SparseMap<PartitionID, Gain> _tmp_gains;

  // After a move, we have to update the gains for all adjacent HNs.
  // For all moves of a HN that were already present in the PQ before the
  // the current max-gain move, this can be done via delta-gain update. However,
  // the current max-gain move might also have increased the connectivity for
  // a certain HN. In this case, we have to calculate the gain for this "new"
  // move from scratch and have to exclude it from all delta-gain updates in
  // the current updateNeighbours call. If we encounter such a new move,
  // we store the newly encountered part in this vector and do not perform
  // delta-gain updates for this part.
  ds::FastResetArray<PartitionID> _already_processed_part;

  ds::FastResetArray<PartitionID> _locked_hes;
  GainCache _gain_cache;
  StoppingPolicy _stopping_policy;
};
}  // namespace kahypar
