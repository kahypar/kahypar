#ifndef LIB_DEFINITIONS_H_
#define LIB_DEFINITIONS_H_

#include <chrono>

#include "lib/datastructure/GenericHypergraph.h"

// Use bucket PQ for FM refinement.
//#define USE_BUCKET_PQ

// Gather advanced statistics
#define GATHER_STATS

namespace defs {
typedef unsigned int hypernode_id_t;
typedef unsigned int hyperedge_id_t;
typedef unsigned int hypernode_weight_t;
typedef int hyperedge_weight_t;
typedef int partition_id_t;

typedef datastructure::GenericHypergraph<hypernode_id_t, hyperedge_id_t, hypernode_weight_t,
                                         hyperedge_weight_t, partition_id_t> Hypergraph;

typedef double RatingType;
typedef Hypergraph::HypernodeID HypernodeID;
typedef Hypergraph::HyperedgeID HyperedgeID;
typedef Hypergraph::PartitionID PartitionID;
typedef Hypergraph::HypernodeWeight HypernodeWeight;
typedef Hypergraph::HyperedgeWeight HyperedgeWeight;
typedef Hypergraph::Type HypergraphType;
typedef Hypergraph::HypernodeIterator HypernodeIterator;
typedef Hypergraph::HyperedgeIterator HyperedgeIterator;
typedef Hypergraph::IncidenceIterator IncidenceIterator;
typedef Hypergraph::HyperedgeIndexVector HyperedgeIndexVector;
typedef Hypergraph::HyperedgeVector HyperedgeVector;
typedef Hypergraph::HyperedgeWeightVector HyperedgeWeightVector;
typedef Hypergraph::HypernodeWeightVector HypernodeWeightVector;
typedef Hypergraph::HypernodeIteratorPair HypernodeIteratorPair;
typedef Hypergraph::HyperedgeIteratorPair HyperedgeIteratorPair;
typedef Hypergraph::IncidenceIteratorPair IncidenceIteratorPair;
typedef Hypergraph::ConnectivitySetIteratorPair ConnectivitySetIteratorPair;
typedef Hypergraph::PartInfoIteratorPair  PartInfoIteratorPair;

typedef std::chrono::time_point<std::chrono::high_resolution_clock> HighResClockTimepoint;

} // namespace defs
#endif  // LIB_DEFINITIONS_H_
