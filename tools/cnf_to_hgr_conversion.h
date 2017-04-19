/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "kahypar/macros.h"

namespace cnfconversion {
// Convert a SAT instance from simplified DIMACS format
// (http://www.satcompetition.org/2009/format-benchmarks2009.html) to hMetis HGR format.


/*
 * We support three hypergraph representations:
 * primal:
 * Each variable is represented by a vertex and each clause is represented by a hyperedge. [1]
 * dual:
 * Each clause is represented by a vertex and each variable is represented by a hyperedge. The
 * hyperedge corresponding to the variable x contains the vertices that represent clauses containing
 * x (with or without negation). [1]
 * literal:
 * Each literal in the formula maps to one vertex in the hypergraph. A boolean variable
 * is a literal and its complement is another literal. Each clause maps to a hyperedge,
 * which connects the vertices that correspond to the literals in the clause. [2]
 *
 * [1] Mann, Z. A., & Papp, P. A. Guiding SAT solving by formula partitioning.
 * [2] Papa, David A., and Igor L. Markov. "Hypergraph partitioning and clustering."
 *     Approximation algorithms and metaheuristics (2007): 61-1.
 */
enum class HypergraphRepresentation : uint8_t {
  Primal,
  Dual,
  Literal,
};

static inline std::string toString(const HypergraphRepresentation& rep) {
  switch (rep) {
    case HypergraphRepresentation::Primal:
      return std::string("primal");
    case HypergraphRepresentation::Dual:
      return std::string("dual");
    case HypergraphRepresentation::Literal:
      return std::string("literal");
  }
  return std::string("");
}

struct PrimalRepresentationTag { };
struct DualRepresentationTag { };

using Hyperedges = std::vector<std::vector<uint64_t> >;

static inline void writeHGRFile(const Hyperedges& hyperedges,
                                const uint64_t num_hyperedges,
                                const uint64_t num_hypernodes,
                                const std::string& hgr_target_filename) {
  std::ofstream out_stream(hgr_target_filename.c_str());
  out_stream << num_hyperedges << " " << num_hypernodes << std::endl;
  uint64_t num_nonempty_hes = 0;
  for (const auto& hyperedge : hyperedges) {
    if (!hyperedge.empty()) {
      for (auto pin_iter = hyperedge.cbegin(); pin_iter != hyperedge.cend(); ++pin_iter) {
        out_stream << *pin_iter;
        if (pin_iter + 1 != hyperedge.end()) {
          out_stream << " ";
        }
      }
      out_stream << std::endl;
      ++num_nonempty_hes;
    }
  }
  out_stream.close();
  if (num_nonempty_hes != num_hyperedges) {
    std::remove(hgr_target_filename.c_str());
    std::cerr << "Hypergraph contained empty hyperedges" << std::endl;
    exit(-1);
  }
}

static inline void convertToLiteral(std::ifstream& cnf_file,
                                    const uint64_t num_variables,
                                    const uint64_t num_clauses,
                                    const std::string& hgr_target_filename) {
  // number of literals = 2x number of variables
  int num_possible_literals = 2 * num_variables;
  LOG << V(num_possible_literals);

  Hyperedges hyperedges(num_clauses);

  // In HGR format, the HNs are numbered 1...N.
  // Since we don't know, how many of the possible 2 * num_variables
  // literals will be used, we use this map to store the mapping from
  // literal to hypernode id.
  std::unordered_map<int64_t, int64_t> literal_to_hypernode;

  int64_t literal = -1;
  int64_t num_hypernodes = 0;

  std::string line;
  // parse hyperedges
  for (uint32_t i = 0; i < num_clauses; ++i) {
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
  LOG << V(literal_to_hypernode.size());
  writeHGRFile(hyperedges, num_clauses, num_hypernodes, hgr_target_filename);
}

template <typename T>
static inline void convertToT(std::ifstream& cnf_file,
                              const uint64_t num_variables,
                              const uint64_t num_clauses,
                              const std::string& hgr_target_filename) {
  uint64_t num_hyperedges = 0;
  if (std::is_same<T, PrimalRepresentationTag>::value) {
    num_hyperedges = num_clauses;
  } else if (std::is_same<T, DualRepresentationTag>::value) {
    num_hyperedges = num_variables;
  }

  Hyperedges hyperedges(num_hyperedges);
  std::unordered_map<uint64_t, uint64_t> hypernode_map;

  int64_t literal = 0;
  uint64_t num_hypernodes = 0;
  std::string line;
  // parse hyperedges
  for (uint32_t i = 0; i < num_clauses; ++i) {
    std::getline(cnf_file, line);
    std::istringstream line_stream(line);
    while (line_stream >> literal) {
      if (literal != 0) {
        const uint64_t variable = std::abs(literal);
        ALWAYS_ASSERT(variable <= num_variables, V(variable));
        if (hypernode_map.find(variable) == hypernode_map.end()) {
          ++num_hypernodes;
          hypernode_map[variable] = num_hypernodes;
        }
        if (std::is_same<T, PrimalRepresentationTag>::value) {
          hyperedges[i].push_back(hypernode_map[variable]);
        } else if (std::is_same<T, DualRepresentationTag>::value) {
          // internally HE-IDs start with 0, while HN-IDs start with 1
          hyperedges[hypernode_map[variable] - 1].push_back(i + 1);
        }
      }
    }
  }

  // in dual representation, the number of hypernodes equals the number of clauses
  if (std::is_same<T, DualRepresentationTag>::value) {
    num_hypernodes = num_clauses;
    // since hyperedges corresponding to unused variable ids might be empty,
    // we adjust the number of hyperedges accordingly
    for (const auto& hyperedge : hyperedges) {
      if (hyperedge.empty()) {
        --num_hyperedges;
      }
    }
  }

  writeHGRFile(hyperedges, num_hyperedges, num_hypernodes, hgr_target_filename);
}

static inline void convertInstance(const std::string& cnf_source_filename,
                                   const std::string& hgr_target_filename,
                                   const HypergraphRepresentation& representation) {
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
    LOG << "Invalid header:";
    LOG << "Expected: p";
    LOG << "Actual: " << header_start;
    LOG << "===============> aborting conversion";
    exit(-1);
  } else {
    sstream >> header_start;
    if (header_start.compare(std::string("cnf")) != 0) {
      LOG << "Invalid header:";
      LOG << "Expected: cnf";
      LOG << "Actual: " << header_start;
      LOG << "===============> aborting conversion";
      exit(-1);
    }
  }
  uint64_t num_variables = 0;
  uint64_t num_clauses = 0;
  sstream >> num_variables >> num_clauses;
  LOG << V(num_variables);
  LOG << V(num_clauses);

  switch (representation) {
    case HypergraphRepresentation::Primal:
      convertToT<PrimalRepresentationTag>(cnf_file, num_variables, num_clauses,
                                          hgr_target_filename);
      break;
    case HypergraphRepresentation::Dual:
      convertToT<DualRepresentationTag>(cnf_file, num_variables, num_clauses,
                                        hgr_target_filename);
      break;
    case HypergraphRepresentation::Literal:
      convertToLiteral(cnf_file, num_variables, num_clauses, hgr_target_filename);
  }
  cnf_file.close();
}
}  // namespace cnfconversion
