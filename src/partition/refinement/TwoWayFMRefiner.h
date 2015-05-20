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

 public:
  TwoWayFMRefiner(const TwoWayFMRefiner&) = delete;
  TwoWayFMRefiner(TwoWayFMRefiner&&) = delete;
  TwoWayFMRefiner& operator= (const TwoWayFMRefiner&) = delete;
  TwoWayFMRefiner& operator= (TwoWayFMRefiner&&) = delete;

  TwoWayFMRefiner(Hypergraph& hypergraph, const Configuration& config) noexcept :
    FMRefinerBase(hypergraph, config),
    _pq(2),
    _marked(_hg.initialNumNodes()),
    _just_activated(_hg.initialNumNodes(), false),
    _he_fully_active(_hg.initialNumEdges(), false),
    _performed_moves(),
    _locked_hes(_hg.initialNumEdges(), kFree),
    _stats() {
    _performed_moves.reserve(_hg.initialNumNodes());
  }

  ~TwoWayFMRefiner() { }

  void activate(const HypernodeID hn, const HypernodeWeight max_allowed_part_weight) noexcept {
    if (isBorderNode(hn)) {
      const PartitionID target_part = _hg.partID(hn) ^ 1;
      ASSERT(!_marked[hn], "Hypernode" << hn << " is already marked");
      ASSERT(!_pq.contains(hn, target_part),
             "HN " << hn << " is already contained in PQ " << target_part);
      DBG(dbg_refinement_2way_fm__activation, "inserting HN " << hn << " with gain " << computeGain(hn)
          << " in PQ " << target_part);
      _pq.insert(hn, target_part, computeGain(hn));
      if (_hg.partWeight(target_part) < max_allowed_part_weight) {
        _pq.enablePart(target_part);
      }
      // mark HN as just activated to prevent delta-gain updates in current
      // updateNeighbours call, because we just calculated the correct gain values.
      _just_activated[hn] = true;
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

  void initializeImpl() noexcept final {
    if (!_is_initialized) {
      _pq.initialize(_hg.initialNumNodes());
    }
    _is_initialized = true;
  }

  void initializeImpl(const HyperedgeWeight max_gain) noexcept final {
    if (!_is_initialized) {
      _pq.initialize(max_gain);
    }
    _is_initialized = true;
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes, const size_t num_refinement_nodes,
                  const HypernodeWeight max_allowed_part_weight,
                  HyperedgeWeight& best_cut, double& best_imbalance) noexcept final {
    ASSERT(best_cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg))),
           "initial best_imbalance " << best_imbalance << "does not equal imbalance induced"
           << " by hypergraph " << metrics::imbalance(_hg));

    _pq.clear();
    _marked.assign(_marked.size(), false);
    _he_fully_active.assign(_he_fully_active.size(), false);
    _locked_hes.resetUsedEntries();

    Randomize::shuffleVector(refinement_nodes, num_refinement_nodes);
    for (size_t i = 0; i < num_refinement_nodes; ++i) {
      activate(refinement_nodes[i], max_allowed_part_weight);
    }

    const HyperedgeWeight initial_cut = best_cut;
    const double initial_imbalance = best_imbalance;
    HyperedgeWeight current_cut = best_cut;
    double current_imbalance = best_imbalance;

    int min_cut_index = -1;
    int num_moves = 0;
    int num_moves_since_last_improvement = 0;
    StoppingPolicy::resetStatistics();

    const double beta = log(_hg.numNodes());
    while (!_pq.empty() && !StoppingPolicy::searchShouldStop(num_moves_since_last_improvement,
                                                             _config, beta, best_cut, current_cut)) {
      Gain max_gain = kInvalidGain;
      HypernodeID max_gain_node = kInvalidHN;
      PartitionID to_part = Hypergraph::kInvalidPartition;
      _pq.deleteMax(max_gain_node, max_gain, to_part);
      PartitionID from_part = _hg.partID(max_gain_node);

      DBG(false, "cut=" << current_cut << " max_gain_node=" << max_gain_node
          << " gain=" << max_gain << " source_part=" << _hg.partID(max_gain_node)
          << " target_part=" << to_part);
      ASSERT(!_marked[max_gain_node],
             "HN " << max_gain_node << "is marked and not eligable to be moved");
      ASSERT(max_gain == computeGain(max_gain_node), "Inconsistent gain caculation: " <<
             "expected g(" << max_gain_node << ")=" << computeGain(max_gain_node) <<
             " - got g(" << max_gain_node << ")=" << max_gain);
      ASSERT(isBorderNode(max_gain_node), "HN " << max_gain_node << "is no border node");
      ASSERT([&]() {
          _hg.changeNodePart(max_gain_node, from_part, to_part);
          ASSERT((current_cut - max_gain) == metrics::hyperedgeCut(_hg),
                 "cut=" << current_cut - max_gain << "!=" << metrics::hyperedgeCut(_hg));
          _hg.changeNodePart(max_gain_node, to_part, from_part);
          return true;
        } ()
             , "max_gain move does not correspond to expected cut!");

      moveHypernode(max_gain_node, from_part, to_part);
      _marked[max_gain_node] = true;

      if (_hg.partWeight(to_part) >= max_allowed_part_weight) {
        _pq.disablePart(to_part);
      }
      if (_hg.partWeight(from_part) < max_allowed_part_weight) {
        _pq.enablePart(from_part);
      }

      current_imbalance = (static_cast<double>(std::max(_hg.partWeight(0), _hg.partWeight(1))) /
                           (ceil((_hg.partWeight(0) + _hg.partWeight(1)) / 2.0))) - 1.0;


      current_cut -= max_gain;
      StoppingPolicy::updateStatistics(max_gain);

      ASSERT(current_cut == metrics::hyperedgeCut(_hg),
             V(current_cut) << V(metrics::hyperedgeCut(_hg)));
      ASSERT(current_imbalance == metrics::imbalance(_hg),
             V(current_imbalance) << V(metrics::imbalance(_hg)));

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
        StoppingPolicy::resetStatistics();
        min_cut_index = num_moves;
        num_moves_since_last_improvement = 0;
      }
      _performed_moves[num_moves] = max_gain_node;
      ++num_moves;
    }
    DBG(dbg_refinement_2way_fm_stopping_crit, "KWayFM performed " << num_moves
        << " local search movements ( min_cut_index=" << min_cut_index << "): stopped because of "
        << (StoppingPolicy::searchShouldStop(num_moves_since_last_improvement, _config, beta,
                                             best_cut, current_cut)
            == true ? "policy" : "empty queue"));

    rollback(num_moves - 1, min_cut_index);
    ASSERT(best_cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_cut);
    return FMImprovementPolicy::improvementFound(best_cut, initial_cut, best_imbalance,
                                                 initial_imbalance, _config.partition.epsilon);
  }


  void updateNeighbours(const HypernodeID moved_hn, const PartitionID from_part,
                        const PartitionID to_part, const HypernodeWeight max_allowed_part_weight) {
    _just_activated.assign(_just_activated.size(), false);
    for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
      if (_locked_hes.get(he) != kLocked) {
        if (_locked_hes.get(he) == to_part) {
          // he is loose
          // TODO(schlag): We could provide a version similar to KFM that does not check for activation.
          fullUpdate(moved_hn, from_part, to_part, he, max_allowed_part_weight);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " maintained state: loose");
        } else if (_locked_hes.get(he) == kFree) {
          // he is free.
          fullUpdate(moved_hn, from_part, to_part, he, max_allowed_part_weight);
          _locked_hes.set(he, to_part);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " changed state: free -> loose");
        } else {
          // he is loose and becomes locked after the move
          fullUpdate(moved_hn, from_part, to_part, he, max_allowed_part_weight);
          _locked_hes.set(he, kLocked);
          DBG(dbg_refinement_2way_locked_hes, "HE " << he << " changed state: loose -> locked");
        }
      } else {
        // he is locked
        // In case of 2-FM, nothing to do here
        DBG(dbg_refinement_2way_locked_hes, he << " is locked");
      }
    }

    ASSERT([&]() {
        for (const HyperedgeID he : _hg.incidentEdges(moved_hn)) {
          for (const HypernodeID pin : _hg.pins(he)) {
            const PartitionID other_part = _hg.partID(pin) ^ 1;
            if (!isBorderNode(pin)) {
              // The pin is an internal HN
              if (_pq.contains(pin, other_part)) {
                LOG("HN " << pin << " should not be contained in PQ");
                return false;
              }
            } else {
              // HN is border HN
              if (_pq.contains(pin, other_part)) {
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
  void fullUpdate(const HypernodeID moved_hn, const PartitionID from_part,
                  const PartitionID to_part, const HyperedgeID he,
                  const HypernodeWeight max_allowed_part_weight) noexcept {
    const HypernodeID pin_count_from_part_after_move = _hg.pinCountInPart(he, from_part);
    const HypernodeID pin_count_to_part_after_move = _hg.pinCountInPart(he, to_part);
    const HypernodeID he_size = _hg.edgeSize(he);

    if (!_he_fully_active[he] || moveAffectsGainUpdate(pin_count_from_part_after_move,
                                                       pin_count_to_part_after_move,
                                                       he_size)) {
      Gain factor = 0;
      if (he_size == 2) {
        factor = (_hg.pinCountInPart(he, 0) == 1 ? 2 : -2);
      } else if (pin_count_to_part_after_move == 1) {
        // HE was an internal edge before move and is a cut HE now.
        // Before the move, all pins had gain -w(e). Now after the move,
        // these pins have gain 0 (since all of them are in from_part).
        factor = 1;
      } else if (pin_count_from_part_after_move == 0) {
        // HE was cut HE before move and is internal HE now.
        // Since the HE is now internal, moving a pin incurs gain -w(e)
        // and make it a cut HE again.
        factor = -1;
      }

      HypernodeID num_active_pins = 0;
      for (const HypernodeID pin : _hg.pins(he)) {
        if (pin == moved_hn) {
          continue;
        }

        const PartitionID target_part = _hg.partID(pin) ^ 1;
        if (!_marked[pin]) {
          if (!_pq.contains(pin, target_part)) {
            activate(pin, max_allowed_part_weight);
          } else {
            if (!isBorderNode(pin)) {
              DBG(dbg_refinement_2way_fm_gain_update, "TwoWayFM deleting pin " << pin << " from PQ "
                  << to_part);
              if (_pq.contains(pin, target_part)) {
                _pq.remove(pin, target_part);
              }
            } else {
              if (factor == 0) {
                if (pin_count_from_part_after_move == 1 && _hg.partID(pin) == from_part) {
                  // Before move, there were two pins (moved_node and the current pin) in from_part.
                  // After moving moved_node to to_part, the gain of the remaining pin in
                  // from_part increases by w(he).
                  factor = 1;  // after all pins are activated, one could break here
                }
                if (pin_count_to_part_after_move == 2 && _hg.partID(pin) == to_part) {
                  // Before move, pin was the only HN in to_part. It thus had a
                  // positive gain, because moving it to from_part would have removed
                  // the HE from the cut. Now, after the move, pin becomes a 0-gain HN
                  // because now there are pins in both parts.
                  factor = -1;  // after all pins are activated, one could break here
                }
              }
              if (factor != 0) {
                updatePin(he, pin, factor, max_allowed_part_weight);
              }
              // Otherwise delta-gain would be zero and zero delta-gain updates are bad.
              // See for example [CadKaMa99]
            }
          }
        }
        num_active_pins += _pq.contains(pin, _hg.partID(pin) ^ 1);
      }
      _he_fully_active[he] = num_active_pins == he_size;
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

  const Stats & statsImpl() const noexcept final {
    return _stats;
  }

  void updatePin(const HyperedgeID he, const HypernodeID pin, const Gain factor,
                 const HypernodeWeight max_allowed_part_weight) noexcept {
    ONLYDEBUG(max_allowed_part_weight);
    const PartitionID target_part = _hg.partID(pin) ^ 1;
    ASSERT(_pq.contains(pin, target_part), V(pin) << V(target_part));
    ASSERT(factor != 0, V(factor));
    ASSERT(!_marked[pin],
           " Trying to update marked HN " << pin << " in PQ " << target_part);
    ASSERT(isBorderNode(pin), "Trying to update non-border HN " << pin << " PQ=" << target_part);
    ASSERT((_hg.partWeight(target_part) < max_allowed_part_weight ?
            _pq.isEnabled(target_part) : !_pq.isEnabled(target_part)), V(target_part));
    if (!_just_activated[pin]) {
      const Gain old_gain = _pq.key(pin, target_part);
      const Gain gain_delta = factor * _hg.edgeWeight(he);
      DBG(dbg_refinement_2way_fm_gain_update, "TwoWayFM updating gain of HN " << pin
          << " from gain " << old_gain << " to " << old_gain + gain_delta << " in PQ "
          << target_part);
      _pq.updateKey(pin, target_part, computeGain(pin));
    }
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
  std::vector<bool> _marked;
  std::vector<bool> _just_activated;
  std::vector<bool> _he_fully_active;
  std::vector<HypernodeID> _performed_moves;
  FastResetVector<char> _locked_hes;
  Stats _stats;
};

template <class T, class U>
const char TwoWayFMRefiner<T, U>::kFree;

#pragma GCC diagnostic pop
}                                   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_TWOWAYFMREFINER_H_
