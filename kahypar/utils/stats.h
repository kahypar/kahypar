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
#include <map>
#include <sstream>
#include <string>

#include "kahypar/partition/context_enum_classes.h"

namespace kahypar {
template <class Context>
class Stats {
  using Log = std::map<std::string, double>;

 public:
  explicit Stats(const Context& context) :
    _context(context),
    _oss(),
    _parent(nullptr),
    _preprocessing_logs(),
    _coarsening_logs(),
    _ip_logs(),
    _local_search_logs(),
    _postprocessing_logs() { }

  Stats(const Context& context, Stats* parent) :
    _context(context),
    _oss(),
    _parent(parent),
    _preprocessing_logs(),
    _coarsening_logs(),
    _ip_logs(),
    _local_search_logs(),
    _postprocessing_logs() { }

  ~Stats() {
    if (_parent != nullptr) {
      serializeToParent();
    }
  }

  Stats(const Stats&) = delete;
  Stats& operator= (const Stats&) = delete;

  Stats(Stats&&) = delete;
  Stats& operator= (Stats&&) = delete;

  double & preprocessing(const std::string& key) {
    return _preprocessing_logs[key];
  }

  double & coarsening(const std::string& key) {
    return _coarsening_logs[key];
  }

  double & initialPartitioning(const std::string& key) {
    return _ip_logs[key];
  }

  double & localSearch(const std::string& key) {
    return _local_search_logs[key];
  }

  double & postprocessing(const std::string& key) {
    return _postprocessing_logs[key];
  }

  Stats & topLevel() {
    Stats* ptr = this;
    while (ptr->parent() != nullptr) {
      ptr = ptr->parent();
    }
    return *ptr;
  }

  std::ostringstream & serialize() {
    serializeToParent();
    return _oss;
  }

 private:
  std::ostringstream & parentOutputStream() {
    if (_parent) {
      return _parent->parentOutputStream();
    }
    return _oss;
  }

  Stats* parent() {
    return _parent;
  }

  void serializeToParent() {
    std::ostringstream& oss = parentOutputStream();
    serialize(_preprocessing_logs, prefix("preprocessing"), oss);
    serialize(_coarsening_logs, prefix("coarsening"), oss);
    serialize(_ip_logs, prefix("ip"), oss);
    serialize(_local_search_logs, prefix("local_search"), oss);
    serialize(_postprocessing_logs, prefix("postprocessing"), oss);
  }

  void serialize(const Log& log, const std::string& prefix, std::ostringstream& oss) {
    for (auto it = log.cbegin(); it != log.cend(); ++it) {
      oss << prefix << it->first << "=" << it->second << " ";
    }
  }

  std::string prefix(const std::string& phase) const {
    return "vcycle_" + std::to_string(_context.partition.current_v_cycle)
           + "-" + toString(_context.type)
           + "-bisection_" + std::to_string(_context.partition.rb_lower_k)
           + "_" + std::to_string(_context.partition.rb_upper_k)
           + "-" + phase + "-";
  }


  const Context& _context;
  std::ostringstream _oss;
  Stats* _parent;
  Log _preprocessing_logs;
  Log _coarsening_logs;
  Log _ip_logs;
  Log _local_search_logs;
  Log _postprocessing_logs;
};
}  // namespace kahypar
