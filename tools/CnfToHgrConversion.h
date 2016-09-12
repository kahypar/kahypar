/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "macros.h"

namespace cnfconversion {
// Convert a SAT instance from simplified DIMACS format
// (http://www.satcompetition.org/2009/format-benchmarks2009.html) to hMetis HGR format.
// Conversion is done as described in:
// Papa, David A., and Igor L. Markov. "Hypergraph partitioning and clustering."
// Approximation algorithms and metaheuristics (2007): 61-1.
// Each literal in the formula maps to one vertex in the hypergraph. A boolean variable
// is a literal and its complement is another literal. Each clause maps to a hyperedge,
// which connects the vertices that correspond to the literals in the clause.

static inline void convertInstance(const std::string& cnf_source_filename,
                                   const std::string& hgr_target_filename) {
  std::ifstream cnf_file(cnf_source_filename);

  std::string line;
  std::getline(cnf_file, line);

  // skip any comments
  while (line[0] == 'c') {
    std::getline(cnf_file, line);
  }

  // parse header
  std::istringstream sstream(line);
  std::string header_start;
  sstream >> header_start;
  if (header_start.compare(std::string("p")) != 0) {
    LOG("Invalid header:");
    LOG("Expected: p");
    LOG("Actual: " << header_start);
    LOG("===============> aborting conversion");
    exit(-1);
  } else {
    sstream >> header_start;
    if (header_start.compare(std::string("cnf")) != 0) {
      LOG("Invalid header:");
      LOG("Expected: cnf");
      LOG("Actual: " << header_start);
      LOG("===============> aborting conversion");
      exit(-1);
    }
  }

  int num_variables = -1;
  int num_clauses = -1;
  sstream >> num_variables >> num_clauses;

  // number of literals = 2x number of variables
  int num_possible_literals = 2 * num_variables;

  LOGVAR(num_variables);
  LOGVAR(num_clauses);
  LOGVAR(num_possible_literals);

  std::vector<std::vector<int64_t> > hyperedges(num_clauses);

  // In HGR format, the HNs are numbered 1...N.
  // Since we don't know, how many of the possible 2 * num_variables
  // literals will be used, we use this map to store the mapping from
  // literal to hypernode id.
  std::unordered_map<int64_t, int64_t> literal_to_hypernode;

  int64_t literal = -1;
  int64_t num_hypernodes = 0;

  // parse hyperedges
  for (int i = 0; i < num_clauses; ++i) {
    std::getline(cnf_file, line);
    std::istringstream line_stream(line);
    while (line_stream >> literal) {
      if (literal != 0) {
        if (literal_to_hypernode.find(literal) == literal_to_hypernode.end()) {
          ++num_hypernodes;
          literal_to_hypernode[literal] = num_hypernodes;
        }
        hyperedges[i].push_back(literal_to_hypernode[literal]);
      }
    }
  }

  LOGVAR(literal_to_hypernode.size());

  std::ofstream out_stream(hgr_target_filename.c_str());
  out_stream << num_clauses << " " << num_hypernodes << std::endl;
  for (const auto& hyperedge : hyperedges) {
    ASSERT(!hyperedge.empty(), "SAT instance contains empty hypereges");
    for (auto pin_iter = hyperedge.cbegin(); pin_iter != hyperedge.cend(); ++pin_iter) {
      out_stream << *pin_iter;
      if (pin_iter + 1 != hyperedge.end()) {
        out_stream << " ";
      }
    }
    out_stream << std::endl;
  }
  out_stream.close();
}
}  // namespace cnfconversion
