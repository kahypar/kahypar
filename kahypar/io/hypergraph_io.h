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

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdlib>

#include "kahypar/definitions.h"
#include "kahypar/utils/timer.h"
#include "kahypar/utils/validate.h"

namespace kahypar {
namespace io {
using Mapping = std::unordered_map<HypernodeID, HypernodeID>;
using ErrorList = std::vector<validate::InputError>;
using validate::CheckedIStream;

static bool getNextLine(std::ifstream& file, std::string& line, size_t& line_number) {
  bool success = false;
  do {
    success = static_cast<bool>(std::getline(file, line));
    ++line_number;
    // skip any comments
  } while (success && line[0] == '%');
  return success;
}

static inline void readHGRHeader(std::ifstream& file, HyperedgeID& num_hyperedges,
                                 HypernodeID& num_hypernodes, HypergraphType& hypergraph_type,
                                 size_t& line_number) {
  std::string line;
  getNextLine(file, line, line_number);

  CheckedIStream sstream(line, line_number);
  int i = 0;
  if (sstream >> num_hyperedges && sstream >> num_hypernodes &&
      // note: hypergraph type may be omitted, assuming unweighted
      ( sstream.empty() || (sstream >> i && sstream.empty()) )) {
    hypergraph_type = static_cast<HypergraphType>(i);
  } else {
    ERROR("Invalid Header. Expected <num_hyperedges> <num_hypernodes> <type>", line_number);
  }
}

static inline void readHypergraphFile(const std::string& filename, HypernodeID& num_hypernodes,
                                      HyperedgeID& num_hyperedges,
                                      HyperedgeIndexVector& index_vector,
                                      HyperedgeVector& edge_vector,
                                      HyperedgeWeightVector* hyperedge_weights = nullptr,
                                      HypernodeWeightVector* hypernode_weights = nullptr,
                                      std::vector<size_t>* line_number_vector = nullptr) {
  ASSERT(!filename.empty(), "No filename for hypergraph file specified");
  HypergraphType hypergraph_type = HypergraphType::Unweighted;
  std::ifstream file(filename);
  size_t line_number = 0;
  if (file) {
    readHGRHeader(file, num_hyperedges, num_hypernodes, hypergraph_type, line_number);
    if (hypergraph_type != HypergraphType::Unweighted &&
        hypergraph_type != HypergraphType::EdgeWeights &&
        hypergraph_type != HypergraphType::NodeWeights &&
        hypergraph_type != HypergraphType::EdgeAndNodeWeights) {
      ERROR("Invalid hypergraph type", line_number);
    }

    const bool has_hyperedge_weights = hypergraph_type == HypergraphType::EdgeWeights ||
                                       hypergraph_type == HypergraphType::EdgeAndNodeWeights ?
                                       true : false;
    const bool has_hypernode_weights = hypergraph_type == HypergraphType::NodeWeights ||
                                       hypergraph_type == HypergraphType::EdgeAndNodeWeights ?
                                       true : false;

    index_vector.reserve(static_cast<size_t>(num_hyperedges) +  /*sentinel*/ 1);
    index_vector.push_back(edge_vector.size());
    if (line_number_vector != nullptr) {
      line_number_vector->reserve(static_cast<size_t>(num_hyperedges) +
                                  (has_hypernode_weights ? static_cast<size_t>(num_hypernodes) : 0));
      line_number_vector->clear();
    }

    std::string line;
    for (HyperedgeID i = 0; i < num_hyperedges; ++i) {
      getNextLine(file, line, line_number);
      CheckedIStream line_stream(line, line_number);
      if (line_stream.empty()) {
        ERROR("Hyperedge is empty", line_number);
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
        if (pin == 0) {
          ERROR("Invalid index 0 for pin. Vertex indices start with 1", line_number);
        }
        // Hypernode IDs start from 0
        --pin;
        edge_vector.push_back(pin);
      }
      index_vector.push_back(edge_vector.size());
      if (line_number_vector != nullptr) {
        line_number_vector->push_back(line_number);
      }
    }

    if (has_hypernode_weights) {
      if (hypernode_weights == nullptr) {
        LOG << " ****** ignoring hypernode weights ******";
      } else {
        ASSERT(hypernode_weights != nullptr, "Hypergraph has hypernode weights");
        for (HypernodeID i = 0; i < num_hypernodes; ++i) {
          getNextLine(file, line, line_number);
          CheckedIStream line_stream(line, line_number);
          HypernodeWeight node_weight;
          if (line_stream >> node_weight) {
            hypernode_weights->push_back(node_weight);
          } else {
            ERROR("Hypergraph has " << num_hypernodes << " hypernodes, but found only " << i << " weights");
          }
          if (!line_stream.empty()) {
            ERROR("Expected hypernode weight, but line contains multiple entries", line_number);
          }
          if (line_number_vector != nullptr) {
            line_number_vector->push_back(line_number);
          }
        }
      }
    }

    if (getNextLine(file, line, line_number) && !CheckedIStream(line).empty()) {
      WARNING("Unexpected content after end of hypergraph data", line_number);
    }
    file.close();
  } else {
    ERROR("File not found.");
  }
}

static inline void readHypergraphFile(const std::string& filename,
                                      HypernodeID& num_hypernodes,
                                      HyperedgeID& num_hyperedges,
                                      std::unique_ptr<size_t[]>& index_vector,
                                      std::unique_ptr<HypernodeID[]>& edge_vector,
                                      std::unique_ptr<HyperedgeWeight[]>& hyperedge_weights,
                                      std::unique_ptr<HypernodeWeight[]>& hypernode_weights) {
  HyperedgeIndexVector index_vec;
  HyperedgeVector edge_vec;
  HyperedgeWeightVector edge_weights_vec;
  HypernodeWeightVector node_weights_vec;

  readHypergraphFile(filename, num_hypernodes, num_hyperedges, index_vec,
                     edge_vec, &edge_weights_vec, &node_weights_vec);

  ASSERT(index_vector == nullptr);
  ASSERT(edge_vector == nullptr);
  index_vector = std::make_unique<size_t[]>(index_vec.size());
  edge_vector = std::make_unique<HypernodeID[]>(edge_vec.size());

  memcpy(index_vector.get(), index_vec.data(), index_vec.size() * sizeof(size_t));
  memcpy(edge_vector.get(), edge_vec.data(), edge_vec.size() * sizeof(HypernodeID));

  if (!edge_weights_vec.empty()) {
    ASSERT(hyperedge_weights == nullptr);
    hyperedge_weights = std::make_unique<HyperedgeWeight[]>(edge_weights_vec.size());
    memcpy(hyperedge_weights.get(), edge_weights_vec.data(),
           edge_weights_vec.size() * sizeof(HyperedgeWeight));
  }

  if (!node_weights_vec.empty()) {
    ASSERT(hypernode_weights == nullptr);
    hypernode_weights = std::make_unique<HypernodeWeight[]>(node_weights_vec.size());
    memcpy(hypernode_weights.get(), node_weights_vec.data(),
           node_weights_vec.size() * sizeof(HypernodeWeight));
  }
}

static inline void validateAndPrintErrors(const HypernodeID num_hypernodes, const HyperedgeID num_hyperedges,
                                          const size_t* hyperedge_indices, const HypernodeID* hyperedges,
                                          const HyperedgeWeight* hyperedge_weights, const HypernodeWeight* vertex_weights,
                                          const std::vector<size_t> line_numbers,
                                          std::vector<HyperedgeID>& ignored_hes,
                                          std::vector<size_t>& ignored_pins,
                                          const bool promote_warnings_to_errors) {
    ErrorList errors;
    HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
    validate::validateInput(num_hypernodes, num_hyperedges, hyperedge_indices, hyperedges,
                            hyperedge_weights, vertex_weights, &errors, &ignored_hes, &ignored_pins);
    validate::printErrors(num_hyperedges, errors, line_numbers, promote_warnings_to_errors);
    if (validate::containsFatalError(errors, promote_warnings_to_errors)) {
      exit(1);
    }
    HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
    Timer::instance().add(Timepoint::input_validation,
                          std::chrono::duration<double>(end - start).count());
}

static inline Hypergraph createHypergraphFromFile(const std::string& filename,
                                                  const PartitionID num_parts,
                                                  bool validate_input,
                                                  bool promote_warnings_to_errors) {
  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HypernodeWeightVector hypernode_weights;
  HyperedgeWeightVector hyperedge_weights;
  std::vector<size_t> line_numbers;
  readHypergraphFile(filename, num_hypernodes, num_hyperedges,
                     index_vector, edge_vector, &hyperedge_weights, &hypernode_weights,
                     validate_input ? &line_numbers : nullptr);

  if (validate_input) {
    std::vector<HyperedgeID> ignored_hes;
    std::vector<size_t> ignored_pins;
    validateAndPrintErrors(num_hypernodes, num_hyperedges, index_vector.data(), edge_vector.data(),
                           hyperedge_weights.empty() ? nullptr : hyperedge_weights.data(),
                           hypernode_weights.empty() ? nullptr : hypernode_weights.data(),
                           line_numbers, ignored_hes, ignored_pins, promote_warnings_to_errors);
    return Hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector,
                      num_parts, &hyperedge_weights, &hypernode_weights, ignored_hes, ignored_pins);
  }

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
    out_stream << "\n";
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
    ERROR("File not found.");
  }
}

static inline void writePartitionFile(const Hypergraph& hypergraph, const std::string& filename) {
  if (!filename.empty()) {
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
    ERROR("File not found: " << filename);
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
