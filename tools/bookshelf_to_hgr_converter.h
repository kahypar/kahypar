/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/macros.h"

using kahypar::HypernodeID;

static inline void convertBookshelfToHgr(const std::string& bookshelf_source_filename,
                                         const std::string& hgr_target_filename) {
  std::ifstream bookshelf_stream(bookshelf_source_filename);
  std::string line;

  // get header line
  std::getline(bookshelf_stream, line);
  if (line != "UCLA nets 1.0") {
    std::cout << "File has wrong header" << std::endl;
    std::exit(-1);
  }

  // skip any comments
  std::getline(bookshelf_stream, line);
  while (line[0] == '#') {
    std::getline(bookshelf_stream, line);
    std::cout << line << std::endl;
  }

  // get number of nets and number of pins
  std::getline(bookshelf_stream, line);
  ALWAYS_ASSERT(line.find("NumNets") != std::string::npos, "Error");
  const uint32_t num_hyperedges = std::stoi(line.substr(line.find_last_of(" ") + 1));
  std::getline(bookshelf_stream, line);
  const uint32_t num_pins = std::stoi(line.substr(line.find_last_of(" ") + 1));

  std::cout << V(num_hyperedges) << std::endl;
  std::cout << V(num_pins) << std::endl;

  // skip empty line
  std::getline(bookshelf_stream, line);

  std::unordered_map<std::string, HypernodeID> node_to_hn;
  std::vector<std::vector<HypernodeID> > hyperedges;

  // since netlists can contain nets with duplicate pins
  // we use this set to make sure that we have each pin only
  // once in each net.
  std::unordered_set<HypernodeID> contained_pins;

  const std::regex digit_regex("[[:digit:]]+");
  const std::regex pin_regex("(o|p)[[:digit:]]+");
  std::smatch match;

  HypernodeID num_hypernodes = 0;
  HypernodeID num_actual_pins = 0;
  HypernodeID num_duplicate_pins = 0;
  while (std::getline(bookshelf_stream, line)) {
    ALWAYS_ASSERT(line.substr(0, 9) == "NetDegree", "Error");
    // new hyperedge
    hyperedges.emplace_back(std::vector<HypernodeID>());
    contained_pins.clear();

    std::regex_search(line, match, digit_regex);
    const HypernodeID he_size = std::stoi(match.str());

    for (HypernodeID i = 0; i < he_size; ++i) {
      std::getline(bookshelf_stream, line);
      std::regex_search(line, match, pin_regex);
      const auto entry = node_to_hn.find(match.str());

      if (entry != node_to_hn.end()) {
        ALWAYS_ASSERT(entry->second < num_hypernodes, "Error");
        if (contained_pins.find(entry->second) == contained_pins.end()) {
          hyperedges.back().push_back(entry->second);
          contained_pins.insert(entry->second);
          ++num_actual_pins;
        } else {
          ++num_duplicate_pins;
        }
      } else {
        ALWAYS_ASSERT(contained_pins.find(num_hypernodes) == contained_pins.end(), "Error");
        hyperedges.back().push_back(num_hypernodes);
        contained_pins.insert(num_hypernodes);
        node_to_hn[match.str()] = num_hypernodes++;
        ++num_actual_pins;
      }
    }
  }

  bookshelf_stream.close();

  // sanity check
  ALWAYS_ASSERT(num_hyperedges == hyperedges.size(), "wrong # HEs");
  ALWAYS_ASSERT(num_actual_pins + num_duplicate_pins == num_pins, "wrong # pins");

  std::ofstream out_stream(hgr_target_filename.c_str());
  out_stream << num_hyperedges << " " << num_hypernodes << std::endl;
  for (const auto& hyperedge : hyperedges) {
    ALWAYS_ASSERT(!hyperedge.empty(), "Instance contains empty hypereges");
    for (auto pin_iter = hyperedge.cbegin(); pin_iter != hyperedge.cend(); ++pin_iter) {
      // hgr format is 1-based;
      out_stream << (*pin_iter + 1);
      if (pin_iter + 1 != hyperedge.end()) {
        out_stream << " ";
      }
    }
    out_stream << std::endl;
  }
  out_stream.close();
}
