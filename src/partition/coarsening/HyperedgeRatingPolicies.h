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

struct EdgeWeightDivGeoMeanPinWeight {
  static HyperedgeRating rate(HyperedgeID he, const Hypergraph& hypergraph,
                              HypernodeWeight threshold_node_weight) {
    IncidenceIterator pins_begin, pins_end;
    std::tie(pins_begin, pins_end) = hypergraph.pins(he);
    ASSERT(pins_begin != pins_end, "Hyperedge does not contain any pins");

    PartitionID partition = hypergraph.partitionIndex(*pins_begin);
    double geo_mean_node_weight = static_cast<double>(hypergraph.nodeWeight(*pins_begin));
    HypernodeWeight sum_node_weights = hypergraph.nodeWeight(*pins_begin);
    bool is_cut_hyperedge = false;
    ++pins_begin;

    for (IncidenceIterator pin = pins_begin; pin != pins_end; ++pin) {
      if (partition != hypergraph.partitionIndex(*pin)) {
        is_cut_hyperedge = true;
        break;
      }
      geo_mean_node_weight *= static_cast<double>(hypergraph.nodeWeight(*pin));
      sum_node_weights += hypergraph.nodeWeight(*pin);
    }

    HyperedgeRating rating;
    rating.total_node_weight = sum_node_weights;
    if (sum_node_weights > threshold_node_weight || is_cut_hyperedge) {
      return rating;
    }

    geo_mean_node_weight = std::pow(geo_mean_node_weight, 1.0 / hypergraph.edgeSize(he));

    rating.valid = true;
    rating.value = hypergraph.edgeWeight(he) / geo_mean_node_weight;
    return rating;
  }
};
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HYPEREDGERATINGPOLICIES_H_
