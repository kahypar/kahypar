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

  public:
  HyperedgeFMRefiner(Hypergraph& hypergraph, const Configuration<Hypergraph>& config) :
    _hg(hypergraph),
    _config(config),
    _partition_size{0, 0},
    _pq{new HyperedgeFMPQ(_hg.initialNumNodes()), new HyperedgeFMPQ(_hg.initialNumNodes())},
    _locked_hyperedges(_hg.initialNumEdges()),
    _evaluated_hyperedges(_hg.initialNumEdges()),
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
      if (isCutHyperedge(*he)) {
        ASSERT(!isLocked(*he), "HE " << *he << " is already locked");
        ASSERT(!_pq[0]->contains(*he), "HE " << *he << " is already contained in PQ 0");
        ASSERT(!_pq[1]->contains(*he), "HE " << *he << " is already contained in PQ 0");
        _pq[0]->reInsert(*he, computeGain(*he, 0, 1));
        _pq[1]->reInsert(*he, computeGain(*he, 1, 0));
        DBG(true, "inserting HE " << *he << " with gain " << _pq[0]->key(*he)
            << " in PQ " << 0);
        DBG(true, "inserting HE " << *he << " with gain " << _pq[1]->key(*he)
            << " in PQ " << 1);
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
      resetEvaluatedHyperedges();
      Gain gain = 1;
      forall_pins(pin, he, _hg) {
        if (_hg.partitionIndex(*pin) != from) { continue; }
        DBG(true, "evaluating pin " << *pin);
        forall_incident_hyperedges(incident_he, *pin, _hg) {
          if (*incident_he == he || isAlreadyEvaluated(*incident_he)) { continue; }
          if (!isCutHyperedge(*incident_he) &&
              !isNestedIntoInPartition(*incident_he, he, from)) {
            gain -= 1;
            DBG(true, "pin " << *pin << " HE: " << *incident_he << " gain-=1: " << gain);
          } else if (isCutHyperedge(*incident_he) &&
                     isNestedIntoInPartition(*incident_he, he, from)) {
            gain += 1;
            DBG(true, "pin " << *pin << " HE: " << *incident_he << " g+=1: " << gain);
          }
          markAsEvaluated(*incident_he);
        } endfor
      } endfor
      return gain;
    } else {
      return 0;
    }
  }

  bool isNestedIntoInPartition(HyperedgeID inner_he, HyperedgeID outer_he,
                               PartitionID relevant_partition) {
    // ToDo: [ ] reuse bitset across queries
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

  bool isCutHyperedge(HyperedgeID he) const {
    return _hg.pinCountInPartition(he, 0) * _hg.pinCountInPartition(he, 1) > 0;
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

  void markAsEvaluated(HyperedgeID he) {
    ASSERT(!_evaluated_hyperedges[he], "HE " << he << " is already marked as evaluated");
    _evaluated_hyperedges[he] = 1;
  }

  bool isAlreadyEvaluated(HyperedgeID he) const {
    return _evaluated_hyperedges[he];
  }

  void resetEvaluatedHyperedges() {
    _evaluated_hyperedges.reset();
  }

  void markAsContained(HypernodeID hn) {
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
  boost::dynamic_bitset<uint64_t> _evaluated_hyperedges;
  boost::dynamic_bitset<uint64_t> _contained_hypernodes;
  bool _is_initialized;
  DISALLOW_COPY_AND_ASSIGN(HyperedgeFMRefiner);
};
}   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
