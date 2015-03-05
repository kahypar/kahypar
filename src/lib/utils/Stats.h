#ifndef SRC_LIB_STATISTICS_STATS_H_
#define SRC_LIB_STATISTICS_STATS_H_

#include <string>
#include <map>
#include <sstream>

#include "lib/definitions.h"

using defs::HypernodeID;
using defs::HyperedgeID;

namespace utils {

#ifdef GATHER_STATS

class Stats{
 private:
  using StatsMap = std::map<std::string, double>;
  
 public:
  Stats(const Stats &) = delete;
  Stats(Stats &&) = delete;
  Stats& operator= (const Stats&) = delete;
  Stats& operator= (Stats&&) = delete;

  Stats() :
      _stats() { }

  void add(const std::string& key, int vcycle, double value) {
    _stats[key + "_v" + std::to_string(vcycle)] += value;
  }

  std::string toString() const {
    std::ostringstream s;
    for (auto& stat : _stats) {
      s << " " << stat.first << "=" << stat.second;
    }
    return s.str();
  }

  std::string toConsoleString() const {
    std::ostringstream s;
    for (auto& stat : _stats) {
      s << stat.first << " = " << stat.second << "\n";
    }
    return s.str();
  }
  
 private:
  StatsMap _stats;
};

#else

class Stats{
 public:
  void add(const std::string& key, int vcycle, double value) {}
  std::string toString() const {
    return std::string("");
  }
   std::string toConsoleString() const {
    return std::string("");
  }
};

#endif

} // namespace utils


#endif  // SRC_LIB_STATISTICS_STATS_H_
