/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
#define SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_

#include <boost/dynamic_bitset.hpp>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/refinement/IRefiner.h"

using defs::INVALID_PARTITION;

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

  public:
  HyperedgeFMRefiner(Hypergraph& hypergraph, const Configuration<Hypergraph>& config) :
    _hg(hypergraph),
    _config(config) { }

  void refine(HypernodeID u, HypernodeID v, HyperedgeWeight& best_cut,
              double max_imbalance, double& best_imbalance) { }

  Gain computeGain(HyperedgeID he, PartitionID from, PartitionID to) const {
    ASSERT((from < 2) && (to < 2), "Trying to compute gain for PartitionIndex >= 2");
    ASSERT((from != INVALID_PARTITION) && (to != INVALID_PARTITION),
           "Trying to compute gain for invalid partition");
    // ToDo: [ ] reuse bitset across queries
    boost::dynamic_bitset<uint64_t> evaluated_hyperedges(_hg.initialNumEdges());
    if (isCutHyperedge(he)) {
      Gain gain = 1;
      forall_pins(pin, he, _hg) {
        if (_hg.partitionIndex(*pin) != from) { continue; }
        DBG(true, "evaluating pin " << *pin);
        forall_incident_hyperedges(incident_he, *pin, _hg) {
          if (*incident_he == he || evaluated_hyperedges[*incident_he]) { continue; }
          if (!isCutHyperedge(*incident_he) &&
              !isPartiallyNestedIntoHyperedge(*incident_he, he, from)) {
            gain -= 1;
            DBG(true, "pin " << *pin << " HE: " << *incident_he << " gain-=1: " << gain);
          } else {
            if (isCutHyperedge(*incident_he) &&
                isPartiallyNestedIntoHyperedge(*incident_he, he, from)) {
              gain += 1;
              DBG(true, "pin " << *pin << " HE: " << *incident_he << " g+=1: " << gain);
            }
          }
          evaluated_hyperedges[*incident_he] = 1;
        } endfor
      } endfor
      return gain;
    } else {
      return 0;
    }
  }

  bool isPartiallyNestedIntoHyperedge(HyperedgeID inner_he, HyperedgeID outer_he,
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
  bool isCutHyperedge(HyperedgeID he) const {
    return _hg.pinCountInPartition(he, 0) * _hg.pinCountInPartition(he, 1) > 0;
  }

  Hypergraph& _hg;
  Configuration<Hypergraph> _config;
  DISALLOW_COPY_AND_ASSIGN(HyperedgeFMRefiner);
};
}   // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_HYPEREDGEFMREFINER_H_
