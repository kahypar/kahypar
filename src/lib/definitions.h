#ifndef LIB_DEFINITIONS_H_
#define LIB_DEFINITIONS_H_

#include <chrono>

#include "lib/datastructure/GenericHypergraph.h"
#include <exception>
#include <unordered_map>

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

typedef std::chrono::time_point<std::chrono::high_resolution_clock> HighResClockTimepoint;



// Vitali LP-definitions
typedef unsigned int Label;

template <typename K, typename V>
using MyHashMap = std::unordered_map<K,V>;

struct NodeData
{
  Label label;
  std::vector<int> location_incident_edges_incident_labels;
};

struct EdgeData
{
  //Label label;
  std::vector<std::pair<Label, PartitionID> > incident_labels;
  std::vector<int> location; // location of the incident_labels in the sample

  bool small_edge = false;

  std::pair<Label, PartitionID> *sampled = nullptr;
  uint32_t sample_size= 0;

  std::unordered_map<Label, uint32_t> label_count_map;

  EdgeData() :
    incident_labels(),
    location(),
    label_count_map()
  { }

  ~EdgeData()
  {
    if (!small_edge)
    {
      delete[] sampled;
    }
  }

  EdgeData(const EdgeData &e)
  {
    throw(std::logic_error("EdgeData with set sampled_pointer was copied!"));
  }
};

// end LP-definitions


} // namespace defs
#endif  // LIB_DEFINITIONS_H_
