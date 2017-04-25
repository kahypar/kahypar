/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/macros.h"

namespace kahypar {
namespace phylo {
static constexpr bool debug = false;

using Hyperedges = std::vector<std::vector<HypernodeID> >;

static inline void convertToHypergraph(std::ifstream& repeats,
                                       const std::string& hgr_target_filename) {
  std::string line;
  std::getline(repeats, line);
  // skip any comments
  while (line[0] == '#') {
    std::getline(repeats, line);
  }
  HypernodeID num_nodes = 0;
  std::istringstream(line) >> num_nodes;
  DBG << V(num_nodes);


  std::multimap<HypernodeID, HypernodeID> tree_node_induced_hes;
  int identifier = -1;
  Hyperedges hes;
  while (std::getline(repeats, line)) {
    int max_identifier = 0;
    std::istringstream line_stream(line);
    for (HypernodeID i = 1; i <= num_nodes; ++i) {
      line_stream >> identifier;
      DBG << identifier;
      max_identifier = std::max(identifier, max_identifier);
      tree_node_induced_hes.emplace(identifier, i);
    }
    DBG << "end of line";
    for (int i = 1; i <= max_identifier; ++i) {
      auto range = tree_node_induced_hes.equal_range(i);
      if (std::distance(range.first, range.second) > 1) {
        hes.emplace_back();
        DBG << V(i);
        for (auto iter = range.first; iter != range.second; ++iter) {
          hes.back().push_back(iter->second);
          DBG << iter->second;
        }
        DBG << "";
      }
    }
    tree_node_induced_hes.clear();
    DBG << "-----";
  }

  std::ofstream out_stream(hgr_target_filename.c_str());
  out_stream << hes.size() << " " << num_nodes << std::endl;
  for (const auto& he : hes) {
    ALWAYS_ASSERT(he.size() > 1, V(he.size()));
    for (auto pin_iter = he.begin(); pin_iter != he.end(); ++pin_iter) {
      ALWAYS_ASSERT(*pin_iter > 0, V(*pin_iter));
      out_stream << *pin_iter;
      if (pin_iter + 1 != he.end()) {
        out_stream << " ";
      }
    }
    out_stream << std::endl;
  }
  out_stream.close();
}
}  // namespace phylo
}  // namespace kahypar
