#ifndef LIB_IO_HYPERGRAPHIO_H_
#define LIB_IO_HYPERGRAPHIO_H_

#include <fstream>
#include <iostream>
#include <sstream>

#include "../datastructure/Hypergraph.h"

namespace io {

using defs::hMetisHyperEdgeIndexVector;
using defs::hMetisHyperEdgeVector;
using defs::hMetisHyperEdgeWeightVector;
using defs::hMetisHyperNodeWeightVector;

typedef hgr::HypergraphType HypergraphType;
typedef HypergraphType::HypernodeID HypernodeID;
typedef HypergraphType::HyperedgeID HyperedgeID;
typedef HypergraphType::HyperedgeWeight HyperedgeWeight;
typedef HypergraphType::HypernodeWeight HypernodeWeight;

enum HypergraphTypes {
  kUnweighted = 0,
  kEdgeWeights = 1,
  kNodeWeights = 10,
  kEdgeAndNodeWeights = 11,
};


void parseHGRHeader(std::ifstream& file, HyperedgeID& num_hyperedges, HypernodeID& num_hypernodes,
                    int& hypergraph_type) {
  if(file) {
    std::string line;
    std::getline(file, line);

    // skip any comments
    while (line[0] == '%') {
      std::getline(file, line);
    }

    std::istringstream sstream(line);
    sstream >> num_hyperedges >> num_hypernodes >> hypergraph_type;
  } else {
    std::cerr << "Error: File not found: " << std::endl;
  }
}

void parseHypergraphFile(std::ifstream& file, HypernodeID &num_hypernodes,
                         HyperedgeID &num_hyperedges,
                         hMetisHyperEdgeIndexVector& index_vector,
                         hMetisHyperEdgeVector& edge_vector,
                         hMetisHyperEdgeWeightVector* hyperedge_weights,
                         hMetisHyperNodeWeightVector* hypernode_weights) {
  int hypergraph_type = kUnweighted;
  parseHGRHeader(file, num_hyperedges, num_hypernodes, hypergraph_type);
  ASSERT(hypergraph_type == kUnweighted || hypergraph_type == kEdgeWeights ||
         hypergraph_type == kNodeWeights || hypergraph_type == kEdgeAndNodeWeights,
         "Hypergraph in file has wrong type");

  bool has_hyperedge_weights = hypergraph_type == kEdgeWeights
                               || hypergraph_type == kEdgeAndNodeWeights ? true : false;
  bool has_hypernode_weights = hypergraph_type == kNodeWeights
                               || hypergraph_type == kEdgeAndNodeWeights ? true : false;

  index_vector.reserve(num_hyperedges + /*sentinel*/ 1);
  index_vector.push_back(edge_vector.size());
  
  std::string line;
  for (HyperedgeID i = 0; i < num_hyperedges; ++i) {
    std::getline(file,line);
    std::istringstream line_stream(line);
    if (has_hyperedge_weights) {
      ASSERT(hyperedge_weights != nullptr, "Hypergraph has hyperedge weights");
      hyperedge_weights->reserve(num_hyperedges);
      HyperedgeWeight edge_weight;
      line_stream >> edge_weight;
      hyperedge_weights->push_back(edge_weight);
    }
    HypernodeID pin;
    while (line_stream >> pin) {
      // Hypernode IDs start from 0
      --pin;
      ASSERT(pin < num_hypernodes, "Invalid hypernode ID");
      edge_vector.push_back(pin);
    }
    index_vector.push_back(edge_vector.size()); 
  }

  if (has_hypernode_weights) {
    ASSERT(hypernode_weights != nullptr, "Hypergraph has hypernode weights");
    hypernode_weights->reserve(num_hypernodes);
    for (HypernodeID i = 0; i < num_hypernodes; ++i) {
      std::getline(file,line);
      std::istringstream line_stream(line);
      HypernodeWeight node_weight;
      line_stream >> node_weight;
      hypernode_weights->push_back(node_weight);
    }
  }
  
}

} // namespace io

#endif  // LIB_IO_HYPERGRAPHIO_H_
