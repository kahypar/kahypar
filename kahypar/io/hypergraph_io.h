/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "kahypar/definitions.h"

namespace kahypar {
namespace io {
using Mapping = std::unordered_map<HypernodeID, HypernodeID>;

static inline void readHGRHeader(std::ifstream& file, HyperedgeID& num_hyperedges,
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

static inline void readHypergraphFile(const std::string& filename, HypernodeID& num_hypernodes,
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

    index_vector.reserve(static_cast<size_t>(num_hyperedges) +  /*sentinel*/ 1);
    index_vector.push_back(edge_vector.size());

    std::string line;
    for (HyperedgeID i = 0; i < num_hyperedges; ++i) {
      std::getline(file, line);
      std::istringstream line_stream(line);
      if (line_stream.peek() == EOF) {
        std::cerr << "Error: Hyperedge " << i << " is empty" << std::endl;
        exit(1);
      }

      if (has_hyperedge_weights) {
        HyperedgeWeight edge_weight;
        line_stream >> edge_weight;
        if (hyperedge_weights == nullptr) {
          LOG << "****** ignoring hyperedge weights ******";
        } else {
          ASSERT(hyperedge_weights != nullptr, "Hypergraph has hyperedge weights");
          hyperedge_weights->push_back(edge_weight);
        }
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
      if (hypernode_weights == nullptr) {
        LOG << " ****** ignoring hypernode weights ******";
      } else {
        ASSERT(hypernode_weights != nullptr, "Hypergraph has hypernode weights");
        for (HypernodeID i = 0; i < num_hypernodes; ++i) {
          std::getline(file, line);
          std::istringstream line_stream(line);
          HypernodeWeight node_weight;
          line_stream >> node_weight;
          hypernode_weights->push_back(node_weight);
        }
      }
    }
    file.close();
  } else {
    std::cerr << "Error: File not found: " << std::endl;
  }
}

static inline Hypergraph createHypergraphFromFile(const std::string& filename,
                                                  const PartitionID num_parts) {
  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HypernodeWeightVector hypernode_weights;
  HyperedgeWeightVector hyperedge_weights;
  readHypergraphFile(filename, num_hypernodes, num_hyperedges,
                     index_vector, edge_vector, &hyperedge_weights, &hypernode_weights);
  return Hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector,
                    num_parts, &hyperedge_weights, &hypernode_weights);
}


static inline void writeHypernodeWeights(std::ofstream& out_stream, const Hypergraph& hypergraph) {
  for (const HypernodeID& hn : hypergraph.nodes()) {
    out_stream << hypergraph.nodeWeight(hn) << std::endl;
  }
}

static inline void writeHGRHeader(std::ofstream& out_stream, const Hypergraph& hypergraph) {
  out_stream << hypergraph.initialNumEdges() << " " << hypergraph.initialNumNodes() << " ";
  if (hypergraph.type() != HypergraphType::Unweighted) {
    out_stream << static_cast<int>(hypergraph.type());
  }
  out_stream << std::endl;
}

static inline void writeHypergraphFile(const Hypergraph& hypergraph, const std::string& filename) {
  ASSERT(!filename.empty(), "No filename for hypergraph file specified");
  ALWAYS_ASSERT(!hypergraph.isModified(), "Hypergraph is modified. Reindexing HNs/HEs necessary.");

  std::ofstream out_stream(filename.c_str());
  writeHGRHeader(out_stream, hypergraph);

  for (const HyperedgeID& he : hypergraph.edges()) {
    if (hypergraph.type() == HypergraphType::EdgeWeights ||
        hypergraph.type() == HypergraphType::EdgeAndNodeWeights) {
      out_stream << hypergraph.edgeWeight(he) << " ";
    }
    for (const HypernodeID& pin : hypergraph.pins(he)) {
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


static inline void writeHypergraphToGraphMLFile(const Hypergraph& hypergraph,
                                                const std::string& filename,
                                                const std::vector<PartitionID>* hn_cluster_ids = nullptr,
                                                const std::vector<PartitionID>* he_cluster_ids = nullptr) {
  std::ofstream out_stream(filename.c_str());

  out_stream << R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>)"
             << R"( <graphml xmlns="http://graphml.graphdrawing.org/xmlns")"
             << R"( xmlns:java="http://www.yworks.com/xml/yfiles-common/1.0/java")"
             << R"( xmlns:sys="http://www.yworks.com/xml/yfiles-common/markup/primitives/2.0")"
             << R"( xmlns:x="http://www.yworks.com/xml/yfiles-common/markup/2.0")"
             << R"( xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance")"
             << R"( xmlns:y="http://www.yworks.com/xml/graphml")"
             << R"( xmlns:yed="http://www.yworks.com/xml/yed/3")"
             << R"( xsi:schemaLocation="http://graphml.graphdrawing.org/xmlns)"
             << R"(http://www.yworks.com/xml/schema/graphml/1.1/ygraphml.xsd">)"
             << std::endl;

  out_stream << R"(<key id="d0" for="node" attr.name="weight" attr.type="double"/>)" << std::endl;
  out_stream << R"(<key id="d1" for="node" attr.name="part" attr.type="int"/>)" << std::endl;
  out_stream << R"(<key id="d2" for="node" attr.name="iscutedge" attr.type="int"/>)" << std::endl;
  out_stream << R"(<key id="d7" for="node" attr.name="modclass" attr.type="int"/>)" << std::endl;
  out_stream << R"(<key id="d8" for="node" attr.name="color" attr.type="string"/>)" << std::endl;
  out_stream << R"(<graph id="G" edgedefault="undirected">)" << std::endl;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    out_stream << R"(<node id="n)" << hn << R"(">)" << std::endl;
    out_stream << R"(<data key="d0">)" << hypergraph.nodeWeight(hn) << "</data>" << std::endl;
    if (hn_cluster_ids != nullptr) {
      out_stream << R"(<data key="d7">)" << (*hn_cluster_ids)[hn] << "</data>" << std::endl;
    } else {
      out_stream << R"(<data key="d1">)" << hypergraph.partID(hn) << "</data>" << std::endl;
    }

    out_stream << R"(<data key="d2">)" << 42 << "</data>" << std::endl;
    out_stream << R"(<data key="d8">)" << "blue" << "</data>" << std::endl;
    out_stream << "</node>" << std::endl;
  }

  HyperedgeID edge_id = 0;
  for (const HyperedgeID& he : hypergraph.edges()) {
    // const HyperedgeID he_id = hypergraph.initialNumNodes() + he;
    out_stream << R"(<node id="h)" << he << R"(">)" << std::endl;
    out_stream << R"(<data key="d0">)" << hypergraph.edgeWeight(he) << "</data>" << std::endl;
    if (he_cluster_ids != nullptr) {
      out_stream << R"(<data key="d7">)" << (*he_cluster_ids)[he] << "</data>" << std::endl;
    } else {
      out_stream << R"(<data key="d1">)" << -1 << "</data>" << std::endl;
    }
    out_stream << R"(<data key="d2">)" << (hypergraph.connectivity(he) > 1) << "</data>" << std::endl;
    out_stream << R"(<data key="d8">)" << "red" << "</data>" << std::endl;
    out_stream << "</node>" << std::endl;
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      out_stream << R"(<edge id="e)" << edge_id++ << R"(" source="n)" << pin << R"(" target="h)"
                 << he << R"("/>)" << std::endl;
    }
  }

  out_stream << "</graph>" << std::endl;
  out_stream << "</graphml>" << std::endl;
  out_stream.close();
}


static inline void writeHypergraphForhMetisPartitioning(const Hypergraph& hypergraph,
                                                        const std::string& filename,
                                                        const Mapping& mapping) {
  ASSERT(!filename.empty(), "No filename for hMetis initial partitioning file specified");
  std::ofstream out_stream(filename.c_str());

  // coarse graphs always have edge and node weights, even if graph wasn't coarsend
  out_stream << hypergraph.currentNumEdges() << " " << hypergraph.currentNumNodes() << " ";
  out_stream << static_cast<int>(HypergraphType::EdgeAndNodeWeights);
  out_stream << std::endl;

  for (const HyperedgeID& he : hypergraph.edges()) {
    out_stream << hypergraph.edgeWeight(he) << " ";
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      ASSERT(mapping.find(pin) != mapping.end(), "No mapping found for pin " << pin);
      out_stream << mapping.find(pin)->second + 1 << " ";
    }
    out_stream << std::endl;
  }

  writeHypernodeWeights(out_stream, hypergraph);
  out_stream.close();
}

static inline void writeHypergraphForPaToHPartitioning(const Hypergraph& hypergraph,
                                                       const std::string& filename,
                                                       const Mapping& mapping) {
  ASSERT(!filename.empty(), "No filename for PaToH initial partitioning file specified");
  std::ofstream out_stream(filename.c_str());
  out_stream << 1;                     // 1-based indexing
  out_stream << " " << hypergraph.currentNumNodes() << " " << hypergraph.currentNumEdges() << " " << hypergraph.currentNumPins();
  out_stream << " " << 3 << std::endl;  // weighting scheme: both edge and node weights

  for (const HyperedgeID& he : hypergraph.edges()) {
    out_stream << hypergraph.edgeWeight(he) << " ";
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      ASSERT(mapping.find(pin) != mapping.end(), "No mapping found for pin " << pin);
      out_stream << mapping.find(pin)->second + 1 << " ";
    }
    out_stream << std::endl;
  }

  for (const HypernodeID& hn : hypergraph.nodes()) {
    out_stream << hypergraph.nodeWeight(hn) << " ";
  }
  out_stream << std::endl;
  out_stream.close();
}

static inline void writeHypergraphForPaToHPartitioning(const Hypergraph& hypergraph,
                                                       const std::string& filename) {
  ASSERT(!filename.empty(), "No filename for PaToH initial partitioning file specified");
  std::ofstream out_stream(filename.c_str());
  out_stream << 0;                     // 0-based indexing
  out_stream << " " << hypergraph.currentNumNodes() << " " << hypergraph.currentNumEdges() << " " << hypergraph.currentNumPins();
  out_stream << " " << 3 << std::endl;  // weighting scheme: both edge and node weights

  for (const HyperedgeID& he : hypergraph.edges()) {
    out_stream << hypergraph.edgeWeight(he) << " ";
    for (const HypernodeID& pin : hypergraph.pins(he)) {
      // ASSERT(mapping.find(pin) != mapping.end(), "No mapping found for pin " << pin);
      out_stream << pin << " ";
    }
    out_stream << std::endl;
  }

  for (const HypernodeID& hn : hypergraph.nodes()) {
    out_stream << hypergraph.nodeWeight(hn) << " ";
  }
  out_stream << std::endl;
  out_stream.close();
}


static inline void readPartitionFile(const std::string& filename, std::vector<PartitionID>& partition) {
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

static inline void writePartitionFile(const Hypergraph& hypergraph, const std::string& filename) {
  if (filename.empty()) {
    LOG << "No filename for partition file specified";
  } else {
    std::ofstream out_stream(filename.c_str());
    for (const HypernodeID& hn : hypergraph.nodes()) {
      out_stream << hypergraph.partID(hn) << std::endl;
    }
    out_stream.close();
  }
}

static inline void readFixedVertexFile(Hypergraph& hypergraph, const std::string& filename) {
  ASSERT(!filename.empty(), "No filename for partition file specified");
  std::ifstream file(filename);
  if (file) {
    PartitionID part;
    HypernodeID hn = 0;
    while (file >> part) {
      if (part != -1) {
        hypergraph.setFixedVertex(hn, part);
      }
      hn++;
    }
    file.close();
  } else {
    std::cerr << "Error: File not found: " << filename << std::endl;
  }
}

static inline void writeFixedVertexFile(const Hypergraph& hypergraph, const std::string& filename) {
  ASSERT(!filename.empty(), "No filename for partition file specified");
  std::ofstream out_stream(filename.c_str());
  for (const HypernodeID& hn : hypergraph.nodes()) {
    out_stream << hypergraph.fixedVertexPartID(hn) << std::endl;
  }
  out_stream.close();
}
}  // namespace io
}  // namespace kahypar
