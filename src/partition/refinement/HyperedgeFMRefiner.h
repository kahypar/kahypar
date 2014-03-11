/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_

#include <boost/dynamic_bitset.hpp>

#include <algorithm>
#include <limits>
#include <vector>

#include "external/fp_compare/Utils.h"
#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/refinement/FMStopPolicies.h"
#include "partition/refinement/IRefiner.h"
#include "tools/RandomFunctions.h"

using defs::INVALID_PARTITION;
using datastructure::PriorityQueue;

namespace partition {
static const bool dbg_refinement_he_fm_he_activation = true;
static const bool dbg_refinement_he_fm_gain_computation = false;
static const bool dbg_refinement_he_fm_update_level = false;
static const bool dbg_refinement_he_fm_update_evaluated = true;
static const bool dbg_refinement_he_fm_update_locked = true;
static const bool dbg_refinement_he_fm_update_cases = true;
static const bool dbg_refinement_he_fm_improvements = true;
static const bool dbg_refinement_he_fm_rollback = true;

template <class Hypergraph, class StoppingPolicy>
class HyperedgeFMRefiner : public IRefiner<Hypergraph>{
  private:
  typedef typename Hypergraph::HypernodeID HypernodeID;
  typedef typename Hypergraph::HyperedgeID HyperedgeID;
  typedef typename Hypergraph::PartitionID PartitionID;
  typedef typename Hypergraph::HyperedgeWeight HyperedgeWeight;
  typedef typename Hypergraph::HypernodeWeight HypernodeWeight;
  typedef HyperedgeWeight Gain;
  typedef PriorityQueue<HyperedgeID, HyperedgeWeight,
                        std::numeric_limits<HyperedgeWeight> > HyperedgeFMPQ;

  static const int K = 2;

  class HyperedgeEvalIndicator {
    public:
    explicit HyperedgeEvalIndicator(HyperedgeID size) :
      _bitvector(size) { }

    void markAsEvaluated(HyperedgeID id) {
      ASSERT(!_bitvector[id], "HE " << id << " is already marked as evaluated");
      DBG(false, "Marking HE " << id << " as evaluated");
      _bitvector[id] = 1;
    }

    bool isAlreadyEvaluated(HyperedgeID id) const {
      return _bitvector[id];
    }

    void reset() {
      _bitvector.reset();
    }

    private:
    boost::dynamic_bitset<uint64_t> _bitvector;
  };

  public:
  HyperedgeFMRefiner(Hypergraph& hypergraph, const Configuration<Hypergraph>& config) :
    _hg(hypergraph),
    _config(config),
    _partition_size{0, 0},
    _pq{new HyperedgeFMPQ(_hg.initialNumNodes()), new HyperedgeFMPQ(_hg.initialNumNodes())},
    _locked_hyperedges(_hg.initialNumEdges()),
    _gain_indicator(_hg.initialNumEdges()),
    _update_indicator(_hg.initialNumEdges()),
    _contained_hypernodes(_hg.initialNumNodes()),
    _movement_indices(),
    _performed_moves(),
    _is_initialized(false) {
    _movement_indices.reserve(_hg.initialNumEdges() + 1);
    _movement_indices[0] = 0;
    _performed_moves.reserve(_hg.initialNumPins());
  }

  ~HyperedgeFMRefiner() {
    delete _pq[0];
    delete _pq[1];
  }

  void initialize() {
    _partition_size[0] = 0;
    _partition_size[1] = 0;
    forall_hypernodes(hn, _hg) {
      ASSERT(_hg.partitionIndex(*hn) != INVALID_PARTITION,
             "TwoWayFmRefiner cannot work with HNs in invalid partition");
      _partition_size[_hg.partitionIndex(*hn)] += _hg.nodeWeight(*hn);
    } endfor

      ASSERT(_partition_size[0] + _partition_size[1] ==[&]() {
               HypernodeWeight total_weight = 0;
               forall_hypernodes(hn, _hg) {
                 total_weight += _hg.nodeWeight(*hn);
               } endfor
               return total_weight;
             } ()
             , "Calculated partition sizes do not match those induced by hypergraph");
    _is_initialized = true;
  }

  void activateIncidentCutHyperedges(HypernodeID hn) {
    DBG(true, "activating cut hyperedges of hypernode " << hn);
    forall_incident_hyperedges(he, hn, _hg) {
      // pq invariant: either a hyperedge is in both PQs or in none of them
      if (isCutHyperedge(*he) && !_pq[0]->contains(*he)) {
        activateHyperedge(*he);
      }
    } endfor
  }

  void refine(HypernodeID u, HypernodeID v, HyperedgeWeight& best_cut,
              double max_imbalance, double& best_imbalance) {
    _pq[0]->clear();
    _pq[1]->clear();
    resetLockedHyperedges();

    activateIncidentCutHyperedges(u);
    activateIncidentCutHyperedges(v);
  }

  Gain computeGain(HyperedgeID he, PartitionID from, PartitionID to) {
    ASSERT((from < 2) && (to < 2), "Trying to compute gain for PartitionIndex >= 2");
    ASSERT((from != INVALID_PARTITION) && (to != INVALID_PARTITION),
           "Trying to compute gain for invalid partition");
    if (isCutHyperedge(he)) {
      _gain_indicator.reset();
      Gain gain = _hg.edgeWeight(he);
      forall_pins(pin, he, _hg) {
        if (_hg.partitionIndex(*pin) != from) { continue; }
        DBG(dbg_refinement_he_fm_gain_computation, "evaluating pin " << *pin);
        forall_incident_hyperedges(incident_he, *pin, _hg) {
          if (*incident_he == he || _gain_indicator.isAlreadyEvaluated(*incident_he)) { continue; }
          if (!isCutHyperedge(*incident_he) &&
              !isNestedIntoInPartition(*incident_he, he, from)) {
            gain -= _hg.edgeWeight(*incident_he);
            DBG(dbg_refinement_he_fm_gain_computation,
                "pin " << *pin << " HE: " << *incident_he << " gain-=1: " << gain);
          } else if (isCutHyperedge(*incident_he) &&
                     isNestedIntoInPartition(*incident_he, he, from)) {
            gain += _hg.edgeWeight(*incident_he);
            DBG(dbg_refinement_he_fm_gain_computation,
                "pin " << *pin << " HE: " << *incident_he << " g+=1: " << gain);
          }
          _gain_indicator.markAsEvaluated(*incident_he);
        } endfor
      } endfor
      return gain;
    } else {
      return 0;
    }
  }

  bool isNestedIntoInPartition(HyperedgeID inner_he, HyperedgeID outer_he,
                               PartitionID relevant_partition) {
    resetContainedHypernodes();
    forall_pins(pin, outer_he, _hg) {
      if (_hg.partitionIndex(*pin) == relevant_partition) {
        markAsContained(*pin);
      }
    } endfor

    forall_pins(pin, inner_he, _hg) {
      if (_hg.partitionIndex(*pin) == relevant_partition && !isContained(*pin)) {
        return false;
      }
    } endfor
    return true;
  }

  private:
  FRIEND_TEST(AHyperedgeFMRefiner, MaintainsSizeOfPartitionsWhichAreInitializedByCallingInitialize);
  FRIEND_TEST(AHyperedgeFMRefiner, ActivatesOnlyCutHyperedgesByInsertingThemIntoPQ);
  FRIEND_TEST(AHyperedgeMovementOperation, UpdatesPartitionSizes);
  FRIEND_TEST(AHyperedgeMovementOperation, DeletesTheRemaningPQEntry);
  FRIEND_TEST(AHyperedgeMovementOperation, LocksHyperedgeAfterPinsAreMoved);
  FRIEND_TEST(TheUpdateGainsMethod, IgnoresLockedHyperedges);
  FRIEND_TEST(TheUpdateGainsMethod, EvaluatedEachHyperedgeOnlyOnce);
  FRIEND_TEST(TheUpdateGainsMethod, RemovesHyperedgesThatAreNoLongerCutHyperedgesFromPQs);
  FRIEND_TEST(TheUpdateGainsMethod, ActivatesHyperedgesThatBecameCutHyperedges);
  FRIEND_TEST(TheUpdateGainsMethod, RecomputesGainForHyperedgesThatRemainCutHyperedges);
  FRIEND_TEST(AHyperedgeFMRefiner, ChoosesHyperedgeWithHighestGainAsNextMove);
  FRIEND_TEST(RollBackInformation, StoresIDsOfMovedPins);
  FRIEND_TEST(RollBackInformation, IsUsedToRollBackMovementsToGivenIndex);
  FRIEND_TEST(RollBackInformation, IsUsedToRollBackMovementsToInitialStateIfNoImprovementWasFound);
  FRIEND_TEST(AHyperedgeFMRefiner, ChecksIfHyperedgeMovePreservesBalanceConstraint);
  FRIEND_TEST(AHyperedgeMovementOperation, ChoosesTheMaxGainMoveFromEligiblePQ);

  void rollback(int last_index, int min_cut_index, Hypergraph& hg) {
    DBG(dbg_refinement_he_fm_rollback, "min_cut_index = " << min_cut_index);
    HypernodeID hn_to_move;
    while (last_index != min_cut_index) {
      DBG(dbg_refinement_he_fm_rollback, "last_index = " << last_index);
      for (int i = _movement_indices[last_index]; i < _movement_indices[last_index + 1]; ++i) {
        hn_to_move = _performed_moves[i];
        DBG(dbg_refinement_he_fm_rollback, "Moving HN " << hn_to_move << " from "
            << hg.partitionIndex(hn_to_move) << " to " << (hg.partitionIndex(hn_to_move) ^ 1));
        _partition_size[hg.partitionIndex(hn_to_move)] -= _hg.nodeWeight(hn_to_move);
        _partition_size[(hg.partitionIndex(hn_to_move) ^ 1)] += _hg.nodeWeight(hn_to_move);
        _hg.changeNodePartition(hn_to_move, hg.partitionIndex(hn_to_move),
                                (hg.partitionIndex(hn_to_move) ^ 1));
      }
      --last_index;
    }
  }

  double calculateImbalance() const {
    double imbalance = (2.0 * std::max(_partition_size[0], _partition_size[1])) /
                       (_partition_size[0] + _partition_size[1]) - 1.0;
    ASSERT(FloatingPoint<double>(imbalance).AlmostEquals(
             FloatingPoint<double>(metrics::imbalance(_hg))),
           "imbalance calculation inconsistent beween fm-refiner and hypergraph");
    return imbalance;
  }

  void chooseNextMove(Gain& max_gain, HyperedgeID& max_gain_he, PartitionID& from_partition,
                      PartitionID& to_partition) {
    ASSERT(!_pq[0]->empty() || !_pq[1]->empty(), "Trying to choose next move with empty PQs");

    bool pq0_eligible = !_pq[0]->empty() && movePreservesBalanceConstraint(_pq[0]->max(), 0, 1);
    bool pq1_eligible = !_pq[1]->empty() && movePreservesBalanceConstraint(_pq[1]->max(), 1, 0);

    Gain gain_pq0 = std::numeric_limits<Gain>::min();
    if (pq0_eligible) {
      gain_pq0 = _pq[0]->maxKey();
    }

    bool chosen_pq_index = 0;
    if (pq1_eligible && ((_pq[1]->maxKey() > gain_pq0) ||
                         (_pq[1]->maxKey() == gain_pq0 && Randomize::flipCoin()))) {
      chosen_pq_index = 1;
    }

    max_gain = _pq[chosen_pq_index]->maxKey();
    max_gain_he = _pq[chosen_pq_index]->max();
    _pq[chosen_pq_index]->deleteMax();
    from_partition = chosen_pq_index;
    to_partition = chosen_pq_index ^ 1;
  }

  bool movePreservesBalanceConstraint(HyperedgeID he, PartitionID from, PartitionID to) {
    HypernodeWeight pins_to_move_weight = 0;
    forall_pins(pin, he, _hg) {
      if (_hg.partitionIndex(*pin) == from) {
        pins_to_move_weight += _hg.nodeWeight(*pin);
      }
    } endfor
    return _partition_size[to] + pins_to_move_weight <= _config.partitioning.partition_size_upper_bound;
  }

  bool queuesAreEmpty() const {
    return _pq[0]->empty() && _pq[1]->empty();
  }

  bool isCutHyperedge(HyperedgeID he) const {
    return _hg.pinCountInPartition(he, 0) * _hg.pinCountInPartition(he, 1) > 0;
  }

  void activateHyperedge(HyperedgeID he) {
    ASSERT(!isLocked(he), "HE " << he << " is already locked");
    ASSERT(!_pq[0]->contains(he), "HE " << he << " is already contained in PQ 0");
    ASSERT(!_pq[1]->contains(he), "HE " << he << " is already contained in PQ 1");
    // we have to use reInsert because PQs will be reused during each uncontraction
    _pq[0]->reInsert(he, computeGain(he, 0, 1));
    _pq[1]->reInsert(he, computeGain(he, 1, 0));
    DBG(dbg_refinement_he_fm_he_activation,
        "inserting HE " << he << " with gain " << _pq[0]->key(he) << " in PQ " << 0);
    DBG(dbg_refinement_he_fm_he_activation,
        "inserting HE " << he << " with gain " << _pq[1]->key(he) << " in PQ " << 1);
  }

  void moveHyperedge(HyperedgeID he, PartitionID from, PartitionID to, int step) {
    int curr_index = _movement_indices[step];
    forall_pins(pin, he, _hg) {
      if (_hg.partitionIndex(*pin) == from) {
        _hg.changeNodePartition(*pin, from, to);
        _partition_size[from] -= _hg.nodeWeight(*pin);
        _partition_size[to] += _hg.nodeWeight(*pin);
        _performed_moves[curr_index++] = *pin;
      }
    } endfor
      ASSERT(_pq[to]->contains(he) == true,
             "HE " << he << " does not exist in PQ " << to);
    _pq[to]->remove(he);
    lock(he);
    //set sentinel
    _movement_indices[step + 1] = curr_index;
  }

  void updateNeighbours(HyperedgeID moved_he) {
    _update_indicator.reset();
    forall_pins(pin, moved_he, _hg) {
      DBG(dbg_refinement_he_fm_update_level, "--->Considering PIN " << *pin);
      forall_incident_hyperedges(incident_he, *pin, _hg) {
        if (*incident_he == moved_he) { continue; }
        DBG(dbg_refinement_he_fm_update_level, "-->Considering incident HE "
            << *incident_he << "of PIN " << *pin);
        if (_update_indicator.isAlreadyEvaluated(*incident_he)) {
          DBG(dbg_refinement_he_fm_update_evaluated,
              "*** Skipping HE " << *incident_he << " because it is already evaluated!");
          continue;
        }
        forall_pins(incident_he_pin, *incident_he, _hg) {
          if (*incident_he_pin == *pin) { continue; }
          DBG(dbg_refinement_he_fm_update_level, "->Considering incident_he_pin "
              << *incident_he_pin << " of HE " << *incident_he);
          recomputeGainsForIncidentCutHyperedges(*incident_he_pin);
        } endfor
      } endfor
    } endfor
  }

  void recomputeGainsForIncidentCutHyperedges(HypernodeID hn) {
    forall_incident_hyperedges(he, hn, _hg) {
      DBG(dbg_refinement_he_fm_update_level,
          " Recomputing Gains for HE " << *he << "  incident to HN " << hn);
      if (_update_indicator.isAlreadyEvaluated(*he)) {
        DBG(dbg_refinement_he_fm_update_evaluated,
            "*** Skipping HE " << *he << " because it is already evaluated!");
        continue;
      }
      if (isLocked(*he)) {
        ASSERT(!_pq[0]->contains(*he), "HE " << *he << "should not be present in PQ 0");
        ASSERT(!_pq[1]->contains(*he), "HE " << *he << "should not be present in PQ 1");
        DBG(dbg_refinement_he_fm_update_locked, "HE " << *he << " is locked");
        _update_indicator.markAsEvaluated(*he);
        continue;
      }
      if (wasCutHyperedgeBeforeMove(*he)) {
        ASSERT(_pq[0]->contains(*he) && _pq[1]->contains(*he),
               "HE " << *he << "should be present in both PQs");
        if (isCutHyperedge(*he)) {
          recomputeGainsForCutHyperedge(*he);
        } else {
          removeNonCutHyperedgeFromQueues(*he);
        }
      } else if (isCutHyperedge(*he)) {
        activateNewCutHyperedge(*he);
      }
      _update_indicator.markAsEvaluated(*he);
    } endfor
  }

  void removeNonCutHyperedgeFromQueues(HyperedgeID he) {
    DBG(dbg_refinement_he_fm_update_cases,
        " Removing HE " << he << " because it is no longer a cut hyperedge");
    _pq[0]->remove(he);
    _pq[1]->remove(he);
  }

  bool wasCutHyperedgeBeforeMove(HyperedgeID he) const {
    return _pq[0]->contains(he);
  }

  void recomputeGainsForCutHyperedge(HyperedgeID he) {
    DBG(dbg_refinement_he_fm_update_cases,
        " Recomputing Gain for HE " << he << " which still is a cut hyperedge");
    _pq[0]->updateKey(he, computeGain(he, 0, 1));
    _pq[1]->updateKey(he, computeGain(he, 1, 0));
  }

  void activateNewCutHyperedge(HyperedgeID he) {
    DBG(dbg_refinement_he_fm_update_cases
        , " Activating HE " << he << " because it has become a cut hyperedge");
    activateHyperedge(he);
  }

  void lock(HyperedgeID he) {
    ASSERT(!_locked_hyperedges[he], "HE " << he << " is already locked");
    _locked_hyperedges[he] = 1;
  }

  bool isLocked(HyperedgeID he) const {
    return _locked_hyperedges[he];
  }

  void resetLockedHyperedges() {
    _locked_hyperedges.reset();
  }

  void markAsContained(HypernodeID hn) {
    ASSERT(!_contained_hypernodes[hn], "HN " << hn << " is already marked as contained");
    _contained_hypernodes[hn] = 1;
  }

  bool isContained(HypernodeID hn) {
    return _contained_hypernodes[hn];
  }

  void resetContainedHypernodes() {
    _contained_hypernodes.reset();
  }

  Hypergraph& _hg;
  const Configuration<Hypergraph> _config;
  std::array<HypernodeWeight, K> _partition_size;
  std::array<HyperedgeFMPQ*, K> _pq;
  boost::dynamic_bitset<uint64_t> _locked_hyperedges;
  HyperedgeEvalIndicator _gain_indicator;
  HyperedgeEvalIndicator _update_indicator;
  boost::dynamic_bitset<uint64_t> _contained_hypernodes;
  std::vector<size_t> _movement_indices;
  std::vector<HypernodeID> _performed_moves;
  bool _is_initialized;
  DISALLOW_COPY_AND_ASSIGN(HyperedgeFMRefiner);
};
}   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
