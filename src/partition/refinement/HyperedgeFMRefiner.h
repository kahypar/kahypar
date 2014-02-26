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
      _is_initialized = true;
  }

  void activateIncidentCutHyperedges(HypernodeID hn) {
    DBG(true, "activating cut hyperedges of hypernode " << hn);
    forall_incident_hyperedges(he, hn, _hg) {
      if (isCutHyperedge(*he)) {
        // ToDo:
        // [ ] check if hyperedge is locked
        // [ ] check if pin is locked?
        DBG(true, "inserting HE " << *he << " with gain " << computeGain(*he, 0, 1)
            << " in PQ " << 0);
        DBG(true, "inserting HE " << *he << " with gain " << computeGain(*he, 1, 0)
            << " in PQ " << 1);
        _pq[0]->reInsert(*he, computeGain(*he, 0, 1));
        _pq[1]->reInsert(*he, computeGain(*he, 1, 0));
      }
    } endfor
  }

  void refine(HypernodeID u, HypernodeID v, HyperedgeWeight& best_cut,
              double max_imbalance, double& best_imbalance) { }

  Gain computeGain(HyperedgeID he, PartitionID from, PartitionID to) const {
    ASSERT((from < 2) && (to < 2), "Trying to compute gain for PartitionIndex >= 2");
    ASSERT((from != INVALID_PARTITION) && (to != INVALID_PARTITION),
           "Trying to compute gain for invalid partition");
    if (isCutHyperedge(he)) {
      // ToDo: [ ] reuse bitset across queries
      boost::dynamic_bitset<uint64_t> evaluated_hyperedges(_hg.initialNumEdges());
      Gain gain = 1;
      forall_pins(pin, he, _hg) {
        if (_hg.partitionIndex(*pin) != from) { continue; }
        DBG(true, "evaluating pin " << *pin);
        forall_incident_hyperedges(incident_he, *pin, _hg) {
          if (*incident_he == he || evaluated_hyperedges[*incident_he]) { continue; }
          if (!isCutHyperedge(*incident_he) &&
              !isNestedIntoInPartition(*incident_he, he, from)) {
            gain -= 1;
            DBG(true, "pin " << *pin << " HE: " << *incident_he << " gain-=1: " << gain);
          } else if (isCutHyperedge(*incident_he) &&
                     isNestedIntoInPartition(*incident_he, he, from)) {
            gain += 1;
            DBG(true, "pin " << *pin << " HE: " << *incident_he << " g+=1: " << gain);
          }
          evaluated_hyperedges[*incident_he] = 1;
        } endfor
      } endfor
      return gain;
    } else {
      return 0;
    }
  }

  bool isNestedIntoInPartition(HyperedgeID inner_he, HyperedgeID outer_he,
                               PartitionID relevant_partition) const {
    // ToDo: [ ] reuse bitset across queries
    boost::dynamic_bitset<uint64_t> outer_nodes(_hg.initialNumNodes());
    forall_pins(pin, outer_he, _hg) {
      if (_hg.partitionIndex(*pin) == relevant_partition) {
        outer_nodes[*pin] = 1;
      }
    } endfor forall_pins(pin, inner_he, _hg) {
      if (_hg.partitionIndex(*pin) == relevant_partition && !outer_nodes[*pin]) {
        return false;
      }
    }
    endfor
    return true;
  }

  private:
  FRIEND_TEST(AHyperedgeFMRefiner, MaintainsSizeOfPartitionsWhichAreInitializedByCallingInitialize);
  FRIEND_TEST(AHyperedgeFMRefiner, ActivatesOnlyCutHyperedgesByInsertingThemIntoPQ);

  bool isCutHyperedge(HyperedgeID he) const {
    return _hg.pinCountInPartition(he, 0) * _hg.pinCountInPartition(he, 1) > 0;
  }

  Hypergraph& _hg;
  const Configuration<Hypergraph> _config;
  std::array<HypernodeWeight, K> _partition_size;
  std::array<HyperedgeFMPQ*, K> _pq;
  bool _is_initialized;
  DISALLOW_COPY_AND_ASSIGN(HyperedgeFMRefiner);
};
}   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
