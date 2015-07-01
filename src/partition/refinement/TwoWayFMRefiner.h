/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_

#include <algorithm>
#include <array>
#include <limits>
#include <string>
#include <vector>

#include "gtest/gtest_prod.h"

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/FastResetVector.h"
#include "lib/datastructure/KWayPriorityQueue.h"
#include "lib/datastructure/PriorityQueue.h"
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
using defs::IncidenceIterator;

namespace partition {
static const bool dbg_refinement_2way_fm_improvements_cut = false;
static const bool dbg_refinement_2way_fm_improvements_balance = false;
static const bool dbg_refinement_2way_fm_stopping_crit = false;
static const bool dbg_refinement_2way_fm_gain_update = false;
static const bool dbg_refinement_2way_fm__activation = false;
static const bool dbg_refinement_2way_locked_hes = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <class StoppingPolicy = Mandatory,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class TwoWayFMRefiner : public IRefiner,
                        private FMRefinerBase {
 private:
  using KWayRefinementPQ = KWayPriorityQueue<HypernodeID, HyperedgeWeight,
                                             std::numeric_limits<HyperedgeWeight> >;

  static constexpr char kLocked = std::numeric_limits<char>::max();
  static const char kFree = std::numeric_limits<char>::max() - 1;
  static constexpr Gain kNotCached = std::numeric_limits<Gain>::max();

 public:
  TwoWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) noexcept :
    FMRefinerBase(hypergraph, config),
    _pq(2),
    _marked(_hg.initialNumNodes(), false),
    _active(_hg.initialNumNodes(), false),
    _he_fully_active(_hg.initialNumEdges(), false),
    _performed_moves(),
    _hns_to_activate(),
    _gain_cache(_hg.initialNumNodes(), kNotCached),
    _rollback_delta_cache(_hg.initialNumNodes(), 0),
    _locked_hes(_hg.initialNumEdges(), kFree),
    _stopping_policy() {
    _performed_moves.reserve(_hg.initialNumNodes());
    _hns_to_activate.reserve(_hg.initialNumNodes());
  }

  virtual ~TwoWayFMRefiner() { }

  TwoWayFMRefiner(const TwoWayFMRefiner&) = delete;
  TwoWayFMRefiner& operator= (const TwoWayFMRefiner&) = delete;

  TwoWayFMRefiner(TwoWayFMRefiner&&) = delete;
  TwoWayFMRefiner& operator= (TwoWayFMRefiner&&) = delete;

  void activate(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) noexcept {
    if (_hg.isBorderNode(hn)) {
      const PartitionID target_part = _hg.partID(hn) ^ 1;
      ASSERT(!_active[hn], V(hn));
      ASSERT(!_marked[hn], "Hypernode" << hn << " is already marked");
      ASSERT(!_pq.contains(hn, target_part),
             "HN " << hn << " is already contained in PQ " << target_part);
      DBG(dbg_refinement_2way_fm__activation, "inserting HN " << hn << " with gain " << computeGain(hn)
          << " in PQ " << target_part);

      if (_gain_cache[hn] == kNotCached) {
        _gain_cache[hn] = computeGain(hn);
      }
      ASSERT(_gain_cache[hn] == computeGain(hn),
             V(hn) << V(_gain_cache[hn]) << V(computeGain(hn)));
      _pq.insert(hn, target_part, _gain_cache[hn]);
      if (_hg.partWeight(target_part) < max_allowed_part_weight) {
        _pq.enablePart(target_part);
      }
      // mark HN as just activated to prevent delta-gain updates in current
      // updateNeighbours call, because we just calculated the correct gain values.
      _active.setBit(hn, true);
    }
  }

  bool isInitialized() const noexcept {
    return _is_initialized;
  }

 private:
  FRIEND_TEST(ATwoWayFMRefiner, IdentifiesBorderHypernodes);
  FRIEND_TEST(ATwoWayFMRefiner, ComputesPartitionSizesOfHE);
  FRIEND_TEST(ATwoWayFMRefiner, ChecksIfPartitionSizesOfHEAreAlreadyCalculated);
  FRIEND_TEST(ATwoWayFMRefiner, ComputesGainOfHypernodeMovement);
  FRIEND_TEST(ATwoWayFMRefiner, ActivatesBorderNodes);
  FRIEND_TEST(ATwoWayFMRefiner, CalculatesNodeCountsInBothPartitions);
  FRIEND_TEST(ATwoWayFMRefiner, UpdatesNodeCountsOnNodeMovements);
  FRIEND_TEST(AGainUpdateMethod, RespectsPositiveGainUpdateSpecialCaseForHyperedgesOfSize2);
  FRIEND_TEST(AGainUpdateMethod, RespectsNegativeGainUpdateSpecialCaseForHyperedgesOfSize2);
  // TODO(schlag): find better names for testcases
  FRIEND_TEST(AGainUpdateMethod, HandlesCase0To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To0);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase2To1);
  FRIEND_TEST(AGainUpdateMethod, HandlesCase1To2);
  FRIEND_TEST(AGainUpdateMethod, HandlesSpecialCaseOfHyperedgeWith3Pins);
  FRIEND_TEST(AGainUpdateMethod, ActivatesUnmarkedNeighbors);
  FRIEND_TEST(AGainUpdateMethod, RemovesNonBorderNodesFromPQ);
  FRIEND_TEST(ATwoWayFMRefiner, UpdatesPartitionWeightsOnRollBack);
  FRIEND_TEST(AGainUpdateMethod, DoesNotDeleteJustActivatedNodes);
  FRIEND_TEST(ARefiner, DoesNotDeleteMaxGainNodeInPQ0IfItChoosesToUseMaxGainNodeInPQ1);
  FRIEND_TEST(ARefiner, ChecksIfMovePreservesBalanceConstraint);
  FRIEND_TEST(ATwoWayFMRefiner, ConsidersSingleNodeHEsDuringInitialGainComputation);
  FRIEND_TEST(ATwoWayFMRefiner, KnowsIfAHyperedgeIsFullyActive);

  void initializeImpl() noexcept final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes());
      _is_initialized = true;
    }
    std::fill(_gain_cache.begin(), _gain_cache.end(), kNotCached);
  }

  void initializeImpl(const HyperedgeWeight max_gain) noexcept final {
    if (!_is_initialized) {
      _pq.initialize(max_gain);
      _is_initialized = true;
    }
    std::fill(_gain_cache.begin(), _gain_cache.end(), kNotCached);
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, const size_t num_refinement_nodes,
                  const HypernodeWeight max_allowed_part_weight,
                  HyperedgeWeight& best_cut, double& best_imbalance) noexcept final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg, _config.partition.k))),
           "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg, _config.partition.k));

    _pq.clear();
    _marked.resetAllBitsToFalse();
    _active.resetAllBitsToFalse();
    _he_fully_active.resetAllBitsToFalse();
    _locked_hes.resetUsedEntries();

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      _gain_cache[refinement_nodes[i]] = kNotCached;
      activate(refinement_nodes[i], max_allowed_part_weight);
    }

    ASSERT([&]() {
        for (const HypernodeID hn : _hg.nodes()) {
          if (_gain_cache[hn] != kNotCached && _gain_cache[hn] != computeGain(hn)) {
            LOGVAR(hn);
            LOGVAR(_gain_cache[hn]);
            LOGVAR(computeGain(hn));
            return false;
          }
        }
        return true;
      } (), "GainCache Invalid");

    const HyperedgeWeight initial_cut = best_cut;
    const double initial_imbalance = best_imbalance;
    HyperedgeWeight current_cut = best_cut;
    double current_imbalance = best_imbalance;

    int min_cut_index = -1;
    int num_moves = 0;
    int num_moves_since_last_improvement = 0;
    _stopping_policy.resetStatistics();

    const double beta = log(_hg.numNodes());
    while (!_pq.empty() && !_stopping_policy.searchShouldStop(num_moves_since_last_improvement,
                                                              _config, beta, best_cut, current_cut)) {
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      const PartitionID from_part = _hg.partID(max_gain_node);

      DBG(false, "cut=" << current_cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << _hg.partID(max_gain_node)
          << " target_part=" << to_part);
      ASSERT(!_marked[max_gain_node],
             "HN " << max_gain_node << "is marked and not eligable to be moved");
      ASSERT(max_gain == computeGain(max_gain_node), "Inconsistent gain caculation: " <<
             "expected g(" << max_gain_node << ")=" << computeGain(max_gain_node) <<
             " - got g(" << max_gain_node << ")=" << max_gain);
      ASSERT(_hg.isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");
      ASSERT([&]() {
          _hg.changeNodePart(max_gain_node, from_part, to_part);
          ASSERT((current_cut - max_gain) == metrics::hyperedgeCut(_hg),
                 "cut=" << current_cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
          _hg.changeNodePart(max_gain_node, to_part, from_part);
          return true;
        } ()
             , "max_gain move does not correspond to expected cut!");

      moveHypernode(max_gain_node, from_part, to_part);
      _marked.setBit(max_gain_node, true);
      _active.setBit(max_gain_node, false);

      if (_hg.partWeight(to_part) >= max_allowed_part_weight) {
        _pq.disablePart(to_part);
      }
      if (_hg.partWeight(from_part) < max_allowed_part_weight) {
        _pq.enablePart(from_part);
      }

      current_imbalance = (static_cast<double>(std::max(_hg.partWeight(0), _hg.partWeight(1))) /
                           (ceil((_hg.partWeight(0) + _hg.partWeight(1)) / 2.0))) - 1.0;


      current_cut -= max_gain;
      _stopping_policy.updateStatistics(max_gain);

      ASSERT(current_cut == metrics::hyperedgeCut(_hg),
             V(current_cut) << V(metrics::hyperedgeCut(_hg)));
      ASSERT(current_imbalance == metrics::imbalance(_hg, _config.partition.k),
             V(current_imbalance) << V(metrics::imbalance(_hg, _config.partition.k)));

      // TODO(schlag):
      // [ ] what about zero-gain updates?
      updateNeighbours(max_gain_node, from_part, to_part, max_allowed_part_weight);

      // right now, we do not allow a decrease in cut in favor of an increase in balance
      const bool improved_cut_within_balance = (current_imbalance <= _config.partition.epsilon) &&
                                               (current_cut < best_cut);
      const bool improved_balance_less_equal_cut = (current_imbalance < best_imbalance) &&
                                                   (current_cut <= best_cut);
      ++num_moves_since_last_improvement;
      if (improved_cut_within_balance || improved_balance_less_equal_cut) {
        DBG(dbg_refinement_2way_fm_improvements_balance && max_gain == 0,
            "2WayFM improved balance between " << from_part << " and " << to_part
            << "(max_gain=" << max_gain << ")");
        DBG(dbg_refinement_2way_fm_improvements_cut && current_cut < best_cut,
            "2WayFM improved cut from " << best_cut << " to " << current_cut);
        best_cut = current_cut;
        best_imbalance = current_imbalance;
        _stopping_policy.resetStatistics();
        min_cut_index = num_moves;
        num_moves_since_last_improvement = 0;
        _rollback_delta_cache.resetUsedEntries();
      }
      _performed_moves[num_moves] = max_gain_node;
      ++num_moves;
    }
    DBG(dbg_refinement_2way_fm_stopping_crit, "KWayFM performed " << num_moves
        << " local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (_stopping_policy.searchShouldStop(num_moves_since_last_improvement, _config, beta,
                                              best_cut, current_cut)
            == true ? "policy" : "empty queue"));

    rollback(num_moves - 1, min_cut_index);
    _rollback_delta_cache.resetUsedEntries(_gain_cache);

    ASSERT([&]() {
        for (HypernodeID hn = 0; hn < _gain_cache.size(); ++hn) {
          if (_gain_cache[hn] != kNotCached && _gain_cache[hn] != computeGain(hn)) {
            LOGVAR(hn);
            LOGVAR(_gain_cache[hn]);
            LOGVAR(computeGain(hn));
            return false;
          }
        }
        return true;
      } (), "GainCache Invalid");


    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_cut);
    return FMImprovementPolicy::improvementFound(best_cut, initial_cut, best_imbalance,
                                                 initial_imbalance, _config.partition.epsilon);
  }

  void updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part, const HypernodeWeight max_allowed_part_weight) {
    _gain_cache[moved_hn] = kNotCached;

    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      if (_locked_hes.get(he) != kLocked) {
        if (_locked_hes.get(he) == to_part) {
          // he is loose
          // TODO(schlag): We could provide a version similar to KFM that does not check for activation.
          fullUpdate(from_part, to_part, he, max_allowed_part_weight);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " maintained state: loose");
        } else if (_locked_hes.get(he) == kFree) {
          // he is free.
          fullUpdate(from_part, to_part, he, max_allowed_part_weight);
          _locked_hes.set(he, to_part);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " changed state: free -> loose");
        } else {
          // he is loose and becomes locked after the move
          fullUpdate(from_part, to_part, he, max_allowed_part_weight);
          _locked_hes.set(he, kLocked);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " changed state: loose -> locked");
        }
      } else {
        // he is locked
        // In case of 2-FM, nothing to do here
        DBG(dbg_refinement_2way_locked_hes, he << " is locked");
      }
    }

    // remove dups
    // TODO(schlag): fix this!!!
    for (const HypernodeID hn : _hns_to_activate) {
      if (!_active[hn]) {
        activate(hn, max_allowed_part_weight);
      }
    }
    _hns_to_activate.clear();

    ASSERT([&]() {
        for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
          for (const HypernodeID pin : _hg.pins(he)) {
            const PartitionID other_part = _hg.partID(pin) ^ 1;
            if (!_hg.isBorderNode(pin)) {
              // The pin is an internal HN
              if (_pq.contains(pin, other_part) || _active[pin]) {
                LOG("HN " << pin << " should not be contained in PQ");
                return false;
              }
            } else {
              // HN is border HN
              if (_pq.contains(pin, other_part)) {
                ASSERT(_active[pin], "HN " << pin << " is in PQ but not active");
                ASSERT(!_marked[pin],
                       "HN " << pin << " should not be in PQ, because it is already marked");
                if (_pq.key(pin, other_part) != computeGain(pin)) {
                  LOG("Incorrect gain for HN " << pin);
                  LOG("expected key=" << computeGain(pin));
                  LOG("actual key=" << _pq.key(pin, other_part));
                  LOG("from_part=" << _hg.partID(pin));
                  LOG("to part = " << other_part);
                  return false;
                }
              } else if (!_marked[pin]) {
                LOG("HN " << pin << " not in PQ, but also not marked!");
                return false;
              }
            }
            if (_gain_cache[pin] != kNotCached && _gain_cache[pin] != computeGain(pin)) {
              LOG("GainCache invalid after UpdateNeighbours");
              LOGVAR(pin);
              LOGVAR(_gain_cache[pin]);
              LOGVAR(computeGain(pin));
              return false;
            }
          }
        }
        return true;
      } (), "UpdateNeighbors failed!");
  }

  bool moveAffectsGainUpdate(const HypernodeID pin_count_source_part_after_move,
                             const HypernodeID pin_count_target_part_after_move,
                             const HypernodeID he_size) const {
    return (pin_count_source_part_after_move == 0 || pin_count_source_part_after_move == 1 ||
            pin_count_target_part_after_move == 1 || pin_count_target_part_after_move == 2 ||
            he_size == 2);
  }

  // Full update includes:
  // 1.) Activation of new border HNs
  // 2.) Removal of new non-border HNs
  // 3.) Delta-Gain Update as decribed in [ParMar06]: encoded in factor
  // This is used for the state transitions: free -> loose and loose -> locked
  void fullUpdate(const PartitionID from_part,
                  const PartitionID to_part, const HyperedgeID he,
                  const HypernodeWeight max_allowed_part_weight) noexcept {
    const HypernodeID pin_count_from_part_after_move = _hg.pinCountInPart(he, from_part);
    const HypernodeID pin_count_to_part_after_move = _hg.pinCountInPart(he, to_part);
    const HypernodeID he_size = _hg.edgeSize(he);
    if (!_he_fully_active[he] || moveAffectsGainUpdate(pin_count_from_part_after_move,
                                                       pin_count_to_part_after_move,
                                                       he_size)) {
      Gain he_induced_factor = 0;
      if (he_size == 2) {
        he_induced_factor = (pin_count_to_part_after_move == 1 ? 2 : -2);
      } else if (pin_count_to_part_after_move == 1) {
        // HE was an internal edge before move and is a cut HE now.
        // Before the move, all pins had gain -w(e). Now after the move,
        // these pins have gain 0 (since all of them are in from_part).
        he_induced_factor = 1;
      } else if (pin_count_from_part_after_move == 0) {
        // HE was cut HE before move and is internal HE now.
        // Since the HE is now internal, moving a pin incurs gain -w(e)
        // and make it a cut HE again.
        he_induced_factor = -1;
      }
      ASSERT(he_size != 1, V(he));
      const HyperedgeWeight he_weight = _hg.edgeWeight(he);
      const bool potential_single_pin_gain_increase = (pin_count_from_part_after_move == 1);
      const bool potential_single_pin_gain_decrease = (pin_count_to_part_after_move == 2);
      HypernodeID num_active_pins = 1;  // because moved_hn was active
      for (const HypernodeID pin : _hg.pins(he)) {
        Gain pin_specific_factor = he_induced_factor;
        if (pin_specific_factor == 0) {
          if (potential_single_pin_gain_increase && _hg.partID(pin) == from_part) {
            // Before move, there were two pins (moved_node and the current pin) in from_part.
            // After moving moved_node to to_part, the gain of the remaining pin in
            // from_part increases by w(he).
            pin_specific_factor = 1;
          } else if (potential_single_pin_gain_decrease && _hg.partID(pin) == to_part) {
            // Before move, pin was the only HN in to_part. It thus had a
            // positive gain, because moving it to from_part would have removed
            // the HE from the cut. Now, after the move, pin becomes a 0-gain HN
            // because now there are pins in both parts.
            pin_specific_factor = -1;
          }
        }
        if (!_marked[pin]) {
          if (!_active[pin]) {
            ASSERT(!_pq.contains(pin, (_hg.partID(pin) ^ 1)), V(pin) << V((_hg.partID(pin) ^ 1)));
            ++num_active_pins;  // since we do lazy activation!
            _hns_to_activate.push_back(pin);
          } else {
            if (!_hg.isBorderNode(pin)) {
              DBG(dbg_refinement_2way_fm_gain_update, "TwoWayFM deleting pin " << pin << " from PQ "
                  << to_part);
              if (_active[pin]) {
                ASSERT(_pq.contains(pin, (_hg.partID(pin) ^ 1)), V(pin) << V((_hg.partID(pin) ^ 1)));
                // This invalidation is not necessary since the cached gain will stay up-to-date
                // _gain_cache[pin] = kNotCached;
                _pq.remove(pin, (_hg.partID(pin) ^ 1));
                _active.setBit(pin, false);
              }
            } else {
              if (pin_specific_factor != 0) {
                updatePin(pin, pin_specific_factor, max_allowed_part_weight, he_weight);
              }
              // Otherwise delta-gain would be zero and zero delta-gain updates are bad.
              // See for example [CadKaMa99]
              ++num_active_pins;
              continue;  // caching is done in updatePin in this case
            }
          }
        }
        // Caching
        if (pin_specific_factor != 0) {
          const Gain gain_delta = pin_specific_factor * he_weight;
          _rollback_delta_cache.set(pin, _rollback_delta_cache.get(pin) - gain_delta);
          _gain_cache[pin] += (_gain_cache[pin] != kNotCached ? gain_delta : 0);
        }
      }
      _he_fully_active.setBit(he, (he_size == num_active_pins));
    }
  }

  int numRepetitionsImpl() const noexcept final {
    return _config.fm_local_search.num_repetitions;
  }

  std::string policyStringImpl() const noexcept final {
    return std::string(" RefinerStoppingPolicy=" + templateToString<StoppingPolicy>() +
                       " RefinerUsesBucketQueue=" +
#ifdef USE_BUCKET_PQ
                       "true"
#else
                       "false"
#endif
                       );
  }

  void updatePin(const HypernodeID pin, const Gain factor,
                 const HypernodeWeight max_allowed_part_weight,
                 const HyperedgeWeight he_weight) noexcept {
    ONLYDEBUG(max_allowed_part_weight);
    const PartitionID target_part = _hg.partID(pin) ^ 1;
    ASSERT(_active[pin], V(pin) << V(target_part));
    ASSERT(_pq.contains(pin, target_part), V(pin) << V(target_part));
    ASSERT(factor != 0, V(factor));
    ASSERT(!_marked[pin],
           " Trying to update marked HN " << pin << " in PQ " << target_part);
    ASSERT(_hg.isBorderNode(pin), "Trying to update non-border HN " << pin << " PQ=" << target_part);
    ASSERT((_hg.partWeight(target_part) < max_allowed_part_weight ?
            _pq.isEnabled(target_part) : !_pq.isEnabled(target_part)), V(target_part));
    const Gain gain_delta = factor * he_weight;
    DBG(dbg_refinement_2way_fm_gain_update, "TwoWayFM updating gain of HN " << pin
        << " from gain " << _pq.key(pin, target_part) << " to "
        << _pq.key(pin, target_part) + gain_delta << " in PQ " << target_part);
    _pq.updateKeyBy(pin, target_part, gain_delta);
    ASSERT(_gain_cache[pin] != kNotCached, "Error");
    _rollback_delta_cache.set(pin, _rollback_delta_cache.get(pin) - gain_delta);
    _gain_cache[pin] += gain_delta;
  }

  void rollback(int last_index, const int min_cut_index) noexcept {
    DBG(false, "min_cut_index=" << min_cut_index);
    DBG(false, "last_index=" << last_index);
    while (last_index != min_cut_index) {
      HypernodeID hn = _performed_moves[last_index];
      _hg.changeNodePart(hn, _hg.partID(hn), (_hg.partID(hn) ^ 1));
      --last_index;
    }
  }

  Gain computeGain(const HypernodeID hn) const noexcept {
    Gain gain = 0;
    ASSERT(_hg.partID(hn) < 2, "Trying to do gain computation for k-way partitioning");
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      // As we currently do not ensure that the hypergraph does not contain any
      // single-node HEs, we explicitly have to check for |e| > 1
      if (_hg.pinCountInPart(he, _hg.partID(hn) ^ 1) == 0 && _hg.edgeSize(he) > 1) {
        gain -= _hg.edgeWeight(he);
      }
      if (_hg.pinCountInPart(he, _hg.partID(hn)) == 1 && _hg.edgeSize(he) > 1) {
        gain += _hg.edgeWeight(he);
      }
    }
    return gain;
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;
  KWayRefinementPQ _pq;
  FastResetBitVector<> _marked;
  FastResetBitVector<> _active;
  FastResetBitVector<> _he_fully_active;
  std::vector<HypernodeID> _performed_moves;
  std::vector<HypernodeID> _hns_to_activate;
  std::vector<Gain> _gain_cache;
  FastResetVector<Gain> _rollback_delta_cache;
  FastResetVector<char> _locked_hes;
  StoppingPolicy _stopping_policy;
};

template <class T, class U>
const char TwoWayFMRefiner<T, U>::kFree;

template <class T, class U>
const HyperedgeWeight TwoWayFMRefiner<T, U>::kNotCached;

#pragma GCC diagnostic pop
}                                   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_
