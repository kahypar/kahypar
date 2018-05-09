/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <chrono>
#include <cstdint>
#include <utility>

#include "datastructure/hypergraph.h"

// Use bucket PQ for FM refinement.
// #define USE_BUCKET_QUEUE

// Gather advanced statistics
// #define GATHER_STATS

namespace kahypar {
using HypernodeID = uint32_t;
using HyperedgeID = uint32_t;
using HypernodeWeight = int32_t;
using HyperedgeWeight = int32_t;
using PartitionID = int32_t;
using Gain = HyperedgeWeight;

using Hypergraph = kahypar::ds::GenericHypergraph<HypernodeID,
                                                  HyperedgeID, HypernodeWeight,
                                                  HyperedgeWeight, PartitionID>;

using RatingType = double;
using HypergraphType = Hypergraph::Type;
using HyperedgeIndexVector = Hypergraph::HyperedgeIndexVector;
using HyperedgeVector = Hypergraph::HyperedgeVector;
using HyperedgeWeightVector = Hypergraph::HyperedgeWeightVector;
using HypernodeWeightVector = Hypergraph::HypernodeWeightVector;
using IncidenceIterator = Hypergraph::IncidenceIterator;

// #########Graph-Definitions#############
using NodeID = HypernodeID;
using EdgeID = HyperedgeID;
using EdgeWeight = long double;
using ClusterID = PartitionID;
using Flow = int32_t;
using Capacity = int32_t;

using HighResClockTimepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
}  // namespace kahypar

// this is nasty and needs to be fixed
namespace std {
static kahypar::IncidenceIterator begin(const std::pair<kahypar::IncidenceIterator,
                                                        kahypar::IncidenceIterator>& x) {
  return x.first;
}

static kahypar::IncidenceIterator end(const std::pair<kahypar::IncidenceIterator,
                                                      kahypar::IncidenceIterator>& x) {
  return x.second;
}

template <typename Iterator>
Iterator begin(std::pair<Iterator, Iterator>& x) {
  return x.first;
}

template <typename Iterator>
Iterator end(std::pair<Iterator, Iterator>& x) {
  return x.second;
}
}  // namespace std
