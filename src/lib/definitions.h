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

  bool small_edge;

  std::pair<Label, PartitionID> *sampled;
  uint32_t sample_size;

  std::unordered_map<Label, uint32_t> label_count_map;

  EdgeData() :
    incident_labels(),
    location(),
    small_edge(false),
    sampled(nullptr),
    sample_size(0),
    label_count_map()
  {
  }

  ~EdgeData()
  {
    if (!small_edge && sampled != nullptr)
    {
      delete[] sampled;
    }
  }

  EdgeData& operator=(const EdgeData &edge_data)
  {
    if (!small_edge && sampled != nullptr)
    {
      delete[] sampled;
    }

    incident_labels = edge_data.incident_labels;
    location = edge_data.location;
    small_edge = edge_data.small_edge;
    sample_size = edge_data.sample_size;
    label_count_map = edge_data.label_count_map;

    if (small_edge)
    {
      sampled = incident_labels.data();
    } else {
      sampled = new std::pair<Label, PartitionID>[sample_size];
    }
  }

  EdgeData& operator=(EdgeData &&edge_data)
  {
    if (!small_edge && sampled != nullptr)
    {
      delete[] sampled;
    }

    incident_labels = std::move(edge_data.incident_labels);
    location = std::move(edge_data.location);
    small_edge = edge_data.small_edge;
    sample_size = edge_data.sample_size;
    label_count_map = std::move(edge_data.label_count_map);

    // "steal" the sampled array
    sampled = edge_data.sampled;
    edge_data.sampled = nullptr;
  }

  EdgeData(const EdgeData &e)
  {
    throw std::logic_error("EdgeData was copy-constructed... not supported yet!");
  }

  EdgeData(const EdgeData &&e)
  {
    throw std::logic_error("EdgeData was move-constructed... not supported yet!");
  }
};

// end LP-definitions


} // namespace defs
#endif  // LIB_DEFINITIONS_H_
