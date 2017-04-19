/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "kahypar/definitions.h"
#include "kahypar/partition/configuration.h"
#include "kahypar/partition/metrics.h"

namespace kahypar {
class Stats {
 private:
  using StatsMap = std::map<std::string, double>;

 public:
  Stats(const Stats&) = delete;
  Stats(Stats&&) = delete;
  Stats& operator= (const Stats&) = delete;
  Stats& operator= (Stats&&) = delete;

  ~Stats() = default;

  void add(const Configuration& config, const std::string& key, double value) {
    if (config.partition.collect_stats) {
      _stats["v" + std::to_string(config.partition.current_v_cycle)
             + "_lk_" + std::to_string(config.partition.rb_lower_k)
             + "_uk_" + std::to_string(config.partition.rb_upper_k)
             + "_" + key] += value;
    }
  }

  void addToTotal(bool collect_stats, const std::string& key, double value) {
    if (collect_stats) {
      _stats[key] += value;
    }
  }

  void addToTotal(const Configuration& config, const std::string& key, double value) {
    addToTotal(config.partition.collect_stats, key, value);
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
    for (const auto& stat : _stats) {
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
    for (const auto& stat : _stats) {
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

static inline void gatherCoarseningStats(const Configuration& config,
                                         const Hypergraph& hypergraph) {
  std::map<HyperedgeID, HypernodeID> node_degree_map;
  std::multimap<HypernodeWeight, HypernodeID> node_weight_map;
  std::map<HyperedgeWeight, HypernodeID> edge_weight_map;
  std::map<HypernodeID, HyperedgeID> edge_size_map;
  HyperedgeWeight sum_exposed_he_weight = 0;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    node_degree_map[hypergraph.nodeDegree(hn)] += 1;
    node_weight_map.emplace(std::pair<HypernodeWeight,
                                      HypernodeID>(hypergraph.nodeWeight(hn), hn));
  }
  for (const HyperedgeID& he : hypergraph.edges()) {
    edge_weight_map[hypergraph.edgeWeight(he)] += 1;
    sum_exposed_he_weight += hypergraph.edgeWeight(he);
    edge_size_map[hypergraph.edgeSize(he)] += 1;
  }
  // LOG << "Hypernode weights:";
  // for (auto& entry : node_weight_map) {
  //   std::cout << "w(" << std::setw(10) << std::left << entry.second << "):";
  //   HypernodeWeight percent = ceil(entry.first * 100.0 / _config.coarsening.max_allowed_node_weight);
  //   for (HypernodeWeight i = 0; i < percent; ++i) {
  //     std::cout << "#";
  //   }
  //   std::cout << " " << entry.first << std::endl;
  // }
  // LOG << "Hypernode degrees:";
  // for (auto& entry : node_degree_map) {
  //   std::cout << "deg=" << std::setw(4) << std::left << entry.first << ":";
  //   for (HyperedgeID i = 0; i < entry.second; ++i) {
  //     std::cout << "#";
  //   }
  //   std::cout << " " << entry.second << std::endl;
  // }
  // LOG << "Hyperedge weights:";
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
  // LOG << "Hyperedge sizes:";
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
  Stats::instance().add(config, "numCoarseHNs", hypergraph.currentNumNodes());
  Stats::instance().add(config, "numCoarseHEs", hypergraph.currentNumEdges());
  Stats::instance().add(config, "sumExposedHEWeight", sum_exposed_he_weight);
  Stats::instance().add(config, "numHNsToInitialNumHNsRATIO",
                        static_cast<double>(hypergraph.currentNumNodes()) /
                        hypergraph.initialNumNodes());
  Stats::instance().add(config, "numHEsToInitialNumHEsRATIO",
                        static_cast<double>(hypergraph.currentNumEdges()) /
                        hypergraph.initialNumEdges());
  Stats::instance().add(config, "exposedHEWeightToInitialExposedHEWeightRATIO",
                        static_cast<double>(sum_exposed_he_weight) / hypergraph.initialNumEdges());
  Stats::instance().add(config, "HEsizeMIN",
                        (edge_size_map.size() > 0 ? edge_size_map.begin()->first : 0));
  Stats::instance().add(config, "HEsizeMAX",
                        (edge_size_map.size() > 0 ? edge_size_map.rbegin()->first : 0));
  Stats::instance().add(config, "HEsizeAVG",
                        metrics::avgHyperedgeDegree(hypergraph));
  Stats::instance().add(config, "HNdegreeMIN",
                        (node_degree_map.size() > 0 ? node_degree_map.begin()->first : 0));
  Stats::instance().add(config, "HNdegreeMAX",
                        (node_degree_map.size() > 0 ? node_degree_map.rbegin()->first : 0));
  Stats::instance().add(config, "HNdegreeAVG",
                        metrics::avgHypernodeDegree(hypergraph));
  Stats::instance().add(config, "HNweightMIN",
                        (node_weight_map.size() > 0 ? node_weight_map.begin()->first : 0));
  Stats::instance().add(config, "HNweightMAX",
                        (node_weight_map.size() > 0 ? node_weight_map.rbegin()->first : 0));
}

#else

static inline void gatherCoarseningStats(const Configuration&, const Hypergraph&) { }

#endif
}  // namespace kahypar
