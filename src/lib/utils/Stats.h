#ifndef SRC_LIB_STATISTICS_STATS_H_
#define SRC_LIB_STATISTICS_STATS_H_

#include <string>
#include <unordered_map>
#include <sstream>

#include "lib/definitions.h"

using defs::HypernodeID;
using defs::HyperedgeID;

namespace utils {

#ifdef GATHER_STATS

class Stats{
 private:
  typedef std::unordered_map<std::string, int> StatsMap;
  
 public:
  
  Stats() :
      _stats() { }

  void add(const std::string& key, int value) {
    _stats[key] = value;
  }

  std::string toString() const {
    std::ostringstream s;
    for (auto& statistic : _stats) {
      s << " " << statistic.first << "=" << statistic.second;
    }
    return s.str();
  }
  
 private:
  StatsMap _stats;
  DISALLOW_COPY_AND_ASSIGN(Stats);
};

#else

class Stats{
 public:
  void add(const std::string&, int) {}
  std::string toString() const {
    return std::string("");
  }
};

#endif

} // namespace utils


#endif  // SRC_LIB_STATISTICS_STATS_H_
