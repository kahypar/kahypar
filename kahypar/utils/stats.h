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
#include "kahypar/partition/context.h"
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

  void add(const Context& context, const std::string& key, double value) {
    if (context.partition.collect_stats) {
      _stats[internalKey(context, key)] += value;
    }
  }

  void addToTotal(const Context& context, const std::string& key, double value) {
    if (context.partition.collect_stats) {
      _stats[internalKey(context, key)] += value;
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

  // TODO(schlag): This is a rather hacky workaround.
  // In recursive bisection mode, we want to sum the timings of all coarsening,
  // IP, and local search phases to get the overall timings for each phase.
  // However, for direct k-way partitioning, we want to differentiate between
  // the time the main coarsening, IP, and local search phases took and the time
  // for those phases during initial partitioning.
  double get(const Context& context, const std::string& key) const {
    double value = 0.0;
    if (context.partition.mode == Mode::direct_kway) {
      const auto& it = _stats.find(internalKey(context, key));
      if (it != _stats.cend()) {
        value = it->second;
      }
    } else {
      const auto& it = _stats.find(key);
      if (it != _stats.cend()) {
        value = it->second;
      }
    }
    return value;
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
  std::string internalKey(const Context& context, const std::string& key) const {
    return "v" + std::to_string(context.partition.current_v_cycle)
           + "_lk_" + std::to_string(context.partition.rb_lower_k)
           + "_uk_" + std::to_string(context.partition.rb_upper_k)
           + "_" + key;
  }

  Stats() :
    _stats() { }
  StatsMap _stats;
};

#ifdef GATHER_STATS

static inline void gatherCoarseningStats(const Context& context,
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
  //   HypernodeWeight percent = ceil(entry.first * 100.0 / _context.coarsening.max_allowed_node_weight);
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
  Stats::instance().add(context, "numCoarseHNs", hypergraph.currentNumNodes());
  Stats::instance().add(context, "numCoarseHEs", hypergraph.currentNumEdges());
  Stats::instance().add(context, "sumExposedHEWeight", sum_exposed_he_weight);
  Stats::instance().add(context, "numHNsToInitialNumHNsRATIO",
                        static_cast<double>(hypergraph.currentNumNodes()) /
                        hypergraph.initialNumNodes());
  Stats::instance().add(context, "numHEsToInitialNumHEsRATIO",
                        static_cast<double>(hypergraph.currentNumEdges()) /
                        hypergraph.initialNumEdges());
  Stats::instance().add(context, "exposedHEWeightToInitialExposedHEWeightRATIO",
                        static_cast<double>(sum_exposed_he_weight) / hypergraph.initialNumEdges());
  Stats::instance().add(context, "HEsizeMIN",
                        (edge_size_map.size() > 0 ? edge_size_map.begin()->first : 0));
  Stats::instance().add(context, "HEsizeMAX",
                        (edge_size_map.size() > 0 ? edge_size_map.rbegin()->first : 0));
  Stats::instance().add(context, "HEsizeAVG",
                        metrics::avgHyperedgeDegree(hypergraph));
  Stats::instance().add(context, "HNdegreeMIN",
                        (node_degree_map.size() > 0 ? node_degree_map.begin()->first : 0));
  Stats::instance().add(context, "HNdegreeMAX",
                        (node_degree_map.size() > 0 ? node_degree_map.rbegin()->first : 0));
  Stats::instance().add(context, "HNdegreeAVG",
                        metrics::avgHypernodeDegree(hypergraph));
  Stats::instance().add(context, "HNweightMIN",
                        (node_weight_map.size() > 0 ? node_weight_map.begin()->first : 0));
  Stats::instance().add(context, "HNweightMAX",
                        (node_weight_map.size() > 0 ? node_weight_map.rbegin()->first : 0));
}

#else

static inline void gatherCoarseningStats(const Context&, const Hypergraph&) { }

#endif
}  // namespace kahypar
