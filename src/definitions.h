/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <chrono>
#include <cstdint>
#include <utility>

#include "datastructure/GenericHypergraph.h"
// #include "datastructure/GenericHypergraph2.h"

// Use bucket PQ for FM refinement.
// #define USE_BUCKET_PQ

// Gather advanced statistics
// #define GATHER_STATS

namespace defs {
using HypernodeID = std::uint32_t;
using HyperedgeID = std::uint32_t;
using HypernodeWeight = std::uint32_t;
using HyperedgeWeight = std::int32_t;
using PartitionID = std::int32_t;
using Gain = HyperedgeWeight;

using Hypergraph = datastructure::GenericHypergraph<HypernodeID,
                                                    HyperedgeID, HypernodeWeight,
                                                    HyperedgeWeight, PartitionID>;

using RatingType = double;
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
  return x.first;
}

static defs::IncidenceIterator end(std::pair<defs::IncidenceIterator, defs::IncidenceIterator>& x) {
  return x.second;
}

// static const defs::HypernodeID* begin(std::pair<const defs::HypernodeID*, const defs::HypernodeID*>& x) {
//   return x.first;
// }

// static const defs::HypernodeID* end(std::pair<const defs::HypernodeID*, const defs::HypernodeID*>& x) {
//   return x.second;
// }
}  // namespace std
