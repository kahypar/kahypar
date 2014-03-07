/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_

#include <boost/dynamic_bitset.hpp>

#include <limits>

#include "lib/datastructure/PriorityQueue.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/refinement/IRefiner.h"

using defs::INVALID_PARTITION;
using datastructure::PriorityQueue;

namespace partition {
static const bool dbg_refinement_he_fm_gain_computation = false;
static const bool dbg_refinement_he_fm_update_level = false;
static const bool dbg_refinement_he_fm_update_evaluated = true;
static const bool dbg_refinement_he_fm_update_locked = true;
static const bool dbg_refinement_he_fm_update_cases = true;

template <class Hypergraph>
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
    HyperedgeEvalIndicator(HyperedgeID size) :
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
    _is_initialized(false) { }

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
      Gain gain = 1;
      forall_pins(pin, he, _hg) {
        if (_hg.partitionIndex(*pin) != from) { continue; }
        DBG(dbg_refinement_he_fm_gain_computation, "evaluating pin " << *pin);
        forall_incident_hyperedges(incident_he, *pin, _hg) {
          if (*incident_he == he || _gain_indicator.isAlreadyEvaluated(*incident_he)) { continue; }
          if (!isCutHyperedge(*incident_he) &&
              !isNestedIntoInPartition(*incident_he, he, from)) {
            gain -= 1;
            DBG(dbg_refinement_he_fm_gain_computation,
                "pin " << *pin << " HE: " << *incident_he << " gain-=1: " << gain);
          } else if (isCutHyperedge(*incident_he) &&
                     isNestedIntoInPartition(*incident_he, he, from)) {
            gain += 1;
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
    } endfor forall_pins(pin, inner_he, _hg) {
      if (_hg.partitionIndex(*pin) == relevant_partition && !isContained(*pin)) {
        return false;
      }
    }
    endfor
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
    DBG(true, "inserting HE " << he << " with gain " << _pq[0]->key(he) << " in PQ " << 0);
    DBG(true, "inserting HE " << he << " with gain " << _pq[1]->key(he) << " in PQ " << 1);
  }

  void moveHyperedge(HyperedgeID he, PartitionID from, PartitionID to) {
    forall_pins(pin, he, _hg) {
      if (_hg.partitionIndex(*pin) == from) {
        _hg.changeNodePartition(*pin, from, to);
        _partition_size[from] -= _hg.nodeWeight(*pin);
        _partition_size[to] += _hg.nodeWeight(*pin);
      }
    } endfor
      ASSERT(_pq[to]->contains(he) == true,
             "HE " << he << " does not exist in PQ " << to);
    _pq[to]->remove(he);
    lock(he);
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
  bool _is_initialized;
  DISALLOW_COPY_AND_ASSIGN(HyperedgeFMRefiner);
};
}   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
