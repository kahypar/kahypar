/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DEFINITIONS_H_
#define SRC_LIB_DEFINITIONS_H_

#include <chrono>
#include <cstdint>
#include <utility>

#include "lib/datastructure/GenericHypergraph.h"

// Use bucket PQ for FM refinement.
// #define USE_BUCKET_PQ

// Gather advanced statistics
// #define GATHER_STATS

namespace defs {
using hypernode_id_t = std::uint32_t;
using hyperedge_id_t = std::uint32_t;
using hypernode_weight_t = std::uint32_t;
using hyperedge_weight_t = std::int32_t;
using partition_id_t = std::int32_t;

using Hypergraph = datastructure::GenericHypergraph<hypernode_id_t,
                                                    hyperedge_id_t, hypernode_weight_t,
                                                    hyperedge_weight_t, partition_id_t>;

using RatingType = double;
using HypernodeID = Hypergraph::HypernodeID;
using HyperedgeID = Hypergraph::HyperedgeID;
using PartitionID = Hypergraph::PartitionID;
using HypernodeWeight = Hypergraph::HypernodeWeight;
using HyperedgeWeight = Hypergraph::HyperedgeWeight;
using HypergraphType = Hypergraph::Type;
using HyperedgeIndexVector = Hypergraph::HyperedgeIndexVector;
using HyperedgeVector = Hypergraph::HyperedgeVector;
using HyperedgeWeightVector = Hypergraph::HyperedgeWeightVector;
using HypernodeWeightVector = Hypergraph::HypernodeWeightVector;
using IncidenceIterator = Hypergraph::IncidenceIterator;

using HighResClockTimepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
}  // namespace defs

// this is nasty and needs to be fixed
namespace std {
static defs::IncidenceIterator begin(std::pair<defs::IncidenceIterator, defs::IncidenceIterator>& x) {
  return std::move(x.first);
}

static defs::IncidenceIterator end(std::pair<defs::IncidenceIterator, defs::IncidenceIterator>& x) {
  return std::move(x.second);
}
}  // namespace std
#endif  // SRC_LIB_DEFINITIONS_H_
