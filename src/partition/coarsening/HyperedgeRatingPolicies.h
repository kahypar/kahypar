/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HYPEREDGERATINGPOLICIES_H_
#define SRC_PARTITION_COARSENING_HYPEREDGERATINGPOLICIES_H_

#include <limits>

#include "lib/definitions.h"

using defs::HyperedgeID;
using defs::HypernodeWeight;
using defs::Hypergraph;
using defs::IncidenceIterator;
using defs::RatingType;
using defs::PartitionID;

namespace partition {
struct HyperedgeRating {
  HypernodeWeight total_node_weight;
  RatingType value;
  bool valid;
  HyperedgeRating(RatingType val, bool is_valid) :
    total_node_weight(std::numeric_limits<HypernodeWeight>::max()),
    value(val),
    valid(is_valid) { }
  HyperedgeRating() :
    total_node_weight(std::numeric_limits<HypernodeWeight>::max()),
    value(std::numeric_limits<RatingType>::min()),
    valid(false) { }
};

struct EdgeWeightDivMultPinWeight {
  static HyperedgeRating rate(HyperedgeID he, const Hypergraph& hypergraph,
                              HypernodeWeight threshold_node_weight) {
    IncidenceIterator pins_begin = hypergraph.pins(he).begin();
    IncidenceIterator pins_end = hypergraph.pins(he).end();
    ASSERT(pins_begin != pins_end, "Hyperedge does not contain any pins");

    PartitionID partition = hypergraph.partID(*pins_begin);
    double pin_weights_mult = static_cast<double>(hypergraph.nodeWeight(*pins_begin));
    HypernodeWeight sum_pin_weights = hypergraph.nodeWeight(*pins_begin);
    bool is_cut_hyperedge = false;
    ++pins_begin;

    for (IncidenceIterator pin = pins_begin; pin != pins_end; ++pin) {
      if (partition != hypergraph.partID(*pin)) {
        is_cut_hyperedge = true;
        break;
      }
      pin_weights_mult *= static_cast<double>(hypergraph.nodeWeight(*pin));
      sum_pin_weights += hypergraph.nodeWeight(*pin);
    }

    HyperedgeRating rating;
    rating.total_node_weight = sum_pin_weights;
    if (sum_pin_weights > threshold_node_weight || is_cut_hyperedge) {
      return rating;
    }

    rating.valid = true;
    //LOG("r(" << he << ")=" << hypergraph.edgeWeight(he) << "/" << pin_weights_mult << "=" << hypergraph.edgeWeight(he) / pin_weights_mult);
    rating.value = hypergraph.edgeWeight(he) / pin_weights_mult;
    return rating;
  }
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HYPEREDGERATINGPOLICIES_H_
