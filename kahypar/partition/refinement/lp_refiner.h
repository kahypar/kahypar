/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Vitali Henne <vitali.henne@gmail.com>
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
#include <unordered_set>
#include <utility>
#include <vector>

#include "kahypar/datastructure/fast_reset_flag_array.h"
#include "kahypar/datastructure/sparse_map.h"
#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/lp_gain_cache.h"
#include "kahypar/partition/refinement/policies/fm_improvement_policy.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
class LPRefiner final : public IRefiner {
 private:
  static constexpr bool debug = false;

  using GainPartitionPair = std::pair<Gain, PartitionID>;

  using GainCache = LPGainCache;
  using FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased;

 public:
  LPRefiner(Hypergraph& hg, const Context& contexturation) :
    _hg(hg),
    _context(contexturation),
    _cur_queue(),
    _next_queue(),
    _contained_cur_queue(hg.initialNumNodes()),
    _contained_next_queue(hg.initialNumNodes()),
    _max_score(),
    _tmp_connectivity_decrease(contexturation.partition.k, std::numeric_limits<PartitionID>::min()),
    _tmp_gains(contexturation.partition.k, { 0, 0 }),
    _gain_cache(_hg.initialNumNodes(), _context.partition.k),
    _already_processed_part(_hg.initialNumNodes(), Hypergraph::kInvalidPartition) {
    ASSERT(_context.partition.mode != Mode::direct_kway ||
           (_context.partition.max_part_weights[0] == _context.partition.max_part_weights[1]),
           "Lmax values should be equal for k-way partitioning");
    _max_score.reserve(contexturation.partition.k);
  }

  ~LPRefiner() override = default;

  LPRefiner(const LPRefiner&) = delete;
  LPRefiner& operator= (const LPRefiner&) = delete;

  LPRefiner(LPRefiner&&) = delete;
  LPRefiner& operator= (LPRefiner&&) = delete;

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>&,
                  const UncontractionGainChanges&,
                  Metrics& best_metrics) override final {
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg),
           V(best_metrics.cut) << V(metrics::hyperedgeCut(_hg)));
    ASSERT(FloatingPoint<double>(best_metrics.imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg, _context))),
           V(best_metrics.imbalance) << V(metrics::imbalance(_hg, _context)));

    _cur_queue.clear();
    _next_queue.clear();
    _contained_cur_queue.reset();
    _contained_next_queue.reset();

    const HyperedgeWeight initial_cut = best_metrics.cut;
    const double initial_imbalance = best_metrics.imbalance;
    HyperedgeWeight current_cut = best_metrics.cut;
    double current_imbalance = best_metrics.imbalance;
    PartitionID heaviest_part = heaviestPart();
    HypernodeWeight heaviest_part_weight = _hg.partWeight(heaviest_part);

    for (const HypernodeID& cur_node : refinement_nodes) {
      _gain_cache.clear(cur_node);
      initializeGainCacheFor(cur_node);
      if (!_contained_cur_queue[cur_node] && _hg.isBorderNode(cur_node)) {
        ASSERT(_hg.partWeight(_hg.partID(cur_node))
               <= _context.partition.max_part_weights[_hg.partID(cur_node) % 2],
               V(_hg.partWeight(_hg.partID(cur_node)))
               << V(_context.partition.max_part_weights[_hg.partID(cur_node) % 2]));
        _cur_queue.push_back(cur_node);
        _contained_cur_queue.set(cur_node, true);
      }
    }

    ASSERT_THAT_GAIN_CACHE_IS_VALID();

    for (int i = 0;
         (i == 0 || best_metrics.cut < current_cut || best_metrics.imbalance < current_imbalance) &&
         !_cur_queue.empty() && i < _context.local_search.sclap.max_number_iterations; ++i) {
      current_cut = best_metrics.cut;
      current_imbalance = best_metrics.imbalance;

      Randomize::instance().shuffleVector(_cur_queue, _cur_queue.size());
      for (const auto& hn : _cur_queue) {
        const auto& gain_pair = computeMaxGainMove(hn);
        const PartitionID from_part = _hg.partID(hn);
        const PartitionID to_part = gain_pair.second;

        DBG << "cut=" << best_metrics.cut << "max_gain_node=" << hn
            << "gain=" << gain_pair.first << "source_part=" << from_part
            << "target_part=" << to_part;

        const bool move_successful = moveHypernode(hn, from_part, gain_pair.second);
        if (move_successful) {
          reCalculateHeaviestPartAndItsWeight(heaviest_part, heaviest_part_weight,
                                              from_part, to_part);

          best_metrics.cut -= gain_pair.first;
          best_metrics.imbalance = static_cast<double>(heaviest_part_weight) /
                                   ceil(static_cast<double>(_context.partition.total_graph_weight) /
                                        _context.partition.k) - 1.0;

          ASSERT(_hg.partWeight(gain_pair.second)
                 <= _context.partition.max_part_weights[gain_pair.second % 2],
                 V(_hg.partWeight(gain_pair.second)));
          ASSERT(best_metrics.cut <= initial_cut, V(best_metrics.cut) << V(initial_cut));
          ASSERT(gain_pair.first >= 0, V(gain_pair.first));
          ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg), V(best_metrics.cut));
          ASSERT(best_metrics.imbalance == metrics::imbalance(_hg, _context),
                 V(best_metrics.imbalance) << V(metrics::imbalance(_hg, _context)));

          _already_processed_part.resetUsedEntries();
          bool moved_hn_remains_conntected_to_from_part = false;
          for (const auto& he : _hg.incidentEdges(hn)) {
            moved_hn_remains_conntected_to_from_part = moved_hn_remains_conntected_to_from_part ||
                                                       (_hg.pinCountInPart(he, from_part) != 0);
            const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
            const HypernodeID pin_count_target_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
            const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;
            const HypernodeID pin_count_target_part_after_move = pin_count_target_part_before_move + 1;

            const bool move_decreased_connectivity = pin_count_source_part_after_move == 0 &&
                                                     pin_count_source_part_before_move != 0;
            const bool move_increased_connectivity = pin_count_target_part_after_move == 1;

            const HypernodeID he_size = _hg.edgeSize(he);
            const HypernodeWeight he_weight = _hg.edgeWeight(he);

            for (const auto& pin : _hg.pins(he)) {
              if (pin != hn) {
                connectivityUpdate(pin, from_part, to_part, move_decreased_connectivity,
                                   move_increased_connectivity);
                deltaGainUpdate(pin, from_part, to_part, he, he_size, he_weight,
                                pin_count_source_part_before_move,
                                pin_count_target_part_after_move);
              }

              // add adjacent pins to next iteration
              if (!_contained_next_queue[pin] && _hg.isBorderNode(pin)) {
                _contained_next_queue.set(pin, true);
                _next_queue.push_back(pin);
              }
            }

            deltaGainUpdateForMovedHN(hn, from_part, to_part, he_size, he_weight,
                                      pin_count_source_part_before_move,
                                      pin_count_target_part_after_move);
          }
          _gain_cache.updateFromAndToPartOfMovedHN(hn, from_part, to_part,
                                                   moved_hn_remains_conntected_to_from_part);
        }
      }

      // clear the current queue data structures
      _contained_cur_queue.reset();
      _cur_queue.clear();

      _cur_queue.swap(_next_queue);
      _contained_cur_queue.swap(_contained_next_queue);
      // LOG << V(i);
      // LOG << V(best_metrics.cut) << "<" V(current_cut);
      // LOG << V(best_metrics.imbalance) << "<" V(initial_imbalance);
    }
    // LOG << "-----------------------------------";
    ASSERT_THAT_GAIN_CACHE_IS_VALID();

    return FMImprovementPolicy::improvementFound(best_metrics.cut, initial_cut,
                                                 best_metrics.imbalance,
                                                 initial_imbalance, _context.partition.epsilon);
  }

 private:
  PartitionID heaviestPart() const {
    PartitionID heaviest_part = 0;
    for (PartitionID part = 1; part < _context.partition.k; ++part) {
      if (_hg.partWeight(part) > _hg.partWeight(heaviest_part)) {
        heaviest_part = part;
      }
    }
    return heaviest_part;
  }

  void reCalculateHeaviestPartAndItsWeight(PartitionID& heaviest_part,
                                           HypernodeWeight& heaviest_part_weight,
                                           const PartitionID from_part,
                                           const PartitionID to_part) const {
    if (heaviest_part == from_part) {
      heaviest_part = heaviestPart();
      heaviest_part_weight = _hg.partWeight(heaviest_part);
    } else if (_hg.partWeight(to_part) > heaviest_part_weight) {
      heaviest_part = to_part;
      heaviest_part_weight = _hg.partWeight(to_part);
    }
    ASSERT([&]() {
        PartitionID heaviest = 0;
        HypernodeWeight max_weight = _hg.partWeight(heaviest);
        for (PartitionID part = 1; part < _context.partition.k; ++part) {
          if (_hg.partWeight(part) > max_weight) {
            heaviest = part;
            max_weight = _hg.partWeight(heaviest);
          }
        }
        if (max_weight != heaviest_part_weight) {
          return false;
        }
        return true;
      } (), "");
  }


  void connectivityUpdate(const HypernodeID pin, const PartitionID from_part,
                          const PartitionID to_part,
                          const bool move_decreased_connectivity,
                          const bool move_increased_connectivity) {
    if (move_decreased_connectivity && _gain_cache.entryExists(pin, from_part) &&
        !hypernodeIsConnectedToPart(pin, from_part)) {
      _gain_cache.removeEntryDueToConnectivityDecrease(pin, from_part);
    }
    if (move_increased_connectivity && !_gain_cache.entryExists(pin, to_part)) {
      _gain_cache.addEntryDueToConnectivityIncrease(pin, to_part,
                                                    { gainInducedByHypergraph(pin, to_part),
                                                      kM1gainInducedByHypergraph(pin, to_part) });
      _already_processed_part.set(pin, to_part);
    }
  }

  void deltaGainUpdateForMovedHN(const HypernodeID hn, const PartitionID from_part,
                                 const PartitionID to_part, const HypernodeID he_size,
                                 const HyperedgeWeight he_weight,
                                 const HypernodeID pin_count_source_part_before_move,
                                 const HypernodeID pin_count_target_part_after_move) {
    if (pin_count_source_part_before_move == he_size) {
      // Update pin of a HE that is not cut before applying the move.
      for (const PartitionID& part : _gain_cache.adjacentParts(hn)) {
        if (part != from_part && part != to_part) {
          ASSERT(_already_processed_part.get(hn) != part);
          _gain_cache.updateExistingEntry(hn, part, { he_weight, 0 });
        }
      }
    }
    if (pin_count_target_part_after_move == he_size) {
      // Update pin of a HE that is removed from the cut.
      for (const PartitionID& part : _gain_cache.adjacentParts(hn)) {
        if (part != to_part && part != from_part) {
          _gain_cache.updateExistingEntry(hn, part, { -he_weight, 0 });
        }
      }
    }


    // km1 update
    if (pin_count_source_part_before_move - 1 == 0 && pin_count_target_part_after_move != 1) {
      for (const PartitionID& part : _gain_cache.adjacentParts(hn)) {
        if (part != from_part && part != to_part) {
          _gain_cache.updateExistingEntry(hn, part, { 0, -he_weight });
        }
      }
    } else if (pin_count_source_part_before_move - 1 != 0 && pin_count_target_part_after_move == 1) {
      for (const PartitionID& part : _gain_cache.adjacentParts(hn)) {
        if (part != from_part && part != to_part) {
          _gain_cache.updateExistingEntry(hn, part, { 0, he_weight });
        }
      }
    }
  }


  void deltaGainUpdate(const HypernodeID pin, const PartitionID from_part,
                       const PartitionID to_part, const HyperedgeID he,
                       const HypernodeID he_size, const HyperedgeWeight he_weight,
                       const HypernodeID pin_count_source_part_before_move,
                       const HypernodeID pin_count_target_part_after_move) {
    ONLYDEBUG(he);
    if (pin_count_source_part_before_move == he_size) {
      ASSERT(_hg.connectivity(he) == 2, V(_hg.connectivity(he)));
      ASSERT(pin_count_target_part_after_move == 1, V(pin_count_target_part_after_move));
      for (const PartitionID& part : _gain_cache.adjacentParts(pin)) {
        if (part != from_part && _already_processed_part.get(pin) != part) {
          _gain_cache.updateExistingEntry(pin, part, { he_weight, 0 });
        }
      }
    }

    if (pin_count_target_part_after_move == he_size) {
      ASSERT(_hg.connectivity(he) == 1, V(_hg.connectivity(he)));
      ASSERT(pin_count_source_part_before_move == 1, V(pin_count_source_part_before_move));
      for (const PartitionID& part : _gain_cache.adjacentParts(pin)) {
        if (part != to_part) {
          _gain_cache.updateExistingEntry(pin, part, { -he_weight, 0 });
        }
      }
    }
    if (pin_count_target_part_after_move == he_size - 1 &&
        _hg.partID(pin) != to_part &&
        _already_processed_part.get(pin) != to_part) {
      // Update single pin that remains outside of to_part after applying the move
      _gain_cache.updateEntryIfItExists(pin, to_part, { he_weight, 0 });
    }

    if (pin_count_source_part_before_move == he_size - 1 && _hg.partID(pin) != from_part) {
      _gain_cache.updateEntryIfItExists(pin, from_part, { -he_weight, 0 });
    }

    // km1 updates
    const PartitionID source_part = _hg.partID(pin);
    if (source_part == from_part) {
      if (pin_count_source_part_before_move == 2) {
        for (const PartitionID& part : _gain_cache.adjacentParts(pin)) {
          if (_already_processed_part.get(pin) != part) {
            _gain_cache.updateExistingEntry(pin, part, { 0, he_weight });
          }
        }
      }
    } else if (source_part == to_part && pin_count_target_part_after_move == 2) {
      for (const PartitionID& part : _gain_cache.adjacentParts(pin)) {
        if (_already_processed_part.get(pin) != part) {
          _gain_cache.updateExistingEntry(pin, part, { 0, -he_weight });
        }
      }
    }

    if (pin_count_source_part_before_move == 1 && _gain_cache.entryExists(pin, from_part)) {
      _gain_cache.updateExistingEntry(pin, from_part, { 0, -he_weight });
    }

    if (pin_count_target_part_after_move == 1 && _already_processed_part.get(pin) != to_part) {
      _gain_cache.updateExistingEntry(pin, to_part, { 0, he_weight });
    }
  }

  void initializeImpl(const HyperedgeWeight) override final {
    if (!_is_initialized) {
      _is_initialized = true;
      _cur_queue.clear();
      _cur_queue.reserve(_hg.initialNumNodes());
      _next_queue.clear();
      _next_queue.reserve(_hg.initialNumNodes());
    }
    _gain_cache.clear();
    initializeGainCache();
  }

  void initializeGainCache() {
    for (const HypernodeID& hn : _hg.nodes()) {
      initializeGainCacheFor(hn);
    }
  }

  bool hypernodeIsConnectedToPart(const HypernodeID pin, const PartitionID part) const {
    for (const HyperedgeID& he : _hg.incidentEdges(pin)) {
      if (_hg.pinCountInPart(he, part) > 0) {
        return true;
      }
    }
    return false;
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

  void initializeGainCacheFor(const HypernodeID hn) {
    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal_weight = 0;
    HyperedgeWeight internal = 0;

    _tmp_gains.clear();

    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      internal += _hg.pinCountInPart(he, source_part) != 1 ? he_weight : 0;
      switch (_hg.connectivity(he)) {
        case 1:
          ASSERT(_hg.edgeSize(he) > 1, V(he));
          internal_weight += he_weight;
          break;
        case 2:
          for (const PartitionID& part : _hg.connectivitySet(he)) {
            _tmp_gains[part].km1 += he_weight;
            if (_hg.pinCountInPart(he, part) == _hg.edgeSize(he) - 1) {
              _tmp_gains[part].cut += he_weight;
            }
          }
          break;
        default:
          for (const PartitionID& part : _hg.connectivitySet(he)) {
            _tmp_gains[part].km1 += he_weight;
          }
          break;
      }
    }

    for (const auto& target_part : _tmp_gains) {
      if (target_part.key == source_part) {
        continue;
      }
      ASSERT(target_part.value.km1 - internal == kM1gainInducedByHypergraph(hn, target_part.key),
             "Km1 Gain calculation failed! Should be" <<
             V(kM1gainInducedByHypergraph(hn, target_part.key)));
      ASSERT(target_part.value.cut - internal_weight == gainInducedByHypergraph(hn, target_part.key),
             "Cut Gain calculation failed! Should be" << V
             (gainInducedByHypergraph(hn, target_part.key)));
      _gain_cache.initializeEntry(hn, target_part.key, { target_part.value.cut - internal_weight,
                                                         target_part.value.km1 - internal });
    }
  }


  Gain kM1gainInducedByHyperedge(const HypernodeID hn, const HyperedgeID he,
                                 const PartitionID target_part) const {
    const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, _hg.partID(hn));
    const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, target_part);
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);
    Gain gain = pins_in_source_part == 1 ? he_weight : 0;
    gain -= pins_in_target_part == 0 ? he_weight : 0;
    return gain;
  }

  Gain kM1gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const {
    ASSERT(target_part != _hg.partID(hn), V(hn) << V(target_part));
    Gain gain = 0;
    for (const HyperedgeID& he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      gain += kM1gainInducedByHyperedge(hn, he, target_part);
    }
    return gain;
  }

  void ASSERT_THAT_GAIN_CACHE_IS_VALID() {
    ASSERT([&]() {
        for (const HypernodeID& hn : _hg.nodes()) {
          ASSERT_THAT_CACHE_IS_VALID_FOR_HN(hn);
        }
        return true;
      } (), "Gain Cache inconsistent");
  }

  void ASSERT_THAT_CACHE_IS_VALID_FOR_HN(const HypernodeID hn) const {
    std::vector<bool> adjacent_parts(_context.partition.k, false);
    for (PartitionID part = 0; part < _context.partition.k; ++part) {
      if (hypernodeIsConnectedToPart(hn, part)) {
        adjacent_parts[part] = true;
      }
      if (_gain_cache.entry(hn, part).cut != GainCache::kNotCached) {
        ASSERT(_gain_cache.entryExists(hn, part), V(hn) << V(part));
        ASSERT(_gain_cache.entry(hn, part).cut == gainInducedByHypergraph(hn, part),
               V(hn) << V(part) << V(_gain_cache.entry(hn, part)) <<
               V(gainInducedByHypergraph(hn, part)));
        ASSERT(hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
        ASSERT(_gain_cache.entry(hn, part).km1 == kM1gainInducedByHypergraph(hn, part), V(hn)
               << V(part));
      } else if (_hg.partID(hn) != part && !hypernodeIsConnectedToPart(hn, part)) {
        ASSERT(!_gain_cache.entryExists(hn, part), V(hn) << V(part)
                                                         << "_hg.partID(hn) != part");
        ASSERT(_gain_cache.entry(hn, part).cut == GainCache::kNotCached, V(hn) << V(part));
        ASSERT(_gain_cache.entry(hn, part).km1 == GainCache::kNotCached, V(hn) << V(part));
      }
      if (_hg.partID(hn) == part) {
        ASSERT(!_gain_cache.entryExists(hn, part), V(hn) << V(part)
                                                         << "_hg.partID(hn) == part");
        ASSERT(_gain_cache.entry(hn, part).cut == GainCache::kNotCached,
               V(hn) << V(part));
        ASSERT(_gain_cache.entry(hn, part).km1 == GainCache::kNotCached,
               V(hn) << V(part));
      }
    }
#ifndef NDEBUG
    for (const PartitionID& part : _gain_cache.adjacentParts(hn)) {
      ASSERT(adjacent_parts[part], V(part));
    }
#endif
  }

  GainPartitionPair computeMaxGainMove(const HypernodeID& hn) {
    _max_score.clear();

    const PartitionID source_part = _hg.partID(hn);
    PartitionID max_gain_part = source_part;
    Gain max_gain = 0;
    Gain max_connectivity_decrease = 0;
    const HypernodeWeight node_weight = _hg.nodeWeight(hn);
    const bool source_part_imbalanced = _hg.partWeight(source_part) >
                                        _context.partition.max_part_weights[source_part % 2];


    _max_score.push_back(source_part);

    for (const PartitionID& target_part : _gain_cache.adjacentParts(hn)) {
      ASSERT(_gain_cache.entry(hn, target_part).cut == gainInducedByHypergraph(hn, target_part),
             V(hn) << V(target_part) << V(_gain_cache.entry(hn, target_part)) <<
             V(gainInducedByHypergraph(hn, target_part)));
      ASSERT(_gain_cache.entry(hn, target_part).km1 == kM1gainInducedByHypergraph(hn, target_part),
             V(hn) << V(target_part) << V(_gain_cache.entry(hn, target_part)) <<
             V(kM1gainInducedByHypergraph(hn, target_part)));
      ASSERT(hypernodeIsConnectedToPart(hn, target_part), V(hn) << V(target_part));
      const Gain target_part_gain = _gain_cache.entry(hn, target_part).cut;
      const Gain target_part_connectivity_decrease = _gain_cache.entry(hn, target_part).km1;
      const HypernodeWeight target_part_weight = _hg.partWeight(target_part);

      if (target_part_weight + node_weight <= _context.partition.max_part_weights[target_part % 2]) {
        if (target_part_gain == max_gain) {
          if (target_part_connectivity_decrease > max_connectivity_decrease) {
            max_connectivity_decrease = target_part_connectivity_decrease;
            _max_score.clear();
            _max_score.push_back(target_part);
          } else if (target_part_connectivity_decrease == max_connectivity_decrease) {
            _max_score.push_back(target_part);
          }
        } else if ((target_part_gain > max_gain) ||
                   (source_part_imbalanced && target_part_weight < _hg.partWeight(max_gain_part))) {
          _max_score.clear();
          max_gain = target_part_gain;
          _max_score.push_back(target_part);
        }
      }
    }

    max_gain_part = (max_gain > 0 || max_connectivity_decrease > 0 || source_part_imbalanced) ?
                    _max_score[(Randomize::instance().getRandomInt(0, _max_score.size() - 1))] : source_part;

    ASSERT(max_gain_part != Hypergraph::kInvalidPartition);

    return GainPartitionPair(max_gain, max_gain_part);
  }


  bool moveHypernode(const HypernodeID hn, const PartitionID from_part, const PartitionID to_part) {
    if (from_part == to_part) {
      return false;
    }

    ASSERT(_hg.partWeight(to_part) + _hg.nodeWeight(hn)
           <= _context.partition.max_part_weights[to_part % 2]);

    if (_hg.partSize(from_part) == 1) {
      // this would result in an extermination of a block
      return false;
    }

    _hg.changeNodePart(hn, from_part, to_part);
    return true;
  }

  Hypergraph& _hg;
  const Context& _context;
  std::vector<HypernodeID> _cur_queue;
  std::vector<HypernodeID> _next_queue;
  ds::FastResetFlagArray<> _contained_cur_queue;
  ds::FastResetFlagArray<> _contained_next_queue;

  std::vector<PartitionID> _max_score;
  std::vector<PartitionID> _tmp_connectivity_decrease;
  ds::SparseMap<PartitionID, LPGain> _tmp_gains;

  GainCache _gain_cache;
  // see KWayFMRefiner.h for documentation.
  ds::FastResetArray<PartitionID> _already_processed_part;
};
}  // namespace kahypar
