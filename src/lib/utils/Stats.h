/***************************************************************************
 *  Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <map>
#include <sstream>
#include <string>

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/Metrics.h"

using defs::HypernodeID;
using defs::HyperedgeID;

using partition::Configuration;

namespace utils {
class Stats {
 private:
  using StatsMap = std::map<std::string, double>;

 public:
  Stats(const Stats&) = delete;
  Stats(Stats&&) = delete;
  Stats& operator= (const Stats&) = delete;
  Stats& operator= (Stats&&) = delete;

  void add(const Configuration& config, const std::string& key, double value) {
    if (config.partition.collect_stats) {
      _stats["v" + std::to_string(config.partition.current_v_cycle)
             + "_lk_" + std::to_string(config.partition.rb_lower_k)
             + "_uk_" + std::to_string(config.partition.rb_upper_k)
             + "_" + key] += value;
    }
  }

  void addToTotal(const Configuration& config, const std::string& key, double value) {
    if (config.partition.collect_stats) {
      _stats[key] += value;
    }
  }

  double get(const std::string& key) const {
    const auto& it = _stats.find(key);
    if (it != _stats.cend()) {
      return it->second;
    }
    return 0;
  }

  std::string toString() const {
    std::ostringstream s;
    for (auto& stat : _stats) {
      s << " " << stat.first << "=" << stat.second;
    }
    return s.str();
  }

  static Stats & instance() {
    static Stats instance;
    return instance;
  }

  std::string toConsoleString() const {
    std::ostringstream s;
    for (auto& stat : _stats) {
      s << stat.first << " = " << stat.second << "\n";
    }
    return s.str();
  }

 private:
  Stats() :
    _stats() { }
  StatsMap _stats;
};

#ifdef GATHER_STATS

static inline void gatherCoarseningStats(const Hypergraph& hypergraph,
                                         const int vcycle,
                                         const PartitionID k1,
                                         const PartitionID k2)  noexcept {
  std::map<HyperedgeID, HypernodeID> node_degree_map;
  std::multimap<HypernodeWeight, HypernodeID> node_weight_map;
  std::map<HyperedgeWeight, HypernodeID> edge_weight_map;
  std::map<HypernodeID, HyperedgeID> edge_size_map;
  HyperedgeWeight sum_exposed_he_weight = 0;
  for (HypernodeID hn : hypergraph.nodes()) {
    node_degree_map[hypergraph.nodeDegree(hn)] += 1;
    node_weight_map.emplace(std::pair<HypernodeWeight,
                                      HypernodeID>(hypergraph.nodeWeight(hn), hn));
  }
  for (HyperedgeID he : hypergraph.edges()) {
    edge_weight_map[hypergraph.edgeWeight(he)] += 1;
    sum_exposed_he_weight += hypergraph.edgeWeight(he);
    edge_size_map[hypergraph.edgeSize(he)] += 1;
  }
  // LOG("Hypernode weights:");
  // for (auto& entry : node_weight_map) {
  //   std::cout << "w(" << std::setw(10) << std::left << entry.second << "):";
  //   HypernodeWeight percent = ceil(entry.first * 100.0 / _config.coarsening.max_allowed_node_weight);
  //   for (HypernodeWeight i = 0; i < percent; ++i) {
  //     std::cout << "#";
  //   }
  //   std::cout << " " << entry.first << std::endl;
  // }
  // LOG("Hypernode degrees:");
  // for (auto& entry : node_degree_map) {
  //   std::cout << "deg=" << std::setw(4) << std::left << entry.first << ":";
  //   for (HyperedgeID i = 0; i < entry.second; ++i) {
  //     std::cout << "#";
  //   }
  //   std::cout << " " << entry.second << std::endl;
  // }
  // LOG("Hyperedge weights:");
  // for (auto& entry : edge_weight_map) {
  //   std::cout << "w=" << std::setw(4) << std::left << entry.first << ":";
  //   for (HyperedgeID i = 0; i < entry.second && i < 100; ++i) {
  //     std::cout << "#";
  //   }
  //   if (entry.second > 100) {
  //     std::cout << " ";
  //     for (HyperedgeID i = 0; i < (entry.second - 100) / 100; ++i) {
  //       std::cout << ".";
  //     }
  //   }
  //   std::cout << " " << entry.second << std::endl;
  // }
  // LOG("Hyperedge sizes:");
  // for (auto& entry : edge_size_map) {
  //   std::cout << "|he|=" << std::setw(4) << std::left << entry.first << ":";
  //   for (HyperedgeID i = 0; i < entry.second && i < 100; ++i) {
  //     std::cout << "#";
  //   }
  //   if (entry.second > 100) {
  //     std::cout << " ";
  //     for (HyperedgeID i = 0; i < entry.second / 100; ++i) {
  //       std::cout << ".";
  //     }
  //   }
  //   std::cout << " " << entry.second << std::endl;
  // }
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_numCoarseHNs", vcycle, hypergraph.numNodes());
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_numCoarseHEs", vcycle, hypergraph.numEdges());
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_SumExposedHEWeight", vcycle, sum_exposed_he_weight);
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_NumHNsToInitialNumHNsRATIO", vcycle,
                        static_cast<double>(hypergraph.numNodes()) / hypergraph.initialNumNodes());
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_NumHEsToInitialNumHEsRATIO", vcycle,
                        static_cast<double>(hypergraph.numEdges()) / hypergraph.initialNumEdges());
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_ExposedHEWeightToInitialExposedHEWeightRATIO", vcycle,
                        static_cast<double>(sum_exposed_he_weight) / hypergraph.initialNumEdges());
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_HEsizeMIN", vcycle,
                        (edge_size_map.size() > 0 ? edge_size_map.begin()->first : 0));
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_HEsizeMAX", vcycle,
                        (edge_size_map.size() > 0 ? edge_size_map.rbegin()->first : 0));
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_HEsizeAVG", vcycle,
                        metrics::avgHyperedgeDegree(hypergraph));
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_HNdegreeMIN", vcycle,
                        (node_degree_map.size() > 0 ? node_degree_map.begin()->first : 0));
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_HNdegreeMAX", vcycle,
                        (node_degree_map.size() > 0 ? node_degree_map.rbegin()->first : 0));
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_HNdegreeAVG", vcycle,
                        metrics::avgHypernodeDegree(hypergraph));
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_HNweightMIN", vcycle,
                        (node_weight_map.size() > 0 ? node_weight_map.begin()->first : 0));
  Stats::instance().add("l" + std::to_string(k1) + "_" + "u" + std::to_string(k2)
                        + "_HNweightMAX", vcycle,
                        (node_weight_map.size() > 0 ? node_weight_map.rbegin()->first : 0));
}

#else

static inline void gatherCoarseningStats(const Hypergraph&, const int, const PartitionID,
                                         const PartitionID) noexcept { }

#endif
}  // namespace utils
