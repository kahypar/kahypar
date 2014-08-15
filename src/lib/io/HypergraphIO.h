/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_IO_HYPERGRAPHIO_H_
#define SRC_LIB_IO_HYPERGRAPHIO_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "lib/definitions.h"

using defs::PartitionID;
using defs::Hypergraph;
using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeWeight;
using defs::HypernodeWeight;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeWeightVector;
using defs::HypernodeWeightVector;
using defs::HypergraphType;

namespace io {
typedef std::unordered_map<HypernodeID, HypernodeID> Mapping;

inline void readHGRHeader(std::ifstream& file, HyperedgeID& num_hyperedges,
                          HypernodeID& num_hypernodes, HypergraphType& hypergraph_type) {
  std::string line;
  std::getline(file, line);

  // skip any comments
  while (line[0] == '%') {
    std::getline(file, line);
  }

  std::istringstream sstream(line);
  int i = 0;
  sstream >> num_hyperedges >> num_hypernodes >> i;
  hypergraph_type = static_cast<HypergraphType>(i);
}

inline void readHypergraphFile(std::string& filename, HypernodeID& num_hypernodes,
                               HyperedgeID& num_hyperedges,
                               HyperedgeIndexVector& index_vector,
                               HyperedgeVector& edge_vector,
                               HyperedgeWeightVector* hyperedge_weights = nullptr,
                               HypernodeWeightVector* hypernode_weights = nullptr) {
  ASSERT(!filename.empty(), "No filename for hypergraph file specified");
  HypergraphType hypergraph_type = HypergraphType::Unweighted;
  std::ifstream file(filename);
  if (file) {
    readHGRHeader(file, num_hyperedges, num_hypernodes, hypergraph_type);
    ASSERT(hypergraph_type == HypergraphType::Unweighted ||
           hypergraph_type == HypergraphType::EdgeWeights ||
           hypergraph_type == HypergraphType::NodeWeights ||
           hypergraph_type == HypergraphType::EdgeAndNodeWeights,
           "Hypergraph in file has wrong type");

    bool has_hyperedge_weights = hypergraph_type == HypergraphType::EdgeWeights ||
                                 hypergraph_type == HypergraphType::EdgeAndNodeWeights ?
                                 true : false;
    bool has_hypernode_weights = hypergraph_type == HypergraphType::NodeWeights ||
                                 hypergraph_type == HypergraphType::EdgeAndNodeWeights ?
                                 true : false;

    index_vector.reserve(num_hyperedges + /*sentinel*/ 1);
    index_vector.push_back(edge_vector.size());

    std::string line;
    for (HyperedgeID i = 0; i < num_hyperedges; ++i) {
      std::getline(file, line);
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
        std::getline(file, line);
        std::istringstream line_stream(line);
        HypernodeWeight node_weight;
        line_stream >> node_weight;
        hypernode_weights->push_back(node_weight);
      }
    }
    file.close();
  } else {
    std::cerr << "Error: File not found: " << std::endl;
  }
}

inline void writeHypernodeWeights(std::ofstream& out_stream, const Hypergraph& hypergraph) {
  for (auto && hn : hypergraph.nodes()) {
    out_stream << hypergraph.nodeWeight(hn) << std::endl;
  }
}

inline void writeHGRHeader(std::ofstream& out_stream, const Hypergraph& hypergraph) {
  out_stream << hypergraph.numEdges() << " " << hypergraph.numNodes() << " ";
  if (hypergraph.type() != HypergraphType::Unweighted) {
    out_stream << static_cast<int>(hypergraph.type());
  }
  out_stream << std::endl;
}

inline void writeHypergraphFile(const Hypergraph& hypergraph, const std::string& filename) {
  ASSERT(!filename.empty(), "No filename for hypergraph file specified");
  std::ofstream out_stream(filename.c_str());
  writeHGRHeader(out_stream, hypergraph);

  for (auto && he : hypergraph.edges()) {
    if (hypergraph.type() == HypergraphType::EdgeWeights ||
        hypergraph.type() == HypergraphType::EdgeAndNodeWeights) {
      out_stream << hypergraph.edgeWeight(he) << " ";
    }
    for (auto && pin : hypergraph.pins(he)) {
      out_stream << pin + 1 << " ";
    }
    out_stream << std::endl;
  }

  if (hypergraph.type() == HypergraphType::NodeWeights ||
      hypergraph.type() == HypergraphType::EdgeAndNodeWeights) {
    writeHypernodeWeights(out_stream, hypergraph);
  }
  out_stream.close();
}

inline void writeHypergraphForhMetisPartitioning(const Hypergraph& hypergraph,
                                                 const std::string& filename,
                                                 const Mapping& mapping) {
  ASSERT(!filename.empty(), "No filename for hMetis initial partitioning file specified");
  ASSERT(hypergraph.type() == HypergraphType::EdgeAndNodeWeights,
         "Method can only be called for coarsened hypergraphs");
  std::ofstream out_stream(filename.c_str());
  writeHGRHeader(out_stream, hypergraph);

  for (auto && he : hypergraph.edges()) {
    out_stream << hypergraph.edgeWeight(he) << " ";
    for (auto && pin : hypergraph.pins(he)) {
      ASSERT(mapping.find(pin) != mapping.end(), "No mapping found for pin " << pin);
      out_stream << mapping.find(pin)->second + 1 << " ";
    }
    out_stream << std::endl;
  }

  writeHypernodeWeights(out_stream, hypergraph);
  out_stream.close();
}

inline void readPartitionFile(const std::string& filename, std::vector<PartitionID>& partition) {
  ASSERT(!filename.empty(), "No filename for partition file specified");
  ASSERT(partition.empty(), "Partition vector is not empty");
  std::ifstream file(filename);
  if (file) {
    int part;
    while (file >> part) {
      partition.push_back(part);
    }
    file.close();
  } else {
    std::cerr << "Error: File not found: " << std::endl;
  }
}

inline void writePartitionFile(const Hypergraph& hypergraph, const std::string& filename) {
  ASSERT(!filename.empty(), "No filename for partition file specified");
  std::ofstream out_stream(filename.c_str());
  for (auto && hn : hypergraph.nodes()) {
    out_stream << hypergraph.partID(hn) << std::endl;
  }
  out_stream.close();
}
} // namespace io

#endif  // SRC_LIB_IO_HYPERGRAPHIO_H_
