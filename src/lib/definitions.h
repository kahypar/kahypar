#ifndef LIB_DEFINITIONS_H_
#define LIB_DEFINITIONS_H_

#include <chrono>

#include "lib/datastructure/GenericHypergraph.h"

// Use bucket PQ for FM refinement.
//#define USE_BUCKET_PQ

// Gather advanced statistics
#define GATHER_STATS

namespace defs {
using hypernode_id_t =unsigned int;
using hyperedge_id_t =unsigned int;
using hypernode_weight_t =unsigned int;
using hyperedge_weight_t = int;
using partition_id_t = int;

using Hypergraph =  datastructure::GenericHypergraph<hypernode_id_t,
                                                     hyperedge_id_t, hypernode_weight_t,
                                                     hyperedge_weight_t, partition_id_t>;

using RatingType = double;
using HypernodeID = Hypergraph::HypernodeID;
using HyperedgeID = Hypergraph::HyperedgeID;
using PartitionID = Hypergraph::PartitionID;
using HypernodeWeight = Hypergraph::HypernodeWeight;
using HyperedgeWeight = Hypergraph::HyperedgeWeight;
using HypergraphType = Hypergraph::Type ;
using HyperedgeIndexVector = Hypergraph::HyperedgeIndexVector;
using HyperedgeVector = Hypergraph::HyperedgeVector;
using HyperedgeWeightVector = Hypergraph::HyperedgeWeightVector;
using HypernodeWeightVector = Hypergraph::HypernodeWeightVector;
using IncidenceIterator = Hypergraph::IncidenceIterator;

using HighResClockTimepoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
} // namespace defs

// this is nasty and needs to be fixed
namespace std {
static defs::IncidenceIterator  begin(std::pair<defs::IncidenceIterator,  defs::IncidenceIterator>& x) {
  return x.first;
}

static defs::IncidenceIterator  end(std::pair<defs::IncidenceIterator,  defs::IncidenceIterator>& x) {
  return x.second;
}
}
#endif  // LIB_DEFINITIONS_H_
