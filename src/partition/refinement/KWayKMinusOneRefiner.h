/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 *  Copyright (C) 2016 Tobias Heuer <tobias.heuer@gmx.net>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_KWAYKMINUSONEREFINER_H_
#define SRC_PARTITION_REFINEMENT_KWAYKMINUSONEREFINER_H_

#include <limits>
#include <stack>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "gtest/gtest_prod.h"

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/FastResetVector.h"
#include "lib/datastructure/InsertOnlyConnectivitySet.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/KwayGainCache.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "tools/RandomFunctions.h"

using datastructure::KWayPriorityQueue;
using datastructure::FastResetVector;
using datastructure::FastResetBitVector;
using datastructure::InsertOnlyConnectivitySet;

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;

namespace partition {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <class StoppingPolicy = Mandatory,
          // does nothing for KFM
          bool global_rebalancing = false,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class KWayKMinusOneRefiner final : public IRefiner,
                                   private FMRefinerBase {
  static const bool dbg_refinement_kway_kminusone_fm_activation = false;
  static const bool dbg_refinement_kway_kminusone_fm_improvements_cut = false;
  static const bool dbg_refinement_kway_kminusone_fm_improvements_balance = false;
  static const bool dbg_refinement_kway_kminusone_fm_stopping_crit = false;
  static const bool dbg_refinement_kway_kminusone_fm_gain_update = false;
  static const bool dbg_refinement_kway_kminusone_fm_gain_comp = false;
  static const bool dbg_refinement_kway_kminusone_infeasible_moves = false;
  static const bool dbg_refinement_kway_kminusone_gain_caching = false;
  static const HypernodeID hn_to_debug = 5589;

  using Gain = HyperedgeWeight;
  using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, Gain,
                                             std::numeric_limits<Gain> >;
  using GainCache = KwayGainCache<HypernodeID, PartitionID, Gain>;

  struct RollbackInfo {
    HypernodeID hn;
    PartitionID from_part;
    PartitionID to_part;
  };

 public:
  KWayKMinusOneRefiner(Hypergraph& hypergraph, const Configuration& config) noexcept :
    FMRefinerBase(hypergraph, config),
    _pq_contains(_hg.initialNumNodes() * _config.partition.k, false),
    _tmp_gains(_config.partition.k, 0),
    _tmp_target_parts(_config.partition.k),
    _performed_moves(),
    _hns_to_activate(),
    _already_processed_part(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
    _pq(_config.partition.k),
    _gain_cache(_hg.initialNumNodes(), _config.partition.k),
    _stopping_policy() {
    _performed_moves.reserve(_hg.initialNumNodes());
    _hns_to_activate.reserve(_hg.initialNumNodes());
  }

  virtual ~KWayKMinusOneRefiner() { }

  KWayKMinusOneRefiner(const KWayKMinusOneRefiner&) = delete;
  KWayKMinusOneRefiner& operator= (const KWayKMinusOneRefiner&) = delete;

  KWayKMinusOneRefiner(KWayKMinusOneRefiner&&) = delete;
  KWayKMinusOneRefiner& operator= (KWayKMinusOneRefiner&&) = delete;

 private:
#ifdef USE_BUCKET_PQ
  void initializeImpl(const HyperedgeWeight max_gain) noexcept override final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes(), max_gain);
      _is_initialized = true;
      initializeGainCache();
    }
  }
#else
  void initializeImpl() noexcept override final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes());
      _is_initialized = true;
      initializeGainCache();
    }
  }
#endif

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                  const UncontractionGainChanges& UNUSED(changes),
                  Metrics& best_metrics) noexcept override final {
    // LOG("=================================================");
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg),
           "initial best_metrics.cut " << best_metrics.cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_metrics.imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg, _config))),
           "initial best_metrics.imbalance " << best_metrics.imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg, _config));

    _pq.clear();
    _hg.resetHypernodeState();
    _pq_contains.resetAllBitsToFalse();

    Randomize::shuffleVector(refinement_nodes, refinement_nodes.size());
    for (const HypernodeID hn : refinement_nodes) {
      // TODO(schlag): can we infer these gains like in 2FM
      // instead of recomputing them?
      // true here indicates that the gain_cache has to be invalidated;
      activate<true>(hn, max_allowed_part_weights[0]);
    }

    ASSERT_THAT_GAIN_CACHE_IS_VALID();

    const double initial_imbalance = best_metrics.imbalance;
    HyperedgeWeight current_cut = best_metrics.cut;
    double current_imbalance = best_metrics.imbalance;
    const HyperedgeWeight initial_kminusone = best_metrics.km1;
    HyperedgeWeight current_kminusone = best_metrics.km1;

    PartitionID heaviest_part = heaviestPart();
    HypernodeWeight heaviest_part_weight = _hg.partWeight(heaviest_part);

    int min_cut_index = -1;
    int num_moves = 0;
    int num_moves_since_last_improvement = 0;
    _stopping_policy.resetStatistics();

    const double beta = log(_hg.numNodes());
    while (!_pq.empty() && !_stopping_policy.searchShouldStop(num_moves_since_last_improvement,
                                                              _config, beta, best_metrics.cut,
                                                              current_cut)) {
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      _pq_contains.setBit(max_gain_node * _config.partition.k + to_part, false);
      const PartitionID from_part = _hg.partID(max_gain_node);

      DBG(false, "cut=" << current_cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << _hg.partID(max_gain_node)
          << " target_part=" << to_part);

      ASSERT(!_hg.marked(max_gain_node),
             "HN " << max_gain_node << "is marked and not eligable to be moved");
      ASSERT(max_gain == gainInducedByHypergraph(max_gain_node, to_part),
             V(max_gain) << ", " << V(gainInducedByHypergraph(max_gain_node, to_part)));
      ASSERT(_hg.isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");


      // Staleness assertion: The move should be to a part that is in the connectivity superset of
      // the max_gain_node.
      ASSERT(hypernodeIsConnectedToPart(max_gain_node, to_part),
             "Move of HN " << max_gain_node << " from " << from_part
             << " to " << to_part << " is stale!");

      moveHypernode(max_gain_node, from_part, to_part);
      _hg.mark(max_gain_node);

      if (_hg.partWeight(to_part) >= max_allowed_part_weights[0]) {
        _pq.disablePart(to_part);
      }
      if (_hg.partWeight(from_part) < max_allowed_part_weights[0]) {
        _pq.enablePart(from_part);
      }

      reCalculateHeaviestPartAndItsWeight(heaviest_part, heaviest_part_weight,
                                          from_part, to_part);

      current_imbalance = static_cast<double>(heaviest_part_weight) /
                          ceil(static_cast<double>(_config.partition.total_graph_weight) /
                               _config.partition.k) - 1.0;

      // remove all other possible moves of the current max_gain_node
      for (PartitionID part = 0; part < _config.partition.k; ++part) {
        if (_pq_contains[max_gain_node * _config.partition.k + part]) {
          _pq.remove(max_gain_node, part);
          _pq_contains.setBit(max_gain_node * _config.partition.k + part, false);
        }
      }

      const Gain fm_gain = updateNeighbours(max_gain_node, from_part, to_part,
                                            max_allowed_part_weights[0]);

      current_cut -= fm_gain;
      current_kminusone -= max_gain;
      _stopping_policy.updateStatistics(max_gain);

      ASSERT(current_cut == metrics::hyperedgeCut(_hg),
             V(current_cut) << V(metrics::hyperedgeCut(_hg)));
      ASSERT(current_kminusone == metrics::kMinus1(_hg),
             V(current_kminusone) << V(metrics::kMinus1(_hg)));
      ASSERT(current_imbalance == metrics::imbalance(_hg, _config),
             V(current_imbalance) << V(metrics::imbalance(_hg, _config)));

      // right now, we do not allow a decrease in cut in favor of an increase in balance
      const bool improved_km1_within_balance = (current_imbalance <= _config.partition.epsilon) &&
                                               (current_kminusone < best_metrics.km1);
      const bool improved_balance_less_equal_km1 = (current_imbalance < best_metrics.imbalance) &&
                                                   (current_kminusone <= best_metrics.km1);

      ++num_moves_since_last_improvement;
      if (improved_km1_within_balance || improved_balance_less_equal_km1) {
        DBG(dbg_refinement_kway_kminusone_fm_improvements_balance && max_gain == 0,
            "KWayFM improved balance between " << from_part << " and " << to_part
            << "(max_gain=" << max_gain << ")");
        DBG(dbg_refinement_kway_kminusone_fm_improvements_cut && current_kminusone < best_metrics.km1,
            "KWayFM improved cut from " << best_metrics.km1 << " to " << current_kminusone);
        best_metrics.cut = current_cut;
        best_metrics.km1 = current_kminusone;
        best_metrics.imbalance = current_imbalance;
        _stopping_policy.resetStatistics();
        min_cut_index = num_moves;
        num_moves_since_last_improvement = 0;
        _gain_cache.resetDelta();
      }
      _performed_moves[num_moves] = { max_gain_node, from_part, to_part };
      ++num_moves;
    }
    DBG(dbg_refinement_kway_kminusone_fm_stopping_crit, "KWayFM performed " << num_moves
        << " local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (_stopping_policy.searchShouldStop(num_moves_since_last_improvement, _config, beta,
                                              best_metrics.cut, current_cut)
            == true ? "policy" : "empty queue"));

    rollback(num_moves - 1, min_cut_index);
    _gain_cache.rollbackDelta();

    ASSERT_THAT_GAIN_CACHE_IS_VALID();

    ASSERT(best_metrics.km1 == metrics::kMinus1(_hg), "Incorrect rollback operation");
    ASSERT(best_metrics.km1 <= initial_kminusone, "kMinusOne quality decreased from "
           << initial_kminusone << " to" << best_metrics.km1);
    return FMImprovementPolicy::improvementFound(best_metrics.km1, initial_kminusone,
                                                 best_metrics.imbalance, initial_imbalance,
                                                 _config.partition.epsilon);
  }

  std::string policyStringImpl() const noexcept override final {
    return std::string(" RefinerStoppingPolicy=" + templateToString<StoppingPolicy>() +
                       " RefinerUsesBucketQueue=" +
#ifdef USE_BUCKET_PQ
                       "true"
#else
                       "false"
#endif
                       );
  }

  void rollback(int last_index, const int min_cut_index) noexcept {
    DBG(false, "min_cut_index=" << min_cut_index);
    DBG(false, "last_index=" << last_index);
    while (last_index != min_cut_index) {
      const HypernodeID hn = _performed_moves[last_index].hn;
      const PartitionID from_part = _performed_moves[last_index].to_part;
      const PartitionID to_part = _performed_moves[last_index].from_part;
      // LOG("Rollback HN " << hn << "from " << from_part << " back to " << to_part);
      _hg.changeNodePart(hn, from_part, to_part);
      --last_index;
    }
  }

  void removeHypernodeMovementsFromPQ(const HypernodeID hn) noexcept {
    if (_hg.active(hn)) {
      _hg.deactivate(hn);
      for (PartitionID part = 0; part < _config.partition.k; ++part) {
        if (_pq_contains[hn * _config.partition.k + part]) {
          _pq.remove(hn, part);
          _pq_contains.setBit(hn * _config.partition.k + part, false);
        }
      }
    }
  }

  template <bool update_cache_only = true>
  void deltaGainUpdates(const HypernodeID pin, const PartitionID from_part,
                        const PartitionID to_part, const HyperedgeID he,
                        const HyperedgeWeight he_weight,
                        const bool one_pin_in_from_part_before,
                        const bool one_pin_in_to_part_after,
                        const bool two_pins_in_from_part_before,
                        const bool two_pins_in_to_part_after,
                        const HypernodeWeight max_allowed_part_weight) noexcept {
    const PartitionID source_part = _hg.partID(pin);
    if (source_part == from_part) {
      if (two_pins_in_from_part_before) {
        for (PartitionID k = 0; k < _config.partition.k; ++k) {
          if (update_cache_only && _already_processed_part.get(pin) != k) {
            _gain_cache.updateEntryIfItExists(pin, k, he_weight);
          } else {
            updatePin(pin, k, he, he_weight, max_allowed_part_weight);
          }
        }
      }
    } else if (source_part == to_part) {
      if (two_pins_in_to_part_after) {
        for (PartitionID k = 0; k < _config.partition.k; ++k) {
          if (update_cache_only && _already_processed_part.get(pin) != k) {
            _gain_cache.updateEntryIfItExists(pin, k, -he_weight);
          } else {
            updatePin(pin, k, he, -he_weight, max_allowed_part_weight);
          }
        }
      }
    }

    if (one_pin_in_from_part_before) {
      if (update_cache_only) {
        _gain_cache.updateEntryIfItExists(pin, from_part, -he_weight);
      } else {
        updatePin(pin, from_part, he, -he_weight, max_allowed_part_weight);
      }
    }

    if (one_pin_in_to_part_after) {
      if (update_cache_only && _already_processed_part.get(pin) != to_part) {
        _gain_cache.updateEntryIfItExists(pin, to_part, he_weight);
      } else {
        updatePin(pin, to_part, he, he_weight, max_allowed_part_weight);
      }
    }
  }


  void connectivityUpdateForCache(const HypernodeID pin, const PartitionID from_part,
                                  const PartitionID to_part, const HyperedgeID he,
                                  const bool move_decreased_connectivity,
                                  const bool move_increased_connectivity) noexcept {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _gain_cache.entryExists(pin, from_part) &&
        !hypernodeIsConnectedToPart(pin, from_part)) {
      DBG(dbg_refinement_kway_kminusone_gain_caching && hn_to_debug == pin,
          "removing cache entry for HN " << pin << " part=" << from_part);
      _gain_cache.removeEntryDueToConnectivityDecrease(pin, from_part);
    }
    if (move_increased_connectivity && !_gain_cache.entryExists(pin, to_part)) {
      ASSERT(_hg.connectivity(he) >= 2, V(_hg.connectivity(he)));
      DBG(dbg_refinement_kway_kminusone_gain_caching && hn_to_debug == pin,
          "adding cache entry for HN " << pin << " part=" << to_part << " gain=");
      _gain_cache.addEntryDueToConnectivityIncrease(pin, to_part,
                                                    gainInducedByHypergraph(pin, to_part));
      _already_processed_part.set(pin, to_part);
    }
  }


  void connectivityUpdate(const HypernodeID pin, const PartitionID from_part,
                          const PartitionID to_part, const HyperedgeID he,
                          const bool move_decreased_connectivity,
                          const bool move_increased_connectivity,
                          const HypernodeWeight max_allowed_part_weight) noexcept {
    ONLYDEBUG(he);
    if (move_decreased_connectivity && _pq_contains[pin * _config.partition.k + from_part] &&
        !hypernodeIsConnectedToPart(pin, from_part)) {
      _pq.remove(pin, from_part);
      _pq_contains.setBit(pin * _config.partition.k + from_part, false);
      _gain_cache.removeEntryDueToConnectivityDecrease(pin, from_part);
      // LOG("normal connectivity decrease for " << pin);
      // Now pq might actually not contain any moves for HN pin.
      // We do not need to set _active to false however, because in this case
      // the move not only decreased but also increased the connectivity and we
      // therefore add a new move to to_part in the next if-condition.
      // This resembled the case in which all but one incident HEs of HN pin are
      // internal and the "other" pin of the border HE (which has size 2) is
      // moved from one part to another.
    }
    if (move_increased_connectivity && !_pq_contains[pin * _config.partition.k + to_part]) {
      ASSERT(_hg.connectivity(he) >= 2, V(_hg.connectivity(he)));
      ASSERT(_already_processed_part.get(pin) == Hypergraph::kInvalidPartition,
             V(_already_processed_part.get(pin)));
      // LOG("normal connectivity increase for " << pin << V(to_part));
      Gain gain = GainCache::kNotCached;
      if (_gain_cache.entryExists(pin, to_part)) {
        gain = _gain_cache.entry(pin, to_part);
        ASSERT(gain == gainInducedByHypergraph(pin, to_part),
               V(pin) << V(gain) << V(gainInducedByHypergraph(pin, to_part)));
      } else {
        gain = gainInducedByHypergraph(pin, to_part);
        _gain_cache.addEntryDueToConnectivityIncrease(pin, to_part, gain);
      }
      _pq.insert(pin, to_part, gain);
      _pq_contains.setBit(pin * _config.partition.k + to_part, true);
      _already_processed_part.set(pin, to_part);

      if (_hg.partWeight(to_part) < max_allowed_part_weight) {
        _pq.enablePart(to_part);
      }
    }
    ASSERT(_pq.contains(pin) && _hg.active(pin), V(pin));
  }

  // Full update includes:
  // 1.) Activation of new border HNs
  // 2.) Removal of new non-border HNs
  // 3.) Connectivity update
  // 4.) Delta-Gain Update
  // This is used for the state transitions: free -> loose and loose -> locked
  void fullUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                  const PartitionID to_part, const HyperedgeID he,
                  const HypernodeWeight max_allowed_part_weight) noexcept {
    ONLYDEBUG(moved_hn);
    const HypernodeID pin_count_from_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_to_part_after_move = _hg.pinCountInPart(he, to_part);
    const bool move_decreased_connectivity = pin_count_from_part_before_move - 1 == 0;
    const bool move_increased_connectivity = pin_count_to_part_after_move == 1;
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);

    const bool one_pin_in_from_part_before = pin_count_from_part_before_move == 1;
    const bool one_pin_in_to_part_after = pin_count_to_part_after_move == 1;
    const bool two_pins_in_from_part_before = pin_count_from_part_before_move == 2;
    const bool two_pins_in_to_part_after = pin_count_to_part_after_move == 2;

    for (const HypernodeID pin : _hg.pins(he)) {
      // LOG(V(pin) << V(_hg.active(pin)) << V(_hg.isBorderNode(pin)));
      if (unlikely(!_hg.marked(pin))) {
        ASSERT(pin != moved_hn, V(pin));
        if (!_hg.active(pin)) {
          _hns_to_activate.push_back(pin);
        } else {
          if (!_hg.isBorderNode(pin)) {
            removeHypernodeMovementsFromPQ(pin);
          } else {
            connectivityUpdate(pin, from_part, to_part, he,
                               move_decreased_connectivity,
                               move_increased_connectivity,
                               max_allowed_part_weight);
            // false indicates that we use this method to also update the PQ.
            deltaGainUpdates<false>(pin, from_part, to_part, he, he_weight,
                                    one_pin_in_from_part_before,
                                    one_pin_in_to_part_after,
                                    two_pins_in_from_part_before,
                                    two_pins_in_to_part_after,
                                    max_allowed_part_weight);
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
        // true indicates that we only want to update cache entries
        deltaGainUpdates<true>(pin, from_part, to_part, he, he_weight,
                               one_pin_in_from_part_before,
                               one_pin_in_to_part_after,
                               two_pins_in_from_part_before,
                               two_pins_in_to_part_after,
                               max_allowed_part_weight);
      }
    }
  }


  Gain updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part, const HypernodeWeight max_allowed_part_weight)
  noexcept {
    _already_processed_part.resetUsedEntries();

    Gain fm_gain = 0;
    bool moved_hn_remains_conntected_to_from_part = false;
    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      const HypernodeID pins_in_source_part_after = _hg.pinCountInPart(he, from_part);
#ifndef NDEBUG
      const HypernodeID pins_in_target_part_after = _hg.pinCountInPart(he, to_part);
      const bool move_decreased_connectivity = pins_in_source_part_after == 0;
      const bool move_increased_connectivity = pins_in_target_part_after == 1;
      const PartitionID connectivity_before = _hg.connectivity(he)
                                              - move_increased_connectivity
                                              + move_decreased_connectivity;
#endif

      ASSERT(!_gain_cache.entryExists(moved_hn, from_part), V(moved_hn) << V(from_part));
      moved_hn_remains_conntected_to_from_part |= pins_in_source_part_after != 0;
      if (_hg.connectivity(he) == 1) {  // move made he internal
        ASSERT(connectivity_before == 2, V(connectivity_before));
        fm_gain += _hg.edgeWeight(he);
      } else if (pins_in_source_part_after + 1 == _hg.edgeSize(he)) {  // move made he cut
        ASSERT(connectivity_before == 1 && _hg.connectivity(he) == 2,
               V(connectivity_before) << V(_hg.connectivity(he)));
        fm_gain -= _hg.edgeWeight(he);
      }

      if (pins_in_source_part_after == 0 && _hg.pinCountInPart(he, to_part) != 1) {
        for (PartitionID part = 0; part < _config.partition.k; ++part) {
          if (part != from_part && part != to_part) {
            _gain_cache.updateEntryIfItExists(moved_hn, part, -_hg.edgeWeight(he));
          }
        }
      } else if (pins_in_source_part_after != 0 && _hg.pinCountInPart(he, to_part) == 1) {
        for (PartitionID part = 0; part < _config.partition.k; ++part) {
          if (part != from_part && part != to_part) {
            _gain_cache.updateEntryIfItExists(moved_hn, part, _hg.edgeWeight(he));
          }
        }
      }

      fullUpdate(moved_hn, from_part, to_part, he, max_allowed_part_weight);
    }

    _gain_cache.updateFromAndToPartOfMovedHN(moved_hn, from_part, to_part,
                                             moved_hn_remains_conntected_to_from_part);

    // remove dups
    // TODO(schlag): fix this!!!
    for (const HypernodeID hn : _hns_to_activate) {
      if (!_hg.active(hn)) {
        activate(hn, max_allowed_part_weight);
      }
    }
    _hns_to_activate.clear();

    ASSERT([&]() {
        // This lambda checks verifies the internal state of KFM for all pins that could
        // have been touched during updateNeighbours.
        for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
          bool valid = true;
          for (const HypernodeID pin : _hg.pins(he)) {
            if (_gain_cache.valid(pin)) {
              for (PartitionID part = 0; part < _config.partition.k; ++part) {
                if (_gain_cache.entry(pin, part) != GainCache::kNotCached) {
                  ASSERT(_gain_cache.entry(pin, part) == gainInducedByHypergraph(pin, part),
                         V(pin) << V(part) << V(_gain_cache.entry(pin, part)) <<
                         V(gainInducedByHypergraph(pin, part)));
                  ASSERT(hypernodeIsConnectedToPart(pin, part), V(pin) << V(part));
                } else if (_hg.partID(pin) != part) {
                  ASSERT(!hypernodeIsConnectedToPart(pin, part), V(pin) << V(part));
                }
              }
            }
            // LOG("HN" << pin << " CHECK!");
            if (!_hg.isBorderNode(pin)) {
              // The pin is an internal HN
              // there should not be any move of this HN in the PQ.
              for (PartitionID part = 0; part < _config.partition.k; ++part) {
                valid = (_pq.contains(pin, part) == false);
                if (!valid) {
                  LOG("HN " << pin << " should not be contained in PQ");
                  return false;
                }
              }
            } else {
              // Pin is a border HN
              for (const PartitionID part : _hg.connectivitySet(he)) {
                ASSERT(_hg.pinCountInPart(he, part) > 0, V(he) << " not connected to " << V(part));
                if (_pq.contains(pin, part)) {
                  // if the move to target.part is in the PQ, it has to have the correct gain
                  ASSERT(_hg.active(pin), "Pin is not active");
                  ASSERT(_hg.isBorderNode(pin), "BorderFail");
                  const Gain expected_gain = gainInducedByHypergraph(pin, part);
                  valid = _pq.key(pin, part) == expected_gain;
                  if (!valid) {
                    LOG("Incorrect maxGain for HN " << pin);
                    LOG("expected key=" << expected_gain);
                    LOG("actual key=" << _pq.key(pin, part));
                    LOG("from_part=" << _hg.partID(pin));
                    LOG("to part = " << part);
                    return false;
                  }
                  if (_hg.partWeight(part) < max_allowed_part_weight &&
                      !_pq.isEnabled(part)) {
                    LOGVAR(pin);
                    LOG("key=" << expected_gain);
                    LOG("Part " << part << " should be enabled as target part");
                    return false;
                  }
                  if (_hg.partWeight(part) >= max_allowed_part_weight &&
                      _pq.isEnabled(part)) {
                    LOGVAR(pin);
                    LOG("key=" << expected_gain);
                    LOG("Part " << part << " should NOT be enabled as target part");
                    return false;
                  }
                } else {
                  // if it is not in the PQ then either the HN has already been marked as moved
                  // or we currently look at the source partition of pin.
                  valid = (_hg.marked(pin) == true) || (part == _hg.partID(pin));
                  if (!valid) {
                    LOG("HN " << pin << " not in PQ but also not marked");
                    LOG("gain=" << gainInducedByHypergraph(pin, part));
                    LOG("from_part=" << _hg.partID(pin));
                    LOG("to_part=" << part);
                    LOG("would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), part));
                    return false;
                  }
                  if (_hg.marked(pin)) {
                    // If the pin is already marked as moved, then all moves concerning this pin
                    // should have been removed from the PQ.
                    for (PartitionID part = 0; part < _config.partition.k; ++part) {
                      if (_pq.contains(pin, part)) {
                        LOG("HN " << pin << " should not be in PQ, because it is already marked");
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
            for (PartitionID part = 0; part < _config.partition.k; ++part) {
              bool connected = false;
              for (const HyperedgeID incident_he : _hg.incidentEdges(pin)) {
                if (_hg.pinCountInPart(incident_he, part) > 0) {
                  connected = true;
                  break;
                }
              }
              if (!connected && _pq.contains(pin, part)) {
                LOG("PQ contains stale move of HN " << pin << ":");
                LOG("calculated gain=" << gainInducedByHypergraph(pin, part));
                LOG("gain in PQ=" << _pq.key(pin, part));
                LOG("from_part=" << _hg.partID(pin));
                LOG("to_part=" << part);
                LOG("would be feasible=" << moveIsFeasible(pin, _hg.partID(pin), part));
                LOG("current HN " << moved_hn << " was moved from " << from_part << " to "
                    << to_part);
                return false;
              }
            }
          }
        }
        return true;
      } (), V(moved_hn));
    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          if (_hg.active(hn)) {
            bool valid = _hg.marked(hn) || !_hg.isBorderNode(hn);
            for (PartitionID part = 0; part < _config.partition.k; ++part) {
              if (_pq.contains(hn, part)) {
                valid = true;
                break;
              }
            }
            if (!valid) {
              LOG(V(hn) << " is active but neither marked nor in one of the PQs");
              return false;
            }
          }
        }
        return true;
      } (), V(moved_hn));

    return fm_gain;
  }

  void updatePin(const HypernodeID pin, const PartitionID part, const HyperedgeID he,
                 const Gain delta, const HypernodeWeight max_allowed_part_weight) noexcept {
    ONLYDEBUG(he);
    ONLYDEBUG(max_allowed_part_weight);
    if (_pq_contains[pin * _config.partition.k + part] && _already_processed_part.get(pin) != part) {
      ASSERT(!_hg.marked(pin), " Trying to update marked HN " << pin << " part=" << part);
      ASSERT(_hg.active(pin), "Trying to update inactive HN " << pin << " part=" << part);
      ASSERT(_hg.isBorderNode(pin), "Trying to update non-border HN " << pin << " part=" << part);
      ASSERT((_hg.partWeight(part) < max_allowed_part_weight ?
              _pq.isEnabled(part) : !_pq.isEnabled(part)), V(part));
      // Assert that we only perform delta-gain updates on moves that are not stale!
      ASSERT(hypernodeIsConnectedToPart(pin, part), V(pin) << V(part));

      DBG(dbg_refinement_kway_kminusone_fm_gain_update,
          "updating gain of HN " << pin
          << " from gain " << _pq.key(pin, part) << " to " << _pq.key(pin, part) + delta
          << " (to_part=" << part << ", ExpectedGain="
          << gainInducedByHypergraph(pin, part) << ")");
      _pq.updateKeyBy(pin, part, delta);
      _gain_cache.updateExistingEntry(pin, part, delta);
    }
  }

  template <bool invalidate_hn = false>
  void activate(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) noexcept {
    ASSERT(!_hg.active(hn), V(hn));
    ASSERT([&]() {
        for (PartitionID part = 0; part < _config.partition.k; ++part) {
          ASSERT(!_pq.contains(hn, part), V(hn) << V(part));
        }
        return true;
      } (), "HN " << hn << " is already contained in PQ ");

    // Currently we cannot infer the gain changes of the two initial refinement nodes
    // from the uncontraction itself (this is still a todo). Therefore, these activations
    // have to invalidate and recalculate the gains.
    if (invalidate_hn) {
      _gain_cache.setInvalid(hn);
      initializeGainCacheFor(hn);
    }


    if (_hg.isBorderNode(hn)) {
      DBG(false && hn == 12518, " activating " << V(hn));
      ASSERT(!_hg.active(hn), V(hn));
      ASSERT(!_hg.marked(hn), "Hypernode" << hn << " is already marked");
      insertHNintoPQ(hn, max_allowed_part_weight);
      // mark HN as active for this round.
      _hg.activate(hn);
    }
  }

  Gain gainInducedByHyperedge(const HypernodeID hn, const HyperedgeID he,
                              const PartitionID target_part) const noexcept {
    const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, _hg.partID(hn));
    const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, target_part);
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);
    Gain gain = pins_in_source_part == 1 ? he_weight : 0;
    gain -= pins_in_target_part == 0 ? he_weight : 0;
    return gain;
  }

  Gain gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const noexcept {
    ASSERT(target_part != _hg.partID(hn), V(hn) << V(target_part));
    Gain gain = 0;
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      gain += gainInducedByHyperedge(hn, he, target_part);
    }
    return gain;
  }

  void initializeGainCache() {
    for (const HypernodeID hn : _hg.nodes()) {
      initializeGainCacheFor(hn);
    }
  }

  void initializeGainCacheFor(const HypernodeID hn) {
    ASSERT_THAT_TMP_GAINS_ARE_INITIALIZED_TO_ZERO();
    _tmp_target_parts.clear();
    const PartitionID source_part = _hg.partID(hn);
    HyperedgeWeight internal = 0;
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      internal += _hg.pinCountInPart(he, source_part) != 1 ? he_weight : 0;
      for (const PartitionID part : _hg.connectivitySet(he)) {
        _tmp_target_parts.add(part);
        _tmp_gains[part] += he_weight;
      }
    }

    for (const PartitionID target_part : _tmp_target_parts) {
      if (target_part == source_part) {
        _tmp_gains[source_part] = 0;
        ASSERT(!_gain_cache.entryExists(hn, source_part), V(hn) << V(source_part));
        continue;
      }
      ASSERT(_tmp_gains[target_part] - internal == gainInducedByHypergraph(hn, target_part),
             "Gain calculation failed! Should be " << V(gainInducedByHypergraph(hn, target_part))
             << " but currently it is " << V(_tmp_gains[target_part] - internal));
      DBG(dbg_refinement_kway_kminusone_gain_caching && (hn == hn_to_debug),
          V(target_part) << V(_tmp_gains[target_part] - internal));
      _gain_cache.initializeEntry(hn, target_part, _tmp_gains[target_part] - internal);
      _tmp_gains[target_part] = 0;
    }
    _gain_cache.setValid(hn);
  }


  void insertHNintoPQ(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) noexcept {
    ASSERT(_hg.isBorderNode(hn), "Cannot compute gain for non-border HN " << hn);
    ASSERT_THAT_TMP_GAINS_ARE_INITIALIZED_TO_ZERO();

    if (_gain_cache.valid(hn)) {
      for (PartitionID part = 0; part < _config.partition.k; ++part) {
        if (_gain_cache.entryExists(hn, part)) {
          ASSERT(part != _hg.partID(hn), V(hn) << V(part) << V(_gain_cache.entry(hn, part)));
          ASSERT(_gain_cache.entry(hn, part) == gainInducedByHypergraph(hn, part),
                 V(hn) << V(part) << V(_gain_cache.entry(hn, part)) <<
                 V(gainInducedByHypergraph(hn, part)));
          ASSERT(hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
          DBG(false && hn == 12518, " inserting " << V(hn) << V(part)
              << V(_gain_cache.entry(hn, part)));
          _pq.insert(hn, part, _gain_cache.entry(hn, part));
          _pq_contains.setBit(hn * _config.partition.k + part, true);
          if (_hg.partWeight(part) < max_allowed_part_weight) {
            _pq.enablePart(part);
          }
        }
      }
    } else {
      _tmp_target_parts.clear();
      const PartitionID source_part = _hg.partID(hn);
      HyperedgeWeight internal = 0;
      for (const HyperedgeID he : _hg.incidentEdges(hn)) {
        const HyperedgeWeight he_weight = _hg.edgeWeight(he);
        internal += _hg.pinCountInPart(he, source_part) != 1 ? he_weight : 0;
        for (const PartitionID part : _hg.connectivitySet(he)) {
          _tmp_target_parts.add(part);
          _tmp_gains[part] += he_weight;
        }
      }

      for (const PartitionID target_part : _tmp_target_parts) {
        if (target_part == source_part) {
          _tmp_gains[source_part] = 0;
          ASSERT(!_gain_cache.entryExists(hn, source_part), V(hn) << V(source_part));
          continue;
        }
        _tmp_gains[target_part] -= internal;
        ASSERT(_tmp_gains[target_part] == gainInducedByHypergraph(hn, target_part),
               "Gain calculation failed! Should be " << V(gainInducedByHypergraph(hn, target_part))
               << " but currently it is " << V(_tmp_gains[target_part]));
        DBG(dbg_refinement_kway_kminusone_fm_gain_comp, "inserting HN " << hn << " with gain "
            << (_tmp_gains[target_part]) << " (ExpectedGain="
            << gainInducedByHypergraph(hn, target_part) << ") sourcePart=" << _hg.partID(hn)
            << " targetPart= " << target_part);
        DBG(dbg_refinement_kway_kminusone_gain_caching && (hn == hn_to_debug),
            V(target_part) << V(_tmp_gains[target_part]));

        _pq.insert(hn, target_part, _tmp_gains[target_part]);
        _gain_cache.initializeEntry(hn, target_part, _tmp_gains[target_part]);
        _pq_contains.setBit(hn * _config.partition.k + target_part, true);
        _tmp_gains[target_part] = 0;
        if (_hg.partWeight(target_part) < max_allowed_part_weight) {
          _pq.enablePart(target_part);
        }
      }
      _gain_cache.setValid(hn);
    }
  }

  void ASSERT_THAT_GAIN_CACHE_IS_VALID() {
    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          ASSERT(_gain_cache.valid(hn), V(hn));
          for (PartitionID part = 0; part < _config.partition.k; ++part) {
            if (_gain_cache.entry(hn, part) != GainCache::kNotCached) {
              ASSERT(_gain_cache.entry(hn, part) == gainInducedByHypergraph(hn, part),
                     V(hn) << V(part) << V(_gain_cache.entry(hn, part)) <<
                     V(gainInducedByHypergraph(hn, part)));
              ASSERT(hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
            } else if (_hg.partID(hn) != part) {
              ASSERT(!hypernodeIsConnectedToPart(hn, part), V(hn) << V(part));
            }
          }
        }
        return true;
      } (), "Gain Cache inconsistent");
  }

  void ASSERT_THAT_TMP_GAINS_ARE_INITIALIZED_TO_ZERO() {
    ASSERT([&]() {
        for (Gain gain : _tmp_gains) {
          ASSERT(gain == 0, V(gain));
        }
        return true;
      } (), "_tmp_gains not initialized correctly");
  }


  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;

  FastResetBitVector<> _pq_contains;
  std::vector<Gain> _tmp_gains;
  InsertOnlyConnectivitySet<PartitionID> _tmp_target_parts;
  std::vector<RollbackInfo> _performed_moves;
  std::vector<HypernodeID> _hns_to_activate;

  // After a move, we have to update the gains for all adjacent HNs.
  // For all moves of a HN that were already present in the PQ before the
  // the current max-gain move, this can be done via delta-gain update. However,
  // the current max-gain move might also have increased the connectivity for
  // a certain HN. In this case, we have to calculate the gain for this "new"
  // move from scratch and have to exclude it from all delta-gain updates in
  // the current updateNeighbours call. If we encounter such a new move,
  // we store the newly encountered part in this vector and do not perform
  // delta-gain updates for this part.
  FastResetVector<PartitionID> _already_processed_part;

  KWayRefinementPQ _pq;
  GainCache _gain_cache;
  StoppingPolicy _stopping_policy;
};
#pragma GCC diagnostic pop
}  // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_KWAYKMINUSONEREFINER_H_
