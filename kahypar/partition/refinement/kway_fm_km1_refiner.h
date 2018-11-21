/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Tobias Heuer <tobias.heuer@gmx.net>
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
class KWayKMinusOneRefiner final : public IRefiner,
                                   private FMRefinerBase<RollbackInfo,
                                                         KWayKMinusOneRefiner<StoppingPolicy,
                                                                              FMImprovementPolicy> >{
 private:
  static constexpr bool debug = false;
  static constexpr HypernodeID hn_to_debug = 5589;

  using GainCache = KwayGainCache<Gain>;
  using Base = FMRefinerBase<RollbackInfo, KWayKMinusOneRefiner<StoppingPolicy,
                                                                FMImprovementPolicy> >;

  friend class FMRefinerBase<RollbackInfo, KWayKMinusOneRefiner<StoppingPolicy,
                                                                FMImprovementPolicy> >;

  using HEState = typename Base::HEState;
  using Base::kInvalidGain;
  using Base::kInvalidHN;

  struct PinState {
    char one_pin_in_from_part_before : 1;
    char one_pin_in_to_part_after : 1;
    char two_pins_in_from_part_before : 1;
    char two_pins_in_to_part_after : 1;

    PinState(const bool one_in_from_before, const bool one_in_to_after,
             const bool two_in_from_before, const bool two_in_to_after) :
      one_pin_in_from_part_before(one_in_from_before),
      one_pin_in_to_part_after(one_in_to_after),
      two_pins_in_from_part_before(two_in_from_before),
      two_pins_in_to_part_after(two_in_to_after) { }
  };

 public:
  KWayKMinusOneRefiner(Hypergraph& hypergraph, const Context& context) :
    Base(hypergraph, context),
    _tmp_gains(_context.partition.k, 0),
    _new_adjacent_part(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
    _unremovable_he_parts(_hg.initialNumEdges() * context.partition.k),
    _gain_cache(_hg.initialNumNodes(), _context.partition.k),
    _stopping_policy() { }

  ~KWayKMinusOneRefiner() override = default;

  KWayKMinusOneRefiner(const KWayKMinusOneRefiner&) = delete;
  KWayKMinusOneRefiner& operator= (const KWayKMinusOneRefiner&) = delete;

  KWayKMinusOneRefiner(KWayKMinusOneRefiner&&) = delete;
  KWayKMinusOneRefiner& operator= (KWayKMinusOneRefiner&&) = delete;

 private:
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
    _unremovable_he_parts.reset();
    Base::performMovesAndUpdateCache(moves, refinement_nodes, changes);
  }


  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics& best_metrics) override final {
    // LOG << "=================================================";
    ASSERT(best_metrics.km1 == metrics::km1(_hg),
           V(best_metrics.km1) << V(metrics::km1(_hg)));
    ASSERT(FloatingPoint<double>(best_metrics.imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg, _context))),
           V(best_metrics.imbalance) << V(metrics::imbalance(_hg, _context)));

    Base::reset();
    _unremovable_he_parts.reset();

    Randomize::instance().shuffleVector(refinement_nodes, refinement_nodes.size());
    for (const HypernodeID& hn : refinement_nodes) {
      activate<true>(hn);
    }

    // Activate all adjacent free vertices of a fixed vertex in refinement_nodes
    Base::activateAdjacentFreeVertices(refinement_nodes);
    ASSERT_THAT_GAIN_CACHE_IS_VALID();

    const double initial_imbalance = best_metrics.imbalance;
    double current_imbalance = best_metrics.imbalance;
    const HyperedgeWeight initial_km1 = best_metrics.km1;
    HyperedgeWeight current_km1 = best_metrics.km1;

    int min_cut_index = -1;
    int touched_hns_since_last_improvement = 0;
    _stopping_policy.resetStatistics();

    const double beta = log(_hg.currentNumNodes());
    while (!_pq.empty() && !_stopping_policy.searchShouldStop(touched_hns_since_last_improvement,
                                                              _context, beta, best_metrics.km1,
                                                              current_km1)) {
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      const PartitionID from_part = _hg.partID(max_gain_node);

      DBG << V(current_km1) << V(max_gain_node) << V(max_gain)
          << V(_hg.partID(max_gain_node)) << V(to_part);

      ASSERT(!_hg.marked(max_gain_node), V(max_gain_node));
      ASSERT(max_gain == gainInducedByHypergraph(max_gain_node, to_part),
             V(max_gain) << "," << V(gainInducedByHypergraph(max_gain_node, to_part)));
      ASSERT(_hg.isBorderNode(max_gain_node), V(max_gain_node));
      ASSERT(Base::hypernodeIsConnectedToPart(max_gain_node, to_part),
             V(max_gain_node) << V(from_part) << V(to_part));

      // Active nodes cannot be incident only to HEs larger than the threshold, because these
      // should never be activated.
      ASSERT([&]() {
          for (const HyperedgeID& he : _hg.incidentEdges(max_gain_node)) {
            if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
              return true;
            }
          }
          return false;
        } (), "HE threshold violated for" << V(max_gain_node));

      // remove all other possible moves of the current max_gain_node
      for (const PartitionID& part : _gain_cache.adjacentParts(max_gain_node)) {
        if (part == to_part) {
          continue;
        }
        _pq.remove(max_gain_node, part);
      }

      _hg.mark(max_gain_node);
      ++touched_hns_since_last_improvement;

      if (Base::moveIsFeasible(max_gain_node, from_part, to_part)) {
        // LOG << "performed MOVE:" << V(max_gain_node) << V(from_part) << V(to_part);
        Base::moveHypernode(max_gain_node, from_part, to_part);

        if (_hg.partWeight(to_part) >= _context.partition.max_part_weights[0]) {
          _pq.disablePart(to_part);
        }
        if (_hg.partWeight(from_part) < _context.partition.max_part_weights[0]) {
          _pq.enablePart(from_part);
        }

        current_imbalance = metrics::imbalance(_hg, _context);

        current_km1 -= max_gain;
        _stopping_policy.updateStatistics(max_gain);

        ASSERT(current_km1 == metrics::km1(_hg),
               V(current_km1) << V(metrics::km1(_hg)));
        ASSERT(current_imbalance == metrics::imbalance(_hg, _context),
               V(current_imbalance) << V(metrics::imbalance(_hg, _context)));

        updateNeighbours(max_gain_node, from_part, to_part);

        // right now, we do not allow a decrease in cut in favor of an increase in balance
        const bool improved_km1_within_balance = (current_imbalance <= _context.partition.epsilon) &&
                                                 (current_km1 < best_metrics.km1);
        const bool improved_balance_less_equal_km1 = (current_imbalance < best_metrics.imbalance) &&
                                                     (current_km1 <= best_metrics.km1);

        if (improved_km1_within_balance || improved_balance_less_equal_km1) {
          DBGC(max_gain == 0) << "KWayFM improved balance between" << from_part
                              << "and" << to_part << "(max_gain=" << max_gain << ")";
          DBGC(current_km1 < best_metrics.km1) << "KWayFM improved cut from "
                                               << best_metrics.km1 << "to" << current_km1;
          best_metrics.km1 = current_km1;
          best_metrics.imbalance = current_imbalance;
          _stopping_policy.resetStatistics();
          min_cut_index = _performed_moves.size();
          touched_hns_since_last_improvement = 0;
          _gain_cache.resetDelta();
        }
        _performed_moves.emplace_back(RollbackInfo { max_gain_node, from_part, to_part });
      } else {
        // If the HN can't be moved to to_part, it locks all its incident
        // HEs in from_part (i.e., from_part becomes unremovable).
        // Unremovable parts are used to speed up updateNeighbors. Although
        // there aren't any gain-changes, we have to make sure that we activate
        // all remaining border nodes, since HEs with unremovable parts do not
        // automatically trigger new activations.
        for (const HyperedgeID& he : _hg.incidentEdges(max_gain_node)) {
          if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
            for (const HypernodeID& pin : _hg.pins(he)) {
              if (!_hg.marked(pin) && !_hg.active(pin) && _hg.isBorderNode(pin)) {
                activate(pin);
              }
            }
          }
          _unremovable_he_parts.set(static_cast<size_t>(he) * _context.partition.k + from_part, 1);
        }
      }
    }
    DBG << "KWayFM performed "
        << _performed_moves.size()
        << "local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (_stopping_policy.searchShouldStop(touched_hns_since_last_improvement, _context, beta,
                                          best_metrics.km1, current_km1)
        == true ? "policy" : "empty queue");

    Base::rollback(_performed_moves.size() - 1, min_cut_index);
    _gain_cache.rollbackDelta();

    ASSERT_THAT_GAIN_CACHE_IS_VALID();

    ASSERT(best_metrics.km1 == metrics::km1(_hg));
    ASSERT(best_metrics.km1 <= initial_km1, V(initial_km1) << V(best_metrics.km1));
    return FMImprovementPolicy::improvementFound(best_metrics.km1, initial_km1,
                                                 best_metrics.imbalance, initial_imbalance,
                                                 _context.partition.epsilon);
  }


  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void deltaGainUpdatesForCacheOnly(const HypernodeID pin,
                                                                    const PartitionID from_part,
                                                                    const PartitionID to_part,
                                                                    const HyperedgeID he,
                                                                    const HyperedgeWeight he_weight,
                                                                    const PinState pin_state) {
    deltaGainUpdates<false>(pin, from_part, to_part, he, he_weight, pin_state);
  }


  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void deltaGainUpdatesForPQandCache(const HypernodeID pin,
                                                                     const PartitionID from_part,
                                                                     const PartitionID to_part,
                                                                     const HyperedgeID he,
                                                                     const HyperedgeWeight he_weight,
                                                                     const PinState pin_state) {
    deltaGainUpdates<true>(pin, from_part, to_part, he, he_weight, pin_state);
  }


  template <bool update_pq = false>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void deltaGainUpdates(const HypernodeID pin,
                                                        const PartitionID from_part,
                                                        const PartitionID to_part,
                                                        const HyperedgeID he,
                                                        const HyperedgeWeight he_weight,
                                                        const PinState pin_state) {
    const PartitionID source_part = _hg.partID(pin);
    if (source_part == from_part) {
      if (pin_state.two_pins_in_from_part_before) {
        for (const PartitionID& part : _gain_cache.adjacentParts(pin)) {
          if (_new_adjacent_part.get(pin) != part) {
            if (update_pq) {
              updatePin(pin, part, he, he_weight);
            }
            _gain_cache.updateExistingEntry(pin, part, he_weight);
          }
        }
      }
    } else if (source_part == to_part && pin_state.two_pins_in_to_part_after) {
      for (const PartitionID& part : _gain_cache.adjacentParts(pin)) {
        if (_new_adjacent_part.get(pin) != part) {
          if (update_pq) {
            updatePin(pin, part, he, -he_weight);
          }
          _gain_cache.updateExistingEntry(pin, part, -he_weight);
        }
      }
    }

    if (pin_state.one_pin_in_from_part_before && _gain_cache.entryExists(pin, from_part)) {
      if (update_pq) {
        updatePin(pin, from_part, he, -he_weight);
      }
      _gain_cache.updateExistingEntry(pin, from_part, -he_weight);
    }

    if (pin_state.one_pin_in_to_part_after && _new_adjacent_part.get(pin) != to_part) {
      if (update_pq) {
        updatePin(pin, to_part, he, he_weight);
      }
      _gain_cache.updateExistingEntry(pin, to_part, he_weight);
    }
  }


  void connectivityUpdateForCache(const HypernodeID pin, const PartitionID from_part,
                                  const PartitionID to_part, const HyperedgeID he,
                                  const bool move_decreased_connectivity,
                                  const bool move_increased_connectivity) KAHYPAR_ATTRIBUTE_ALWAYS_INLINE {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _gain_cache.entryExists(pin, from_part) &&
        !Base::hypernodeIsConnectedToPart(pin, from_part)) {
      DBGC(hn_to_debug == pin) << "removing cache entry for HN" << pin << "part=" << from_part;
      _gain_cache.removeEntryDueToConnectivityDecrease(pin, from_part);
    }
    if (move_increased_connectivity && !_gain_cache.entryExists(pin, to_part)) {
      ASSERT(_hg.connectivity(he) >= 2, V(_hg.connectivity(he)));
      DBGC(hn_to_debug == 8498) << "adding cache entry for HN" << pin << "part="
                                << to_part << "gain=";
      _gain_cache.addEntryDueToConnectivityIncrease(pin, to_part,
                                                    gainInducedByHypergraph(pin, to_part));
      _new_adjacent_part.set(pin, to_part);
    }
  }


  void connectivityUpdate(const HypernodeID pin, const PartitionID from_part,
                          const PartitionID to_part, const HyperedgeID he,
                          const bool move_decreased_connectivity,
                          const bool move_increased_connectivity) KAHYPAR_ATTRIBUTE_ALWAYS_INLINE {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _gain_cache.entryExists(pin, from_part) &&
        !Base::hypernodeIsConnectedToPart(pin, from_part)) {
      _pq.remove(pin, from_part);
      _gain_cache.removeEntryDueToConnectivityDecrease(pin, from_part);
      // LOG << "normal connectivity decrease for" << pin;
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
      ASSERT(_new_adjacent_part.get(pin) == Hypergraph::kInvalidPartition,
             V(_new_adjacent_part.get(pin)));
      // LOG << "normal connectivity increase for" << pin << V(to_part);
      Gain gain = GainCache::kNotCached;
      if (_gain_cache.entryExists(pin, to_part)) {
        gain = _gain_cache.entry(pin, to_part);
        ASSERT(gain == gainInducedByHypergraph(pin, to_part),
               V(pin) << V(gain) << V(gainInducedByHypergraph(pin, to_part)));
      } else {
        gain = gainInducedByHypergraph(pin, to_part);
        _gain_cache.addEntryDueToConnectivityIncrease(pin, to_part, gain);
      }
      if (likely(!_hg.isFixedVertex(pin))) {
        _pq.insert(pin, to_part, gain);
        if (_hg.partWeight(to_part) < _context.partition.max_part_weights[0]) {
          _pq.enablePart(to_part);
        }
      }
      _new_adjacent_part.set(pin, to_part);
    }
    ASSERT(_pq.contains(pin) && _hg.active(pin), V(pin));
  }

  // Full update includes:
  // 1.) Activation of new border HNs
  // 2.) Removal of new non-border HNs
  // 3.) Connectivity update
  // 4.) Delta-Gain Update
  template <bool only_update_cache = false>
  void fullUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                  const PartitionID to_part, const HyperedgeID he) {
    ONLYDEBUG(moved_hn);
    const HypernodeID pin_count_from_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_to_part_after_move = _hg.pinCountInPart(he, to_part);
    const bool move_decreased_connectivity = pin_count_from_part_before_move - 1 == 0;
    const bool move_increased_connectivity = pin_count_to_part_after_move == 1;
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);

    const PinState pin_state(pin_count_from_part_before_move == 1,
                             pin_count_to_part_after_move == 1,
                             pin_count_from_part_before_move == 2,
                             pin_count_to_part_after_move == 2);

    for (const HypernodeID& pin : _hg.pins(he)) {
      // LOG << V(pin) << V(_hg.active(pin)) << V(_hg.isBorderNode(pin));
      if (!only_update_cache && !_hg.marked(pin)) {
        ASSERT(pin != moved_hn, V(pin));
        if (!_hg.active(pin)) {
          if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
            _hns_to_activate.push_back(pin);
          }
        } else {
          if (!_hg.isBorderNode(pin)) {
            Base::removeHypernodeMovementsFromPQ(pin, _gain_cache);
          } else {
            connectivityUpdate(pin, from_part, to_part, he,
                               move_decreased_connectivity,
                               move_increased_connectivity);
            deltaGainUpdatesForPQandCache(pin, from_part, to_part, he, he_weight,
                                          pin_state);
            continue;
          }
        }
      }
      // currently necessary because we set all cache entries of moved_hn to invalid -->
      // if we can do correct delta-gain updates for moved hn, than this if can be removed!
      if (pin != moved_hn) {
        connectivityUpdateForCache(pin, from_part, to_part, he,
                                   move_decreased_connectivity,
                                   move_increased_connectivity);
        deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_weight,
                                     pin_state);
      }
    }
  }


  // UR -> R and R -> UR
  template <bool only_update_cache = false>
  void updateForHEwithUnequalPartState(const HypernodeID moved_hn,
                                       const PartitionID from_part,
                                       const PartitionID to_part,
                                       const HyperedgeID he) {
    const HypernodeID pin_count_from_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_to_part_after_move = _hg.pinCountInPart(he, to_part);
    const HypernodeID pin_count_from_part_after_move = pin_count_from_part_before_move - 1;
    const bool move_decreased_connectivity = pin_count_from_part_before_move - 1 == 0;
    const bool move_increased_connectivity = pin_count_to_part_after_move == 1;
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);

    const PinState pin_state(pin_count_from_part_before_move == 1,
                             pin_count_to_part_after_move == 1,
                             pin_count_from_part_before_move == 2,
                             pin_count_to_part_after_move == 2);

    if (move_decreased_connectivity || move_increased_connectivity) {
      for (const HypernodeID& pin : _hg.pins(he)) {
        if (!only_update_cache && !_hg.marked(pin)) {
          ASSERT(pin != moved_hn, V(pin));
          if (move_decreased_connectivity && !_hg.isBorderNode(pin) &&
              _hg.active(pin)) {
            Base::removeHypernodeMovementsFromPQ(pin, _gain_cache);
          } else if (move_increased_connectivity && !_hg.active(pin)) {
            if (_hg.edgeSize(he) <= _context.partition.hyperedge_size_threshold) {
              _hns_to_activate.push_back(pin);
            }
          } else if (_hg.active(pin)) {
            connectivityUpdate(pin, from_part, to_part, he,
                               move_decreased_connectivity,
                               move_increased_connectivity);
            deltaGainUpdatesForPQandCache(pin, from_part, to_part, he, he_weight,
                                          pin_state);
            continue;
          }
        }
        // currently necessary because we set all cache entries of moved_hn to invalid -->
        // if we can do correct delta-gain updates for moved hn, than this if can be removed!
        if (pin != moved_hn) {
          connectivityUpdateForCache(pin, from_part, to_part, he,
                                     move_decreased_connectivity,
                                     move_increased_connectivity);
          deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_weight,
                                       pin_state);
        }
      }
    } else {
      if (pin_count_from_part_after_move == 1) {
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (_hg.partID(pin) == from_part) {
            if (_hg.active(pin)) {
              deltaGainUpdatesForPQandCache(pin, from_part, to_part, he, he_weight, pin_state);
            } else {
              deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_weight, pin_state);
            }
            break;
          }
        }
      }
      if (pin_count_to_part_after_move == 2) {
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (_hg.partID(pin) == to_part && pin != moved_hn) {
            if (_hg.active(pin)) {
              deltaGainUpdatesForPQandCache(pin, from_part, to_part, he, he_weight, pin_state);
            } else {
              deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_weight, pin_state);
            }
            break;
          }
        }
      }
    }
  }

  void updateForHEwithUnremovableFromAndToPart(const HypernodeID moved_hn,
                                               const PartitionID from_part,
                                               const PartitionID to_part,
                                               const HyperedgeID he) {
    const HypernodeID pin_count_from_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_to_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
    const HypernodeID pin_count_from_part_after_move = pin_count_from_part_before_move - 1;
    const HypernodeID pin_count_to_part_after_move = pin_count_to_part_before_move + 1;

    ASSERT(pin_count_from_part_after_move != 0, "move decreased connectivity");
    ASSERT(pin_count_to_part_after_move != 1, "move increased connectivity");

    if (pin_count_from_part_after_move == 1 || pin_count_to_part_after_move == 2) {
      const PinState pin_state(pin_count_from_part_before_move == 1,
                               pin_count_to_part_after_move == 1,
                               pin_count_from_part_before_move == 2,
                               pin_count_to_part_after_move == 2);
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);

      if (pin_count_from_part_after_move == 1) {
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (_hg.partID(pin) == from_part) {
            deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_weight, pin_state);
            break;
          }
        }
      }
      if (pin_count_to_part_after_move == 2) {
        for (const HypernodeID& pin : _hg.pins(he)) {
          if (_hg.partID(pin) == to_part && pin != moved_hn) {
            deltaGainUpdatesForCacheOnly(pin, from_part, to_part, he, he_weight, pin_state);
            break;
          }
        }
      }
    }
  }

  bool fromAndToPartAreUnremovable(const HyperedgeID he, const PartitionID from_part,
                                   const PartitionID to_part) const {
    return _unremovable_he_parts[static_cast<size_t>(he) * _context.partition.k + from_part] &&
           _unremovable_he_parts[static_cast<size_t>(he) * _context.partition.k + to_part];
  }

  bool fromAndToPartHaveUnequalStates(const HyperedgeID he, const PartitionID from_part,
                                      const PartitionID to_part) const {
    return (!_unremovable_he_parts[static_cast<size_t>(he) * _context.partition.k + from_part] &&
            _unremovable_he_parts[static_cast<size_t>(he) * _context.partition.k + to_part]) ||
           (_unremovable_he_parts[static_cast<size_t>(he) * _context.partition.k + from_part] &&
            !_unremovable_he_parts[static_cast<size_t>(he) * _context.partition.k + to_part]);
  }

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void updateNeighboursGainCacheOnly(const HypernodeID moved_hn,
                                                                     const PartitionID from_part,
                                                                     const PartitionID to_part) {
    updateNeighbours<true>(moved_hn, from_part, to_part);
  }

  template <bool only_update_cache = false>
  void updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part) {
    _new_adjacent_part.resetUsedEntries();

    bool moved_hn_remains_conntected_to_from_part = false;
    for (const HyperedgeID& he : _hg.incidentEdges(moved_hn)) {
      const HypernodeID pins_in_source_part_after = _hg.pinCountInPart(he, from_part);

      ASSERT(!_gain_cache.entryExists(moved_hn, from_part), V(moved_hn) << V(from_part));
      moved_hn_remains_conntected_to_from_part |= pins_in_source_part_after != 0;

      if (pins_in_source_part_after == 0 && _hg.pinCountInPart(he, to_part) != 1) {
        for (const PartitionID& part : _gain_cache.adjacentParts(moved_hn)) {
          if (part != from_part && part != to_part) {
            _gain_cache.updateExistingEntry(moved_hn, part, -_hg.edgeWeight(he));
          }
        }
      } else if (pins_in_source_part_after != 0 && _hg.pinCountInPart(he, to_part) == 1) {
        for (const PartitionID& part : _gain_cache.adjacentParts(moved_hn)) {
          if (part != from_part && part != to_part) {
            _gain_cache.updateExistingEntry(moved_hn, part, _hg.edgeWeight(he));
          }
        }
      }

      if (fromAndToPartAreUnremovable(he, from_part, to_part)) {
        updateForHEwithUnremovableFromAndToPart(moved_hn, from_part, to_part, he);
      } else if (fromAndToPartHaveUnequalStates(he, from_part, to_part)) {
        updateForHEwithUnequalPartState<only_update_cache>(moved_hn, from_part, to_part, he);
      } else {
        fullUpdate<only_update_cache>(moved_hn, from_part, to_part, he);
      }
      _unremovable_he_parts.set(static_cast<size_t>(he) * _context.partition.k + to_part, 1);

      ASSERT([&]() {
          // Search parts of hyperedge he which are unremoveable
          std::vector<bool> ur_parts(_context.partition.k, false);
          for (const HypernodeID& pin : _hg.pins(he)) {
            if (_hg.marked(pin)) {
              ur_parts[_hg.partID(pin)] = true;
            }
          }
          // _unremovable_he_parts should have the same bits set as ur_parts
          for (PartitionID k = 0; k < _context.partition.k; k++) {
            ASSERT(ur_parts[k] == _unremovable_he_parts[he * _context.partition.k + k],
                   V(ur_parts[k]) << V(_unremovable_he_parts[he * _context.partition.k + k]));
          }
          return true;
        } (), "Error in locking of he/parts!");
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
        if (only_update_cache) { return true; }
        // This lambda checks verifies the internal state of KFM for all pins that could
        // have been touched during updateNeighbours.
        for (const HyperedgeID& he : _hg.incidentEdges(moved_hn)) {
          bool valid = true;
          for (const HypernodeID& pin : _hg.pins(he)) {
            ASSERT_THAT_CACHE_IS_VALID_FOR_HN(pin);
            // LOG << "HN" << pin << "CHECK!";
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
                  valid = _pq.key(pin, part) == expected_gain;
                  if (!valid) {
                    LOG << "Incorrect maxGain for HN" << pin;
                    LOG << "expected key=" << expected_gain;
                    LOG << "actual key=" << _pq.key(pin, part);
                    LOG << "from_part=" << _hg.partID(pin);
                    LOG << "to part =" << part;
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
                  // or we currently look at the source partition of pin or pin is a fixed vertex
                  valid = (_hg.marked(pin) == true) || (part == _hg.partID(pin)) || unlikely(_hg.isFixedVertex(pin));
                  valid = valid || _hg.edgeSize(he) > _context.partition.hyperedge_size_threshold;
                  if (!valid) {
                    LOG << "HN" << pin << "not in PQ but also not marked";
                    LOG << "gain=" << gainInducedByHypergraph(pin, part);
                    LOG << "from_part=" << _hg.partID(pin);
                    LOG << "to_part=" << part;
                    LOG << "would be feasible=" << Base::moveIsFeasible(pin, _hg.partID(pin), part);
                    _hg.printNodeState(pin);
                    for (const HyperedgeID& incident_edge : _hg.incidentEdges(pin)) {
                      for (PartitionID i = 0; i < _context.partition.k; ++i) {
                        LOG << "Part" << i << "unremovable: "
                            << _unremovable_he_parts[he * _context.partition.k + i];
                      }
                      _hg.printEdgeState(incident_edge);
                    }
                    return false;
                  }
                  if (_hg.marked(pin)) {
                    // If the pin is already marked as moved, then all moves concerning this pin
                    // should have been removed from the PQ.
                    for (PartitionID part = 0; part < _context.partition.k; ++part) {
                      if (_pq.contains(pin, part)) {
                        LOG << "HN" << pin << "should not be in PQ, because it is already marked";
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
                LOG << "current HN" << moved_hn << "was moved from" << from_part << "to"
                    << to_part;
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

  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void updatePin(const HypernodeID pin, const PartitionID part,
                                                 const HyperedgeID he, const Gain delta) {
    ONLYDEBUG(he);
    if (likely(!_hg.isFixedVertex(pin))) {
      ASSERT(_gain_cache.entryExists(pin, part), V(pin) << V(part));
      ASSERT(_new_adjacent_part.get(pin) != part, V(pin) << V(part));
      ASSERT(!_hg.marked(pin));
      ASSERT(_hg.active(pin));
      ASSERT(_hg.isBorderNode(pin));
      ASSERT((_hg.partWeight(part) < _context.partition.max_part_weights[0] ?
              _pq.isEnabled(part) : !_pq.isEnabled(part)), V(part));
      // Assert that we only perform delta-gain updates on moves that are not stale!
      ASSERT(Base::hypernodeIsConnectedToPart(pin, part), V(pin) << V(part));
      ASSERT(_hg.partID(pin) != part, V(pin) << V(part));

      DBG << "updating gain of HN" << pin
          << "from gain" << _pq.key(pin, part) << "to" << _pq.key(pin, part) + delta
          << "(to_part=" << part << ", ExpectedGain="
          << gainInducedByHypergraph(pin, part) << ")";
      _pq.updateKeyBy(pin, part, delta);
    }
  }

  template <bool invalidate_hn = false>
  void activate(const HypernodeID hn) {
    ASSERT(!_hg.active(hn), V(hn));
    ASSERT([&]() {
        for (PartitionID part = 0; part < _context.partition.k; ++part) {
          ASSERT(!_pq.contains(hn, part), V(hn) << V(part));
        }
        return true;
      } (), "HN" << hn << "is already contained in PQ ");

    // Currently we cannot infer the gain changes of the two initial refinement nodes
    // from the uncontraction itself (this is still a todo). Therefore, these activations
    // have to invalidate and recalculate the gains.
    if (invalidate_hn) {
      _gain_cache.clear(hn);
      initializeGainCacheFor(hn);
    }
    if (_hg.isBorderNode(hn) && likely(!_hg.isFixedVertex(hn))) {
      ASSERT(!_hg.active(hn), V(hn));
      ASSERT(!_hg.marked(hn), "Hypernode" << hn << "is already marked");
      insertHNintoPQ(hn);
      // mark HN as active for this round.
      _hg.activate(hn);
    }
  }

  Gain gainInducedByHyperedge(const HypernodeID hn, const HyperedgeID he,
                              const PartitionID target_part) const {
    const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, _hg.partID(hn));
    const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, target_part);
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);
    Gain gain = pins_in_source_part == 1 ? he_weight : 0;
    gain -= pins_in_target_part == 0 ? he_weight : 0;
    return gain;
  }

  Gain gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const {
    ASSERT(target_part != _hg.partID(hn), V(hn) << V(target_part));
    Gain gain = 0;
    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      gain += gainInducedByHyperedge(hn, he, target_part);
    }
    return gain;
  }

  void initializeGainCache() {
    for (const HypernodeID& hn : _hg.nodes()) {
      initializeGainCacheFor(hn);
    }
  }

  void initializeGainCacheFor(const HypernodeID hn) {
    _tmp_gains.clear();
    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal = 0;
    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      internal += _hg.pinCountInPart(he, source_part) != 1 ? he_weight : 0;
      for (const PartitionID& part : _hg.connectivitySet(he)) {
        _tmp_gains[part] += he_weight;
      }
    }

    for (const auto& target_part : _tmp_gains) {
      if (target_part.key == source_part) {
        ASSERT(!_gain_cache.entryExists(hn, source_part), V(hn) << V(source_part));
        continue;
      }
      ASSERT(target_part.value - internal == gainInducedByHypergraph(hn, target_part.key),
             V(gainInducedByHypergraph(hn, target_part.key)) << V(target_part.value - internal));
      DBGC(hn == hn_to_debug) << V(target_part.key) << V(target_part.value - internal);
      _gain_cache.initializeEntry(hn, target_part.key, target_part.value - internal);
    }
  }


  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void insertHNintoPQ(const HypernodeID hn) {
    ASSERT(_hg.isBorderNode(hn));

    if (likely(!_hg.isFixedVertex(hn))) {
      for (const PartitionID& part : _gain_cache.adjacentParts(hn)) {
        ASSERT(part != _hg.partID(hn), V(hn) << V(part) << V(_gain_cache.entry(hn, part)));
        ASSERT(_gain_cache.entry(hn, part) == gainInducedByHypergraph(hn, part),
               V(hn) << V(part) << V(_gain_cache.entry(hn, part)) <<
               V(gainInducedByHypergraph(hn, part)));
        ASSERT(Base::hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
        DBGC(hn == 12518) << "inserting" << V(hn) << V(part) << V(_gain_cache.entry(hn, part));
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
                 V(gainInducedByHypergraph(hn, part)) << V(_hg.partID(hn)));
          ASSERT(Base::hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
        } else if (_hg.partID(hn) != part && !Base::hypernodeIsConnectedToPart(hn, part)) {
          ASSERT(!_gain_cache.entryExists(hn, part), V(hn) << V(part)
                                                           << "_hg.partID(hn) != part");
          ASSERT(_gain_cache.entry(hn, part) == GainCache::kNotCached, V(hn) << V(part));
        }
        if (_hg.partID(hn) == part) {
          ASSERT(!_gain_cache.entryExists(hn, part), V(hn) << V(part)
                                                           << "_hg.partID(hn) == part");
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
  ds::FastResetArray<PartitionID> _new_adjacent_part;

  // 'Locking' of hyperedges for K-1 metric. When optimizing this metric,
  // each part of a hyperedge becomes unremovable, as soon as one of its
  // pins is moved to that part. For each HE e, this bitvector stores whether
  // or not a part in the connectivity set of e is unremovable.
  ds::FastResetFlagArray<> _unremovable_he_parts;

  GainCache _gain_cache;
  StoppingPolicy _stopping_policy;
};
}  // namespace kahypar
