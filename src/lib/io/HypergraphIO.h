#ifndef LIB_IO_HYPERGRAPHIO_H_
#define LIB_IO_HYPERGRAPHIO_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "lib/datastructure/Hypergraph.h"
#include "lib/definitions.h"

namespace io {
using defs::PartitionID;

using datastructure::HypergraphType;
using datastructure::HypernodeID;
using datastructure::HyperedgeID;
using datastructure::HypergraphWeightType;
using datastructure::HyperedgeWeight;
using datastructure::HypernodeWeight;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;
using datastructure::HyperedgeWeightVector;
using datastructure::HypernodeWeightVector;

typedef std::unordered_map<HypernodeID, HypernodeID> Mapping;

inline void readHGRHeader(std::ifstream& file, HyperedgeID& num_hyperedges,
                          HypernodeID& num_hypernodes, HypergraphWeightType& hypergraph_type) {
  std::string line;
  std::getline(file, line);

  // skip any comments
  while (line[0] == '%') {
    std::getline(file, line);
  }

  std::istringstream sstream(line);
  int i = 0;
  sstream >> num_hyperedges >> num_hypernodes >> i;
  hypergraph_type = static_cast<HypergraphWeightType>(i);
}

inline void readHypergraphFile(std::string& filename, HypernodeID& num_hypernodes,
                               HyperedgeID& num_hyperedges,
                               HyperedgeIndexVector& index_vector,
                               HyperedgeVector& edge_vector,
                               HyperedgeWeightVector* hyperedge_weights = nullptr,
                               HypernodeWeightVector* hypernode_weights = nullptr) {
  ASSERT(!filename.empty(), "No filename for hypergraph file specified");
  HypergraphWeightType hypergraph_type = HypergraphWeightType::Unweighted;
  std::ifstream file(filename);
  if (file) {
    readHGRHeader(file, num_hyperedges, num_hypernodes, hypergraph_type);
    ASSERT(hypergraph_type == HypergraphWeightType::Unweighted ||
           hypergraph_type == HypergraphWeightType::EdgeWeights ||
           hypergraph_type == HypergraphWeightType::NodeWeights ||
           hypergraph_type == HypergraphWeightType::EdgeAndNodeWeights,
           "Hypergraph in file has wrong type");

    bool has_hyperedge_weights = hypergraph_type == HypergraphWeightType::EdgeWeights ||
                                 hypergraph_type == HypergraphWeightType::EdgeAndNodeWeights ?
                                 true : false;
    bool has_hypernode_weights = hypergraph_type == HypergraphWeightType::NodeWeights ||
                                 hypergraph_type == HypergraphWeightType::EdgeAndNodeWeights ?
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

template <class Hypergraph>
inline void writeHypernodeWeights(std::ofstream& out_stream, const Hypergraph& hypergraph) {
  forall_hypernodes(hn, hypergraph) {
    out_stream << hypergraph.nodeWeight(*hn) << std::endl;
  } endfor
}

template <class Hypergraph>
inline void writeHGRHeader(std::ofstream& out_stream, const Hypergraph& hypergraph) {
  out_stream << hypergraph.numEdges() << " " << hypergraph.numNodes() << " ";
  if (hypergraph.type() != HypergraphWeightType::Unweighted) {
    out_stream << static_cast<int>(hypergraph.type());
  }
  out_stream << std::endl;
}

template <class Hypergraph>
inline void writeHypergraphFile(const Hypergraph& hypergraph, const std::string& filename) {
  ASSERT(!filename.empty(), "No filename for hypergraph file specified");
  std::ofstream out_stream(filename.c_str());
  writeHGRHeader(out_stream, hypergraph);

  forall_hyperedges(he, hypergraph) {
    if (hypergraph.type() == HypergraphWeightType::EdgeWeights ||
        hypergraph.type() == HypergraphWeightType::EdgeAndNodeWeights) {
      out_stream << hypergraph.edgeWeight(*he) << " ";
    }
    forall_pins(pin, *he, hypergraph) {
      out_stream << *pin + 1 << " ";
    } endfor
      out_stream << std::endl;
  } endfor

  if (hypergraph.type() == HypergraphWeightType::NodeWeights ||
      hypergraph.type() == HypergraphWeightType::EdgeAndNodeWeights) {
    writeHypernodeWeights(out_stream, hypergraph);
  }
  out_stream.close();
}

template <typename Hypergraph>
inline void writeHypergraphForhMetisPartitioning(const Hypergraph& hypergraph,
                                                 const std::string& filename,
                                                 const Mapping& mapping) {
  ASSERT(!filename.empty(), "No filename for hMetis initial partitioning file specified");
  ASSERT(hypergraph.type() == HypergraphWeightType::EdgeAndNodeWeights,
         "Method can only be called for coarsened hypergraphs");
  std::ofstream out_stream(filename.c_str());
  writeHGRHeader(out_stream, hypergraph);

  forall_hyperedges(he, hypergraph) {
    out_stream << hypergraph.edgeWeight(*he) << " ";
    forall_pins(pin, *he, hypergraph) {
      ASSERT(mapping.find(*pin) != mapping.end(), "No mapping found for pin " << *pin);
      out_stream << mapping.find(*pin)->second + 1 << " ";
    } endfor
      out_stream << std::endl;
  } endfor

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

template <class Hypergraph>
inline void writePartitionFile(const Hypergraph& hypergraph, const std::string& filename) {
  ASSERT(!filename.empty(), "No filename for partition file specified");
  std::ofstream out_stream(filename.c_str());
  forall_hypernodes(hn, hypergraph) {
    out_stream << hypergraph.partitionIndex(*hn) << std::endl;
  } endfor
  out_stream.close();
}
} // namespace io

#endif  // LIB_IO_HYPERGRAPHIO_H_
