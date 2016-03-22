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
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "tools/RandomFunctions.h"

using datastructure::KWayPriorityQueue;
using datastructure::FastResetVector;
using datastructure::FastResetBitVector;

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
  static const bool dbg_refinement_kway_kminusone_locked_hes = false;
  static const bool dbg_refinement_kway_kminusone_infeasible_moves = false;

  using Gain = HyperedgeWeight;
  using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, Gain,
                                             std::numeric_limits<Gain> >;

  struct RollbackInfo {
    HypernodeID hn;
    PartitionID from_part;
    PartitionID to_part;
  };

  static constexpr PartitionID kLocked = std::numeric_limits<PartitionID>::max();
  static const PartitionID kFree = -1;

 public:
  KWayKMinusOneRefiner(Hypergraph& hypergraph, const Configuration& config) noexcept :
    FMRefinerBase(hypergraph, config),
    _seen(_config.partition.k, false),
    _he_fully_active(_hg.initialNumEdges(), false),
    _pq_contains(_hg.initialNumNodes() * _config.partition.k, false),
    _tmp_gains(_config.partition.k, 0),
    _tmp_target_parts(),
    _performed_moves(),
    _hns_to_activate(),
    _already_processed_part(_hg.initialNumNodes(), Hypergraph::kInvalidPartition),
    _locked_hes(_hg.initialNumEdges(), kFree),
    _pq(_config.partition.k),
    _stopping_policy() {
    _tmp_target_parts.reserve(_config.partition.k);
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
    }
  }
#else
  void initializeImpl() noexcept override final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes());
      _is_initialized = true;
    }
  }
#endif

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                  const std::pair<HyperedgeWeight, HyperedgeWeight>& UNUSED(changes),
                  Metrics& best_metrics) noexcept override final {
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg),
           "initial best_metrics.cut " << best_metrics.cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_metrics.imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg, _config))),
           "initial best_metrics.imbalance " << best_metrics.imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg, _config));

    _pq.clear();
    _hg.resetHypernodeState();
    _he_fully_active.resetAllBitsToFalse();
    _pq_contains.resetAllBitsToFalse();

    _locked_hes.resetUsedEntries();

    Randomize::shuffleVector(refinement_nodes, refinement_nodes.size());
    for (const HypernodeID hn : refinement_nodes) {
      activate(hn, max_allowed_part_weights[0]);
    }

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
                                                              _config, beta, best_metrics.cut, current_cut)) {
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
      ASSERT(max_gain == gainInducedByHypergraph(max_gain_node, to_part), "Inconsistent gain caculation!"
             << V(max_gain) << ", " << V(gainInducedByHypergraph(max_gain_node, to_part)));
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
    ASSERT(best_metrics.km1 == metrics::kMinus1(_hg), "Incorrect rollback operation");
    ASSERT(best_metrics.km1 <= initial_kminusone, "kMinusOne quality decreased from "
           << initial_kminusone << " to" << best_metrics.km1);
    return FMImprovementPolicy::improvementFound(best_metrics.km1, initial_kminusone, best_metrics.imbalance,
                                                 initial_imbalance, _config.partition.epsilon);
  }

  int numRepetitionsImpl() const noexcept override final {
    return _config.fm_local_search.num_repetitions;
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
    ASSERT([&]() {
        for (PartitionID part = 0; part < _config.partition.k; ++part) {
          if (_pq_contains[hn * _config.partition.k + part]) {
            return false;
          }
        }
        return true;
      } (), "Error");
  }

  void deltaGainUpdates(const HypernodeID pin, const PartitionID from_part,
                        const PartitionID to_part, const HyperedgeID he, const HypernodeID he_size,
                        const HyperedgeWeight he_weight,
                        const HypernodeID pin_count_source_part_before_move,
                        const HypernodeID pin_count_target_part_after_move,
                        const HypernodeWeight max_allowed_part_weight) noexcept {
    PartitionID source_part = _hg.partID(pin);
    if (pin_count_source_part_before_move == 2 && source_part == from_part) {
      for (PartitionID k = 0; k < _config.partition.k; ++k) {
        if (_pq_contains[pin * _config.partition.k + k]) {
          updatePin(pin, k, he, he_weight, max_allowed_part_weight);
        }
      }
    }

    if (pin_count_source_part_before_move == 1) {
      if (_pq_contains[pin * _config.partition.k + from_part]) {
        updatePin(pin, from_part, he, -he_weight, max_allowed_part_weight);
      }
    }

    if (pin_count_target_part_after_move == 1) {
      if (_pq_contains[pin * _config.partition.k + to_part]) {
        updatePin(pin, to_part, he, he_weight, max_allowed_part_weight);
      }
    }

    if (pin_count_target_part_after_move == 2 && source_part == to_part) {
      for (PartitionID k = 0; k < _config.partition.k; ++k) {
        if (_pq_contains[pin * _config.partition.k + k]) {
          updatePin(pin, k, he, -he_weight, max_allowed_part_weight);
        }
      }
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
      _pq.insert(pin, to_part, gainInducedByHypergraph(pin, to_part));
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
    const HypernodeID pin_count_source_part_before_move = _hg.pinCountInPart(he, from_part) + 1;
    const HypernodeID pin_count_target_part_before_move = _hg.pinCountInPart(he, to_part) - 1;
    const HypernodeID pin_count_source_part_after_move = pin_count_source_part_before_move - 1;

    const HypernodeID pin_count_target_part_after_move = pin_count_target_part_before_move + 1;
    const bool move_decreased_connectivity = pin_count_source_part_after_move == 0;
    const bool move_increased_connectivity = pin_count_target_part_after_move == 1;

    const HypernodeID he_size = _hg.edgeSize(he);
    const HyperedgeWeight he_weight = _hg.edgeWeight(he);

    HypernodeID num_active_pins = 0;
    for (const HypernodeID pin : _hg.pins(he)) {
      if (!_hg.marked(pin)) {
        ASSERT(pin != moved_hn, V(pin));
        if (!_hg.active(pin)) {
          _hns_to_activate.push_back(pin);
          ++num_active_pins;
        } else {
          if (!_hg.isBorderNode(pin)) {
            removeHypernodeMovementsFromPQ(pin);
          } else {
            connectivityUpdate(pin, from_part, to_part, he,
                               move_decreased_connectivity,
                               move_increased_connectivity,
                               max_allowed_part_weight);
            deltaGainUpdates(pin, from_part, to_part, he, he_size, he_weight,
                             pin_count_source_part_before_move,
                             pin_count_target_part_after_move,
                             max_allowed_part_weight);
          }
        }
      }
      num_active_pins += _hg.marked(pin) || _hg.active(pin);
    }
    _he_fully_active.setBit(he, (num_active_pins == he_size));
  }


  void connectivityUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                          const PartitionID to_part, const HyperedgeID he,
                          const HypernodeWeight max_allowed_part_weight) noexcept {
    ONLYDEBUG(moved_hn);
    const bool move_decreased_connectivity = _hg.pinCountInPart(he, from_part) == 0;
    const bool move_increased_connectivity = _hg.pinCountInPart(he, to_part) - 1 == 0;
    if (move_decreased_connectivity || move_increased_connectivity) {
      for (const HypernodeID pin : _hg.pins(he)) {
        if (!_hg.marked(pin)) {
          ASSERT(pin != moved_hn, V(pin));
          ASSERT(_hg.active(pin), V(pin));
          ASSERT(_hg.isBorderNode(pin), V(pin));
          connectivityUpdate(pin, from_part, to_part, he,
                             move_decreased_connectivity,
                             move_increased_connectivity,
                             max_allowed_part_weight);
        }
      }
    }
  }


  Gain updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part, const HypernodeWeight max_allowed_part_weight)
  noexcept {
    _already_processed_part.resetUsedEntries();
    Gain fm_gain = 0;
    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      const HypernodeID pins_in_source_part_after = _hg.pinCountInPart(he, from_part);
      const HypernodeID pins_in_target_part_after = _hg.pinCountInPart(he, to_part);
      const bool move_decreased_connectivity = pins_in_source_part_after == 0;
      const bool move_increased_connectivity = pins_in_target_part_after == 1;
      const PartitionID connectivity_before = _hg.connectivity(he)
                                              - move_increased_connectivity
                                              + move_decreased_connectivity;
      if (connectivity_before == 2 && _hg.connectivity(he) == 1) {
        fm_gain += _hg.edgeWeight(he);
      } else if (connectivity_before == 1 && _hg.connectivity(he) == 2) {
        fm_gain -= _hg.edgeWeight(he);
      }

      fullUpdate(moved_hn, from_part, to_part, he, max_allowed_part_weight);
    }

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
                    LOG("_locked_hes[" << he << "]=" << _locked_hes.get(he));
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
                    LOG("_locked_hes[" << he << "]=" << _locked_hes.get(he));
                    return false;
                  }
                  if (_hg.marked(pin)) {
                    // If the pin is already marked as moved, then all moves concerning this pin
                    // should have been removed from the PQ.
                    for (PartitionID part = 0; part < _config.partition.k; ++part) {
                      if (_pq.contains(pin, part)) {
                        LOG("HN " << pin << " should not be contained in PQ, because it is already marked");
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
                LOG("current HN " << moved_hn << " was moved from " << from_part << " to " << to_part);
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
      ASSERT([&]() {
          for (const HyperedgeID he : _hg.incidentEdges(pin)) {
            if (_hg.pinCountInPart(he, part) > 0) {
              return true;
            }
          }
          return false;
        } (), V(pin));

      DBG(dbg_refinement_kway_kminusone_fm_gain_update,
          "updating gain of HN " << pin
          << " from gain " << _pq.key(pin, part) << " to " << _pq.key(pin, part) + delta << " (to_part="
          << part << ", ExpectedGain=" << gainInducedByHypergraph(pin, part) << ")");
      _pq.updateKeyBy(pin, part, delta);
    }
  }

  void activate(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) noexcept {
    ASSERT(!_hg.active(hn), V(hn));
    ASSERT([&]() {
        for (PartitionID part = 0; part < _config.partition.k; ++part) {
          if (_pq.contains(hn, part)) {
            return false;
          }
        }
        return true;
      } (),
           "HN " << hn << " is already contained in PQ ");
    if (_hg.isBorderNode(hn)) {
      insertHNintoPQ(hn, max_allowed_part_weight);
      // mark HN as active for this round.
      _hg.activate(hn);
    }
  }


  Gain gainInducedByHyperedge(const HypernodeID hn, const HyperedgeID he,
                              const PartitionID target_part) const noexcept {
    const HypernodeID pins_in_source_part = _hg.pinCountInPart(he, _hg.partID(hn));
    const HypernodeID pins_in_target_part = _hg.pinCountInPart(he, target_part);
    Gain gain = 0;
    if (pins_in_source_part == 1) {
      // Hypergraph should not contain single-node HEs
      ASSERT(_hg.connectivity(he) > 1, V(he));
      gain += _hg.edgeWeight(he);
    }
    if (pins_in_target_part == 0) {
      gain -= _hg.edgeWeight(he);
    }
    return gain;
  }

  Gain gainInducedByHypergraph(const HypernodeID hn, const PartitionID target_part) const noexcept {
    Gain gain = 0;
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      ASSERT(_hg.edgeSize(he) > 1, V(he));
      gain += gainInducedByHyperedge(hn, he, target_part);
    }
    return gain;
  }


  void insertHNintoPQ(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) noexcept {
    ASSERT(!_hg.marked(hn), " Trying to insertHNintoPQ for  marked HN " << hn);
    ASSERT(_hg.isBorderNode(hn), "Cannot compute gain for non-border HN " << hn);
    ASSERT([&]() {
        for (Gain gain : _tmp_gains) {
          if (gain != 0) {
            return false;
          }
        }
        return true;
      } () == true, "_tmp_gains not initialized correctly");

    const PartitionID source_part = _hg.partID(hn);

    _tmp_target_parts.clear();
    _seen.resetAllBitsToFalse();

    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      for (const PartitionID part : _hg.connectivitySet(he)) {
        if (likely(!_seen[part])) {
          _seen.setBit(part, true);
          _tmp_target_parts.push_back(part);
        }
      }
    }

    for (const PartitionID target_part : _tmp_target_parts) {
      for (const HyperedgeID he : _hg.incidentEdges(hn)) {
        if (target_part != source_part) {
          _tmp_gains[target_part] += gainInducedByHyperedge(hn, he, target_part);
        }
      }
    }

    for (const PartitionID target_part : _tmp_target_parts) {
      if (target_part == source_part) {
        _tmp_gains[source_part] = 0;
        continue;
      }
      ASSERT(_tmp_gains[target_part] == gainInducedByHypergraph(hn, target_part),
             "Gain calculation failed! Should be " << V(gainInducedByHypergraph(hn, target_part))
             << " but currently it is " << V(_tmp_gains[target_part]));
      DBG(dbg_refinement_kway_kminusone_fm_gain_comp, "inserting HN " << hn << " with gain "
          << (_tmp_gains[target_part]) << " (ExpectedGain=" << gainInducedByHypergraph(hn, target_part) << ") sourcePart=" << _hg.partID(hn)
          << " targetPart= " << target_part);
      _pq.insert(hn, target_part, _tmp_gains[target_part]);
      _pq_contains.setBit(hn * _config.partition.k + target_part, true);
      _tmp_gains[target_part] = 0;
      if (_hg.partWeight(target_part) < max_allowed_part_weight) {
        _pq.enablePart(target_part);
      }
    }
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;
  FastResetBitVector<> _seen;

  FastResetBitVector<> _he_fully_active;
  FastResetBitVector<> _pq_contains;
  std::vector<Gain> _tmp_gains;
  std::vector<PartitionID> _tmp_target_parts;
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

  FastResetVector<PartitionID> _locked_hes;
  KWayRefinementPQ _pq;
  StoppingPolicy _stopping_policy;
};

template <class T, bool b, class U>
const PartitionID KWayKMinusOneRefiner<T, b, U>::kFree;
#pragma GCC diagnostic pop
}  // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_KWAYKMINUSONEREFINER_H_
