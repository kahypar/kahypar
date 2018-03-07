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
#include <array>
#include <map>
#include <sstream>
#include <string>

#include "kahypar/partition/context_enum_classes.h"

namespace kahypar {
enum class StatTag : uint8_t {
  Preprocessing,
  Coarsening,
  InitialPartitioning,
  LocalSearch,
  Postprocessing,
  COUNT
};


std::ostream& operator<< (std::ostream& os, StatTag tag) {
  switch (tag) {
    case StatTag::Preprocessing: return os << "preprocessing";
    case StatTag::Coarsening: return os << "coarsening";
    case StatTag::InitialPartitioning: return os << "initial_partitioning";
    case StatTag::LocalSearch: return os << "local_search";
    case StatTag::Postprocessing: return os << "postprocessing";
    case StatTag::COUNT: return os << "";
      // omit default case to trigger compiler warning for missing cases
  }
  return os << static_cast<uint8_t>(tag);
}

#ifdef GATHER_STATS
template <class Context>
class Stats {
  using Log = std::map<std::string, double>;

 public:
  explicit Stats(const Context& context) :
    _context(context),
    _oss(),
    _parent(nullptr),
    _logs() { }

  Stats(const Context& context, Stats* parent) :
    _context(context),
    _oss(),
    _parent(parent),
    _logs() { }

  ~Stats() {
    if (_parent != nullptr) {
      serializeToParent();
    }
  }

  Stats(const Stats&) = delete;
  Stats& operator= (const Stats&) = delete;

  Stats(Stats&&) = delete;
  Stats& operator= (Stats&&) = delete;

  void set(const StatTag& tag, const std::string& key, const double& value) {
    _logs[static_cast<size_t>(tag)][key] = value;
  }

  void add(const StatTag& tag, const std::string& key, const double& value) {
    _logs[static_cast<size_t>(tag)][key] = value;
  }

  Stats & topLevel() {
    if (_parent != nullptr) {
      return *_parent;
    }
    return *this;
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

  void serializeToParent() {
    std::ostringstream& oss = parentOutputStream();
    for (int i = 0; i < static_cast<int>(StatTag::COUNT); ++i) {
      serialize(_logs[i], static_cast<StatTag>(i), oss);
    }
  }

  void serialize(const Log& log, const StatTag& tag, std::ostringstream& oss) {
    for (auto it = log.cbegin(); it != log.cend(); ++it) {
      oss << "vcycle_" << std::to_string(_context.partition.current_v_cycle)
          << "-" << _context.type
          << "-bisection_" << std::to_string(_context.partition.rb_lower_k)
          << "_" << std::to_string(_context.partition.rb_upper_k)
          << "-" << tag << "-" << it->first << "=" << it->second << " ";
    }
  }

  const Context& _context;
  std::ostringstream _oss;
  Stats* _parent;
  std::array<Log, static_cast<int>(StatTag::COUNT)> _logs;
};

#else
template <class Context>
class Stats {
  using Log = std::map<std::string, double>;

 public:
  explicit Stats(const Context&) :
    _oss() { }

  Stats(const Context&, Stats*) :
    _oss() { }

  Stats(const Stats&) = delete;
  Stats& operator= (const Stats&) = delete;

  Stats(Stats&&) = delete;
  Stats& operator= (Stats&&) = delete;

  template <typename T>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void set(const StatTag&, const T&, const double&) { }

  template <typename T>
  KAHYPAR_ATTRIBUTE_ALWAYS_INLINE void add(const StatTag&, const T&, const double&) { }

  Stats & topLevel() { return *this; }

  std::ostringstream & serialize() {
    return _oss;
  }

 private:
  std::ostringstream _oss;
};
#endif
}  // namespace kahypar
