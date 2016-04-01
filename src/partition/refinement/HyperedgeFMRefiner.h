/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "external/fp_compare/Utils.h"
#include "lib/TemplateParameterToString.h"
#include "lib/core/Mandatory.h"
#include "lib/datastructure/FastResetBitVector.h"
#include "lib/datastructure/heaps/NoDataBinaryMaxHeap.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/refinement/FMRefinerBase.h"
#include "partition/refinement/IRefiner.h"
#include "partition/refinement/policies/FMImprovementPolicies.h"
#include "partition/refinement/policies/FMQueueCloggingPolicies.h"
#include "partition/refinement/policies/FMQueueSelectionPolicies.h"
#include "tools/RandomFunctions.h"

using datastructure::NoDataBinaryMaxHeap;
using datastructure::FastResetBitVector;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::PartitionID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;

namespace partition {
static const bool dbg_refinement_he_fm_he_activation = false;
static const bool dbg_refinement_he_fm_gain_computation = false;
static const bool dbg_refinement_he_fm_update_level = false;
static const bool dbg_refinement_he_fm_update_evaluated = false;
static const bool dbg_refinement_he_fm_update_locked = false;
static const bool dbg_refinement_he_fm_update_cases = false;
static const bool dbg_refinement_he_fm_improvements = true;
static const bool dbg_refinement_he_fm_rollback = false;
static const bool dbg_refinement_he_fm_remove_clogging = false;
static const bool dbg_refinement_he_fm_eligible_pqs = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
template <class StoppingPolicy = Mandatory,
          bool global_rebalancing = false,
          class FMImprovementPolicy = CutDecreasedOrInfeasibleImbalanceDecreased>
class HyperedgeFMRefiner final : public IRefiner,
                                 private FMRefinerBase {
 private:
  using HyperedgeFMHeap = NoDataBinaryMaxHeap<HyperedgeID, HyperedgeWeight,
                                              std::numeric_limits<HyperedgeWeight> >;
  using HyperedgeFMPQ = HyperedgeFMHeap;

  static const int K = 2;

  class HyperedgeEvalIndicator {
 public:
    explicit HyperedgeEvalIndicator(HyperedgeID size) noexcept :
      _bitvector(size, false) { }

    void markAsEvaluated(HyperedgeID id) noexcept {
      ASSERT(!_bitvector[id], "HE " << id << " is already marked as evaluated");
      DBG(false, "Marking HE " << id << " as evaluated");
      _bitvector.setBit(id, true);
    }

    bool isAlreadyEvaluated(HyperedgeID id) const {
      return _bitvector[id];
    }

    void reset() noexcept {
      _bitvector.resetAllBitsToFalse();
    }

 private:
    FastResetBitVector<> _bitvector;
  };

 public:
  HyperedgeFMRefiner(Hypergraph& hypergraph, const Configuration& config) noexcept :
    FMRefinerBase(hypergraph, config),
    _pq{new HyperedgeFMPQ(_hg.initialNumEdges()), new HyperedgeFMPQ(_hg.initialNumEdges())},
    _marked_HEs(_hg.initialNumEdges(), false),
    _gain_indicator(_hg.initialNumEdges()),
    _update_indicator(_hg.initialNumEdges()),
    _contained_hypernodes(_hg.initialNumNodes(), false),
    _movement_indices(),
    _performed_moves(),
    _stopping_policy() {
    _movement_indices.reserve(_hg.initialNumEdges() + 1);
    _movement_indices[0] = 0;
    _performed_moves.reserve(_hg.initialNumPins());
  }

  ~HyperedgeFMRefiner() {
    delete _pq[0];
    delete _pq[1];
  }

  HyperedgeFMRefiner(const HyperedgeFMRefiner&) = delete;
  HyperedgeFMRefiner& operator= (const HyperedgeFMRefiner&) = delete;

  HyperedgeFMRefiner(HyperedgeFMRefiner&&) = delete;
  HyperedgeFMRefiner& operator= (HyperedgeFMRefiner&&) = delete;

  void initializeImpl(HyperedgeWeight) noexcept override final {
    if (!_is_initialized) {
      _is_initialized = true;
    }
  }

  void initializeImpl() noexcept override final {
    if (!_is_initialized) {
      _is_initialized = true;
    }
  }

  void activateIncidentCutHyperedges(HypernodeID hn) noexcept {
    DBG(false, "activating cut hyperedges of hypernode " << hn);
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      if (isCutHyperedge(he) && !isMarkedAsMoved(he)) {
        activateHyperedge(he);
      }
    }
  }

  bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                  const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                  const UncontractionGainChanges& UNUSED(changes),
                  Metrics& best_metrics) noexcept override final {
    ONLYDEBUG(max_allowed_part_weights);
    ASSERT(_is_initialized, "initialize() has to be called before refine");
    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg),
           "initial best_cut " << best_metrics.cut << "does not equal cut induced by hypergraph "
           << metrics::hyperedgeCut(_hg));
    ASSERT(FloatingPoint<double>(best_metrics.imbalance).AlmostEquals(
             FloatingPoint<double>(calculateImbalance())),
           "initial best_imbalance " << best_metrics.imbalance << "does not equal imbalance induced"
           << " by hypergraph " << calculateImbalance());

    _pq[0]->clear();
    _pq[1]->clear();
    resetMarkedHyperedges();

    Randomize::shuffleVector(refinement_nodes, refinement_nodes.size());
    for (const HypernodeID hn : refinement_nodes) {
      activateIncidentCutHyperedges(hn);
    }
    // DBG(true, "------pq[0].size=" << _pq[0]->size() << "------------_pq[1]->size()"
    //    << _pq[1]->size() << "----------------------");

#ifndef NDEBUG
    HyperedgeWeight initial_cut = best_metrics.cut;
#endif

    HyperedgeWeight cut = best_metrics.cut;
    int min_cut_index = -1;
    double imbalance = best_metrics.imbalance;

    int step = 0;
    int num_moves_since_last_improvement = 0;
    _stopping_policy.resetStatistics();
    const double beta = log(_hg.numNodes());
    while (!queuesAreEmpty() && (best_metrics.cut == cut ||
                                 !_stopping_policy.searchShouldStop(num_moves_since_last_improvement,
                                                                    _config, beta, best_metrics.cut, cut))) {
      ASSERT(cut == metrics::hyperedgeCut(_hg),
             "Precondition failed: calculated cut (" << cut << ") and cut induced by hypergraph ("
             << metrics::hyperedgeCut(_hg) << ") do not match");

      bool pq0_eligible = false;
      bool pq1_eligible = false;
      checkPQsForEligibleMoves(pq0_eligible, pq1_eligible);

      // TODO(schlag):
      // [ ] We might consider the removal of HEs from PQs as one step that lead to no
      //     improvement. Thus we might need to "increment step by 1". However we'd need
      //     another counting variable that only counts the loop iterations to do that, since
      //     step currently refers to the number of actual moves that are performed.
      if (OnlyRemoveIfBothQueuesClogged::removeCloggingQueueEntries(pq0_eligible, pq1_eligible,
                                                                    _pq[0], _pq[1])) {
        continue;
      }

      // TODO(schlag):
      // [ ] think about selection strategies and tiebreaking
      bool chosen_pq_index = selectQueue(pq0_eligible, pq1_eligible);
      Gain max_gain = _pq[chosen_pq_index]->getMaxKey();
      HyperedgeID max_gain_hyperedge = _pq[chosen_pq_index]->getMax();
      _pq[chosen_pq_index]->deleteMax();
      PartitionID from_partition = chosen_pq_index;
      PartitionID to_partition = chosen_pq_index ^ 1;

      ASSERT(!isMarkedAsMoved(max_gain_hyperedge), "Selection strategy failed");

      DBG(false, "HER-FM moving HE" << max_gain_hyperedge << " from " << from_partition
          << " to " << to_partition << " (gain: " << max_gain << ")");


      moveHyperedge(max_gain_hyperedge, from_partition, to_partition, step);

      cut -= max_gain;
      imbalance = calculateImbalance();
      _stopping_policy.updateStatistics(max_gain);

      ASSERT(cut == metrics::hyperedgeCut(_hg),
             "Calculated cut (" << cut << ") and cut induced by hypergraph ("
             << metrics::hyperedgeCut(_hg) << ") do not match");

      updateNeighbours(max_gain_hyperedge);

      // right now, we do not allow a decrease in cut in favor of an increase in balance
      bool improved_cut_within_balance = (cut < best_metrics.cut) &&
                                         (imbalance < _config.partition.epsilon);
      bool improved_balance_equal_cut = (imbalance < best_metrics.imbalance) && (cut <= best_metrics.cut);

      ++num_moves_since_last_improvement;
      if (improved_balance_equal_cut || improved_cut_within_balance) {
        ASSERT(cut <= best_metrics.cut, "Accepted a HE move which decreased cut");
        if (cut < best_metrics.cut) {
          DBG(dbg_refinement_he_fm_improvements,
              "HER-FM improved cut from " << best_metrics.cut << " to " << cut);
        }
        DBG(dbg_refinement_he_fm_improvements,
            "HER-FM improved imbalance from " << best_metrics.imbalance << " to " << imbalance);
        best_metrics.imbalance = imbalance;
        best_metrics.cut = cut;
        min_cut_index = step;
        _stopping_policy.resetStatistics();
        num_moves_since_last_improvement = 0;
      }
      ++step;
    }

    rollback(step - 1, min_cut_index, _hg);

    ASSERT(best_metrics.cut == metrics::hyperedgeCut(_hg), "Incorrect rollback operation");
    ASSERT(best_metrics.cut <= initial_cut, "Cut quality decreased from "
           << initial_cut << " to" << best_metrics.cut);
    if (min_cut_index != -1) {
      return true;
    }
    return false;
  }

  Gain computeGain(HyperedgeID he, PartitionID from, PartitionID to) noexcept {
    ONLYDEBUG(to);
    ASSERT((from < 2) && (to < 2), "Trying to compute gain for PartitionIndex >= 2");
    ASSERT((from != Hypergraph::kInvalidPartition) && (to != Hypergraph::kInvalidPartition),
           "Trying to compute gain for invalid partition");
    if (isCutHyperedge(he)) {
      _gain_indicator.reset();
      Gain gain = _hg.edgeWeight(he);
      for (const HypernodeID pin : _hg.pins(he)) {
        if (_hg.partID(pin) != from) { continue; }
        DBG(dbg_refinement_he_fm_gain_computation, "evaluating pin " << pin);
        for (const HyperedgeID incident_he : _hg.incidentEdges(pin)) {
          if (incident_he == he || _gain_indicator.isAlreadyEvaluated(incident_he)) { continue; }
          if (!isCutHyperedge(incident_he) &&
              !isNestedIntoInPartition(incident_he, he, from)) {
            gain -= _hg.edgeWeight(incident_he);
            DBG(dbg_refinement_he_fm_gain_computation,
                "pin " << pin << " HE: " << incident_he << " gain-="
                << _hg.edgeWeight(incident_he) << ": " << gain);
          } else if (isCutHyperedge(incident_he) &&
                     isNestedIntoInPartition(incident_he, he, from)) {
            gain += _hg.edgeWeight(incident_he);
            DBG(dbg_refinement_he_fm_gain_computation,
                "pin " << pin << " HE: " << incident_he << " g+="
                << _hg.edgeWeight(incident_he) << ": " << gain);
          }
          _gain_indicator.markAsEvaluated(incident_he);
        }
      }
      return gain;
    } else {
      return 0;
    }
  }

  bool isNestedIntoInPartition(HyperedgeID inner_he, HyperedgeID outer_he,
                               PartitionID relevant_partition) noexcept {
    if (_hg.pinCountInPart(inner_he, relevant_partition) >
        _hg.pinCountInPart(outer_he, relevant_partition)) {
      return false;
    }
    resetContainedHypernodes();
    for (const HypernodeID pin : _hg.pins(outer_he)) {
      if (_hg.partID(pin) == relevant_partition) {
        markAsContained(pin);
      }
    }
    for (const HypernodeID pin : _hg.pins(inner_he)) {
      if (_hg.partID(pin) == relevant_partition && !isContained(pin)) {
        return false;
      }
    }
    return true;
  }

  int numRepetitionsImpl() const noexcept override final {
    return _config.her_fm.num_repetitions;
  }

  std::string policyStringImpl() const noexcept override final {
    return std::string(templateToString<EligibleTopGain<Gain> >()
                       + templateToString<OnlyRemoveIfBothQueuesClogged>()
                       + templateToString<StoppingPolicy>());
  }

 private:
  FRIEND_TEST(AHyperedgeFMRefiner, MaintainsSizeOfPartitionsWhichAreInitializedByCallingInitialize);
  FRIEND_TEST(AHyperedgeFMRefiner, ActivatesOnlyCutHyperedgesByInsertingThemIntoPQ);
  FRIEND_TEST(AHyperedgeFMRefiner, ChecksIfHyperedgeMovePreservesBalanceConstraint);
  FRIEND_TEST(AHyperedgeFMRefiner, ChoosesHyperedgeWithHighestGainAsNextMove);
  FRIEND_TEST(AHyperedgeFMRefiner, RemovesHyperedgeMovesFromPQsIfBothPQsAreNotEligible);
  FRIEND_TEST(AHyperedgeMovementOperation, UpdatesPartitionSizes);
  FRIEND_TEST(AHyperedgeMovementOperation, DeletesTheRemaningPQEntry);
  FRIEND_TEST(AHyperedgeMovementOperation, LocksHyperedgeAfterPinsAreMoved);
  FRIEND_TEST(AHyperedgeMovementOperation, ChoosesTheMaxGainMoveFromEligiblePQ);
  FRIEND_TEST(AHyperedgeMovementOperation, ChoosesTheMaxGainMoveIfBothPQsAreEligible);
  FRIEND_TEST(TheUpdateGainsMethod, IgnoresLockedHyperedges);
  FRIEND_TEST(TheUpdateGainsMethod, EvaluatedEachHyperedgeOnlyOnce);
  FRIEND_TEST(TheUpdateGainsMethod, RemovesHyperedgesThatAreNoLongerCutHyperedgesFromPQs);
  FRIEND_TEST(TheUpdateGainsMethod, ActivatesHyperedgesThatBecameCutHyperedges);
  FRIEND_TEST(TheUpdateGainsMethod, RecomputesGainForHyperedgesThatRemainCutHyperedges);
  FRIEND_TEST(RollBackInformation, StoresIDsOfMovedPins);
  FRIEND_TEST(RollBackInformation, IsUsedToRollBackMovementsToGivenIndex);
  FRIEND_TEST(RollBackInformation, IsUsedToRollBackMovementsToInitialStateIfNoImprovementWasFound);

  void rollback(int last_index, int min_cut_index, Hypergraph& hg) noexcept {
    ASSERT(min_cut_index <= last_index, "Min-Cut index " << min_cut_index << " out of bounds");
    DBG(dbg_refinement_he_fm_rollback, "min_cut_index = " << min_cut_index);
    HypernodeID hn_to_move;
    while (last_index != min_cut_index) {
      DBG(dbg_refinement_he_fm_rollback, "last_index = " << last_index);
      for (size_t i = _movement_indices[last_index]; i < _movement_indices[last_index + 1]; ++i) {
        hn_to_move = _performed_moves[i];
        DBG(dbg_refinement_he_fm_rollback, "Moving HN " << hn_to_move << " from "
            << hg.partID(hn_to_move) << " back to " << (hg.partID(hn_to_move) ^ 1));
        _hg.changeNodePart(hn_to_move, hg.partID(hn_to_move),
                           (hg.partID(hn_to_move) ^ 1));
      }
      --last_index;
    }
  }

  double calculateImbalance() const noexcept {
    double imbalance = (2.0 * std::max(_hg.partWeight(0), _hg.partWeight(1))) /
                       (_hg.partWeight(0) + _hg.partWeight(1)) - 1.0;
    ASSERT(FloatingPoint<double>(imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg, _config))),
           "imbalance calculation inconsistent beween fm-refiner and hypergraph");
    return imbalance;
  }

  void checkPQsForEligibleMoves(bool& pq0_eligible, bool& pq1_eligible) const noexcept {
    pq0_eligible = !_pq[0]->empty() && movePreservesBalanceConstraint(_pq[0]->getMax(), 0, 1);
    pq1_eligible = !_pq[1]->empty() && movePreservesBalanceConstraint(_pq[1]->getMax(), 1, 0);
  }

  bool selectQueue(bool pq0_eligible, bool pq1_eligible) noexcept {
    ASSERT(!_pq[0]->empty() || !_pq[1]->empty(), "Trying to choose next move with empty PQs");
    ASSERT(pq0_eligible || pq1_eligible, "Both PQs contain non-eligible moves!");
    DBG(dbg_refinement_he_fm_eligible_pqs, "HE " << _pq[0]->getMax() << " is "
        << (pq0_eligible ? "" : " NOT ") << " eligable in PQ0, gain=" << _pq[0]->getMaxKey());
    DBG(dbg_refinement_he_fm_eligible_pqs, "HE " << _pq[1]->getMax() << " is "
        << (pq1_eligible ? "" : " NOT ") << " eligable in PQ1, gain=" << _pq[1]->getMaxKey());
    return EligibleTopGain<Gain>::selectQueue(pq0_eligible, pq1_eligible, _pq[0], _pq[1]);
  }

  bool movePreservesBalanceConstraint(HyperedgeID he, PartitionID from,
                                      PartitionID to) const noexcept {
    HypernodeWeight pins_to_move_weight = 0;
    for (const HypernodeID pin : _hg.pins(he)) {
      if (_hg.partID(pin) == from) {
        pins_to_move_weight += _hg.nodeWeight(pin);
      }
    }
    return _hg.partWeight(to) + pins_to_move_weight <= _config.partition.max_part_weights[0];
  }

  bool queuesAreEmpty() const noexcept {
    return _pq[0]->empty() && _pq[1]->empty();
  }

  bool isCutHyperedge(HyperedgeID he) const noexcept {
    return _hg.pinCountInPart(he, 0) * _hg.pinCountInPart(he, 1) > 0;
  }

  void activateHyperedge(HyperedgeID he) noexcept {
    ASSERT(!isMarkedAsMoved(he),
           "HE " << he << " has already been moved and cannot be activated again");
    // we have to use insert because PQs will be reused during each uncontraction
    if (!_pq[0]->contains(he) && movePreservesBalanceConstraint(he, 0, 1)) {
      _pq[0]->push(he, computeGain(he, 0, 1));
      DBG(dbg_refinement_he_fm_he_activation,
          "inserting HE " << he << " with gain " << _pq[0]->getKey(he) << " in PQ " << 0);
    }
    if (!_pq[1]->contains(he) && movePreservesBalanceConstraint(he, 1, 0)) {
      _pq[1]->push(he, computeGain(he, 1, 0));
      DBG(dbg_refinement_he_fm_he_activation,
          "inserting HE " << he << " with gain " << _pq[1]->getKey(he) << " in PQ " << 1);
    }
  }

  void moveHyperedge(HyperedgeID he, PartitionID from, PartitionID to, int step) noexcept {
    int curr_index = _movement_indices[step];
    for (const HypernodeID pin : _hg.pins(he)) {
      if (_hg.partID(pin) == from) {
        _hg.changeNodePart(pin, from, to);
        _performed_moves[curr_index++] = pin;
      }
    }
    if (_pq[to]->contains(he)) {
      _pq[to]->deleteNode(he);
    }
    markAsMoved(he);
    // set sentinel
    _movement_indices[step + 1] = curr_index;
  }

  void updateNeighbours(HyperedgeID moved_he) noexcept {
    _update_indicator.reset();
    for (const HypernodeID pin : _hg.pins(moved_he)) {
      DBG(dbg_refinement_he_fm_update_level, "--->Considering PIN " << pin);
      for (const HyperedgeID incident_he : _hg.incidentEdges(pin)) {
        if (incident_he == moved_he) { continue; }
        DBG(dbg_refinement_he_fm_update_level, "-->Considering incident HE "
            << incident_he << "of PIN " << pin);
        for (const HypernodeID incident_he_pin : _hg.pins(incident_he)) {
          if (incident_he_pin == pin) { continue; }
          DBG(dbg_refinement_he_fm_update_level, "->Considering incident_he_pin "
              << incident_he_pin << " of HE " << incident_he);
          recomputeGainsForIncidentCutHyperedges(incident_he_pin);
        }
      }
    }
  }

  void recomputeGainsForIncidentCutHyperedges(HypernodeID hn) noexcept {
    for (const HyperedgeID he : _hg.incidentEdges(hn)) {
      if (_update_indicator.isAlreadyEvaluated(he)) {
        DBG(dbg_refinement_he_fm_update_evaluated,
            "*** Skipping HE " << he << " because it is already evaluated!");
        continue;
      }
      if (isMarkedAsMoved(he)) {
        ASSERT(!_pq[0]->contains(he), "HE " << he << "should not be present in PQ 0");
        ASSERT(!_pq[1]->contains(he), "HE " << he << "should not be present in PQ 1");
        DBG(dbg_refinement_he_fm_update_locked, "HE " << he << " is locked");
        _update_indicator.markAsEvaluated(he);
        continue;
      }
      DBG(dbg_refinement_he_fm_update_level,
          " Recomputing Gains for HE " << he << "  incident to HN " << hn);
      if (wasCutHyperedgeBeforeMove(he)) {
        if (isCutHyperedge(he)) {
          recomputeGainsForCutHyperedge(he);
        } else {
          removeNonCutHyperedgeFromQueues(he);
        }
      } else if (isCutHyperedge(he) && !isMarkedAsMoved(he)) {
        DBG(dbg_refinement_he_fm_update_cases,
            " Activating HE " << he << " because it has become a cut hyperedge");
        activateHyperedge(he);
      }
      _update_indicator.markAsEvaluated(he);
    }
  }

  void removeNonCutHyperedgeFromQueues(HyperedgeID he) noexcept {
    ASSERT(!isMarkedAsMoved(he), "Trying to recompute gains for already moved HE " << he);
    ASSERT(_pq[0]->contains(he) || _pq[1]->contains(he),
           "Trying to remove a HE (" << he << ") which is not active");
    if (_pq[0]->contains(he)) {
      DBG(dbg_refinement_he_fm_update_cases,
          " Removing HE " << he << " from PQ 0 because it is no longer a cut hyperedge");
      _pq[0]->deleteNode(he);
    }
    if (_pq[1]->contains(he)) {
      DBG(dbg_refinement_he_fm_update_cases,
          " Removing HE " << he << " from PQ 1 because it is no longer a cut hyperedge");
      _pq[1]->deleteNode(he);
    }
  }

  bool wasCutHyperedgeBeforeMove(HyperedgeID he) const noexcept {
    return _pq[0]->contains(he) || _pq[1]->contains(he);
  }

  void recomputeGainsForCutHyperedge(HyperedgeID he) noexcept {
    ASSERT(!isMarkedAsMoved(he), "Trying to recompute gains for already moved HE " << he);
    ASSERT(_pq[0]->contains(he) || _pq[1]->contains(he),
           "Trying to remove a HE (" << he << ") which is not active");
    if (_pq[0]->contains(he)) {
      DBG(dbg_refinement_he_fm_update_cases,
          " Recomputing Gain for HE PQ0 " << he << " which still is a cut hyperedge: "
          << _pq[0]->getKey(he) << " --> " << computeGain(he, 0, 1));
      _pq[0]->updateKey(he, computeGain(he, 0, 1));
    }
    if (_pq[1]->contains(he)) {
      DBG(dbg_refinement_he_fm_update_cases,
          " Recomputing Gain for HE PQ1 " << he << " which still is a cut hyperedge: "
          << _pq[1]->getKey(he) << " --> " << computeGain(he, 1, 0));
      _pq[1]->updateKey(he, computeGain(he, 1, 0));
    }
  }

  void markAsContained(HypernodeID hn) noexcept {
    ASSERT(!_contained_hypernodes[hn], "HN " << hn << " is already marked as contained");
    _contained_hypernodes.setBit(hn, true);
  }

  bool isContained(HypernodeID hn) const noexcept {
    return _contained_hypernodes[hn];
  }

  void resetContainedHypernodes() noexcept {
    _contained_hypernodes.resetAllBitsToFalse();
  }

  bool isMarkedAsMoved(HyperedgeID he) const noexcept {
    return _marked_HEs[he];
  }

  void markAsMoved(HyperedgeID he) noexcept {
    _marked_HEs.setBit(he, true);
  }

  void resetMarkedHyperedges() noexcept {
    _marked_HEs.resetAllBitsToFalse();
  }

  using FMRefinerBase::_hg;
  using FMRefinerBase::_config;
  std::array<HyperedgeFMPQ*, K> _pq;
  FastResetBitVector<> _marked_HEs;
  HyperedgeEvalIndicator _gain_indicator;
  HyperedgeEvalIndicator _update_indicator;
  FastResetBitVector<> _contained_hypernodes;
  std::vector<size_t> _movement_indices;
  std::vector<HypernodeID> _performed_moves;
  StoppingPolicy _stopping_policy;
};
#pragma GCC diagnostic pop
}   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
