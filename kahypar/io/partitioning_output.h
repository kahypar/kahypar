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

#include <algorithm>
#include <chrono>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/git_revision.h"
#include "kahypar/partition/configuration.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/utils/math.h"
#include "kahypar/utils/stats.h"

namespace kahypar {
namespace io {
namespace internal {
struct Statistic {
  uint64_t min = 0;
  uint64_t q1 = 0;
  uint64_t med = 0;
  uint64_t q3 = 0;
  uint64_t max = 0;
  double avg = 0.0;
  double sd = 0.0;
};

template <typename T>
Statistic createStats(const std::vector<T>& vec, const double avg, const double stdev) {
  internal::Statistic stats;
  if (!vec.empty()) {
    const auto quartiles = math::firstAndThirdQuartile(vec);
    stats.min = vec.front();
    stats.q1 = quartiles.first;
    stats.med = math::median(vec);
    stats.q3 = quartiles.second;
    stats.max = vec.back();
    stats.avg = avg;
    stats.sd = stdev;
  }
  return stats;
}


void printStats(const Statistic& he_size_stats,
                const Statistic& he_weight_stats,
                const Statistic& hn_deg_stats,
                const Statistic& hn_weight_stats) {
  // default double precision is 7
  const uint8_t double_width = 7;
  const uint8_t he_size_width = std::max(math::digits(he_size_stats.max), double_width) + 4;
  const uint8_t he_weight_width = std::max(math::digits(he_weight_stats.max), double_width) + 4;
  const uint8_t hn_deg_width = std::max(math::digits(hn_deg_stats.max), double_width) + 4;
  const uint8_t hn_weight_width = std::max(math::digits(hn_weight_stats.max), double_width) + 4;

  LOG << "HE size" << std::right << std::setw(he_size_width + 10)
      << "HE weight" << std::right << std::setw(he_weight_width + 8)
      << "HN degree" << std::right << std::setw(hn_deg_width + 8)
      << "HN weight";
  LOG << "| min=" << std::left << std::setw(he_size_width) << he_size_stats.min
      << " | min=" << std::left << std::setw(he_weight_width) << he_weight_stats.min
      << " | min=" << std::left << std::setw(hn_deg_width) << hn_deg_stats.min
      << " | min=" << std::left << std::setw(hn_weight_width) << hn_weight_stats.min;
  LOG << "| Q1 =" << std::left << std::setw(he_size_width) << he_size_stats.q1
      << " | Q1 =" << std::left << std::setw(he_weight_width) << he_weight_stats.q1
      << " | Q1 =" << std::left << std::setw(hn_deg_width) << hn_deg_stats.q1
      << " | Q1 =" << std::left << std::setw(hn_weight_width) << hn_weight_stats.q1;
  LOG << "| med=" << std::left << std::setw(he_size_width) << he_size_stats.med
      << " | med=" << std::left << std::setw(he_weight_width) << he_weight_stats.med
      << " | med=" << std::left << std::setw(hn_deg_width) << hn_deg_stats.med
      << " | med=" << std::left << std::setw(hn_weight_width) << hn_weight_stats.med;
  LOG << "| Q3 =" << std::left << std::setw(he_size_width) << he_size_stats.q3
      << " | Q3 =" << std::left << std::setw(he_weight_width) << he_weight_stats.q3
      << " | Q3 =" << std::left << std::setw(hn_deg_width) << hn_deg_stats.q3
      << " | Q3 =" << std::left << std::setw(hn_weight_width) << hn_weight_stats.q3;
  LOG << "| max=" << std::left << std::setw(he_size_width) << he_size_stats.max
      << " | max=" << std::left << std::setw(he_weight_width) << he_weight_stats.max
      << " | max=" << std::left << std::setw(hn_deg_width) << hn_deg_stats.max
      << " | max=" << std::left << std::setw(hn_weight_width) << hn_weight_stats.max;
  LOG << "| avg=" << std::left << std::setw(he_size_width) << he_size_stats.avg
      << " | avg=" << std::left << std::setw(he_weight_width) << he_weight_stats.avg
      << " | avg=" << std::left << std::setw(hn_deg_width) << hn_deg_stats.avg
      << " | avg=" << std::left << std::setw(hn_weight_width) << hn_weight_stats.avg;
  LOG << "| sd =" << std::left << std::setw(he_size_width) << he_size_stats.sd
      << " | sd =" << std::left << std::setw(he_weight_width) << he_weight_stats.sd
      << " | sd =" << std::left << std::setw(hn_deg_width) << hn_deg_stats.sd
      << " | sd =" << std::left << std::setw(hn_weight_width) << hn_weight_stats.sd;
}
}  // namespace internal

inline void printHypergraphInfo(const Hypergraph& hypergraph, const std::string& name) {
  std::vector<HypernodeID> he_sizes;
  std::vector<HyperedgeWeight> he_weights;
  std::vector<HyperedgeID> hn_degrees;
  std::vector<HypernodeWeight> hn_weights;
  he_sizes.reserve(hypergraph.currentNumEdges());
  he_weights.reserve(hypergraph.currentNumEdges());
  hn_degrees.reserve(hypergraph.currentNumNodes());
  hn_weights.reserve(hypergraph.currentNumNodes());

  const double avg_hn_degree = metrics::avgHypernodeDegree(hypergraph);
  double stdev_hn_degree = 0.0;
  for (const auto& hn : hypergraph.nodes()) {
    hn_degrees.push_back(hypergraph.nodeDegree(hn));
    hn_weights.push_back(hypergraph.nodeWeight(hn));
    stdev_hn_degree += (hypergraph.nodeDegree(hn) - avg_hn_degree) *
                       (hypergraph.nodeDegree(hn) - avg_hn_degree);
  }
  stdev_hn_degree = std::sqrt(stdev_hn_degree / (hypergraph.currentNumNodes() - 1));


  const double avg_he_size = metrics::avgHyperedgeDegree(hypergraph);
  double stdev_he_size = 0.0;
  for (const auto& he : hypergraph.edges()) {
    he_sizes.push_back(hypergraph.edgeSize(he));
    he_weights.push_back(hypergraph.edgeWeight(he));
    stdev_he_size += (hypergraph.edgeSize(he) - avg_he_size) *
                     (hypergraph.edgeSize(he) - avg_he_size);
  }
  stdev_he_size = std::sqrt(stdev_he_size / (hypergraph.currentNumEdges() - 1));

  std::sort(he_sizes.begin(), he_sizes.end());
  std::sort(he_weights.begin(), he_weights.end());
  std::sort(hn_degrees.begin(), hn_degrees.end());
  std::sort(hn_weights.begin(), hn_weights.end());

  const double avg_hn_weight = std::accumulate(hn_weights.begin(), hn_weights.end(), 0.0) /
                               static_cast<double>(hn_weights.size());
  const double avg_he_weight = std::accumulate(he_weights.begin(), he_weights.end(), 0.0) /
                               static_cast<double>(he_weights.size());

  double stdev_hn_weight = 0.0;
  for (const HypernodeWeight& hn_weight : hn_weights) {
    stdev_hn_weight += (hn_weight - avg_hn_weight) * (hn_weight - avg_hn_weight);
  }
  stdev_hn_weight = std::sqrt(stdev_hn_weight / (hypergraph.currentNumNodes() - 1));

  double stdev_he_weight = 0.0;
  for (const HyperedgeWeight& he_weight : he_weights) {
    stdev_he_weight += (he_weight - avg_he_weight) * (he_weight - avg_he_weight);
  }
  stdev_he_weight = std::sqrt(stdev_he_weight / (hypergraph.currentNumNodes() - 1));

  LOG << "Hypergraph Information";
  LOG << "Name :" << name;
  LOG << "Type:" << hypergraph.typeAsString();
  LOG << "# HNs :" << hypergraph.currentNumNodes()
      << "# HEs :" << hypergraph.currentNumEdges()
      << "# pins:" << hypergraph.currentNumPins();

  internal::printStats(internal::createStats(he_sizes, avg_he_size, stdev_he_size),
                       internal::createStats(he_weights, avg_he_weight, stdev_he_weight),
                       internal::createStats(hn_degrees, avg_hn_degree, stdev_hn_degree),
                       internal::createStats(hn_weights, avg_hn_weight, stdev_hn_weight));
}

inline void printPartSizesAndWeights(const Hypergraph& hypergraph) {
  HypernodeID max_part_size = 0;
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    max_part_size = std::max(max_part_size, hypergraph.partSize(i));
  }
  const uint8_t part_digits = math::digits(max_part_size);
  const uint8_t k_digits = math::digits(hypergraph.k());
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    LOG << "|part" << std::right << std::setw(k_digits) << i
        << std::setw(1) << "| =" << std::right << std::setw(part_digits) << hypergraph.partSize(i)
        << std::setw(1) << " w(" << std::right << std::setw(k_digits) << i
        << std::setw(1) << ") =" << std::right << std::setw(part_digits)
        << hypergraph.partWeight(i);
  }
}

inline void printPartitioningResults(const Hypergraph& hypergraph,
                                     const Configuration& config,
                                     const std::chrono::duration<double>& elapsed_seconds) {
  LOG << "********************************************************************************";
  LOG << "*                             Partitioning Result                              *";
  LOG << "********************************************************************************";
  LOG << "Objectives:";
  LOG << "Hyperedge Cut  (minimize) =" << metrics::hyperedgeCut(hypergraph);
  LOG << "SOED           (minimize) =" << metrics::soed(hypergraph);
  LOG << "(k-1)          (minimize) =" << metrics::km1(hypergraph);
  LOG << "Absorption     (maximize) =" << metrics::absorption(hypergraph);
  LOG << "Imbalance                 =" << metrics::imbalance(hypergraph, config);

  LOG << "\nPartition sizes and weights: ";
  printPartSizesAndWeights(hypergraph);

  LOG << "\nTimings:";
  LOG << "Partition time                   =" << elapsed_seconds.count() << "s";
  LOG << "  | initial parallel HE removal  ="
      << Stats::instance().get(config, "InitialParallelHEremovalTime")
      << "s [currently not implemented]";
  LOG << "  | initial large HE removal     ="
      << Stats::instance().get(config, "InitialLargeHEremovalTime") << "s";
  LOG << "  | min hash sparsifier          ="
      << Stats::instance().get(config, "MinHashSparsifierTime") << "s";
  if (config.preprocessing.enable_louvain_community_detection) {
    LOG << "  | community detection          ="
        << Stats::instance().get(config, "CommunityDetectionTime") << "s";
  }
  LOG << "  | coarsening                   ="
      << Stats::instance().get(config, "CoarseningTime") << "s";
  LOG << "  | initial partitioning         ="
      << Stats::instance().get(config, "InitialPartitioningTime") << "s";
  LOG << "  | uncoarsening/refinement      ="
      << Stats::instance().get(config, "UncoarseningRefinementTime") << "s";
  LOG << "  | initial large HE restore     ="
      << Stats::instance().get(config, "InitialLargeHErestoreTime") << "s";
  LOG << "  | initial parallel HE restore  ="
      << Stats::instance().get(config, "InitialParallelHErestoreTime")
      << " s [currently not implemented]";
  if (config.partition.global_search_iterations > 0) {
    LOG << " | v-cycle coarsening              = "
        << Stats::instance().get(config, "VCycleCoarseningTime") << "s";
    LOG << " | v-cycle uncoarsening/refinement ="
        << Stats::instance().get(config, "VCycleUnCoarseningRefinementTime") << "s";
  }
}

inline void printPartitioningStatistics() {
  LOG << "\nStatistics ********************************************************************";
  LOG << "numRemovedParalellHEs: Number of HEs that were removed because they were parallel to some other HE.";
  LOG << "removedSingleNodeHEWeight: Total weight of HEs that were removed because they contained only 1 HN.\n"
      << "This sum includes the weight of previously removed parallel HEs, because we sum over the edge weights";
  LOG << Stats::instance().toConsoleString();
}

inline void printConnectivityStats(const std::vector<PartitionID>& connectivity_stats) {
  LOG << "\nConnectivity Values ***********************************************************";
  for (size_t i = 0; i < connectivity_stats.size(); ++i) {
    LOG << "# HEs with λ=" << i << ": " << connectivity_stats[i];
  }
}

static inline void printBanner() {
  LOG << R"(+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++)";
  LOG << R"(+                    _  __     _   _       ____                               +)";
  LOG << R"(+                   | |/ /__ _| | | |_   _|  _ \ __ _ _ __                    +)";
  LOG << R"(+                   | ' // _` | |_| | | | | |_) / _` | '__|                   +)";
  LOG << R"(+                   | . \ (_| |  _  | |_| |  __/ (_| | |                      +)";
  LOG << R"(+                   |_|\_\__,_|_| |_|\__, |_|   \__,_|_|                      +)";
  LOG << R"(+                                    |___/                                    +)";
  LOG << R"(+                 Karlsruhe Hypergraph Partitioning Framework                 +)";
  LOG << R"(+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++)";
}
}  // namespace io
}  // namespace kahypar
