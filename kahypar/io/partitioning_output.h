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
template <typename T>
void printStats(const std::string& name, const std::vector<T>& vec, double avg, double stdev,
                const std::pair<double, double>& quartiles) {
  const uint8_t width = 10;
  LOG << name
      << ":   [min:" << std::setw(width) << std::left << (vec.empty() ? 0 : vec.front())
      << "Q1:" << std::setw(width) << std::left << (vec.empty() ? 0 : quartiles.first)
      << "med:" << std::setw(width) << std::left << (vec.empty() ? 0 : math::median(vec))
      << "Q3:" << std::setw(width) << std::left << (vec.empty() ? 0 : quartiles.second)
      << "max:" << std::setw(width) << std::left << (vec.empty() ? 0 : vec.back())
      << "avg:" << std::setw(width) << std::left << avg
      << "sd:" << std::setw(width) << std::left << stdev
      << "]";
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

  LOG << "***********************Hypergraph Information************************";
  LOG << "Name :" << name;
  LOG << "Type:" << hypergraph.typeAsString();
  LOG << "# HEs:" << hypergraph.currentNumEdges();
  internal::printStats("HE size  ", he_sizes, avg_he_size, stdev_he_size,
                       math::firstAndThirdQuartile(he_sizes));
  internal::printStats("HE weight", he_weights, avg_he_weight, stdev_he_weight,
                       math::firstAndThirdQuartile(he_weights));
  LOG << "# HNs:" << hypergraph.currentNumNodes();
  internal::printStats("HN degree", hn_degrees, avg_hn_degree, stdev_hn_degree,
                       math::firstAndThirdQuartile(hn_degrees));
  internal::printStats("HN weight", hn_weights, avg_hn_weight, stdev_hn_weight,
                       math::firstAndThirdQuartile(hn_weights));
}

template <class Configuration>
inline void printPartitionerConfiguration(const Configuration& config) {
  LOG << "*********************Partitioning Configuration**********************";
  LOG << toString(config);
}

inline void printPartitioningResults(const Hypergraph& hypergraph,
                                     const Configuration& config,
                                     const std::chrono::duration<double>& elapsed_seconds) {
  LOG << "************************Partitioning Result**************************";
  LOG << "Hyperedge Cut  (minimize) =" << metrics::hyperedgeCut(hypergraph);
  LOG << "SOED           (minimize) =" << metrics::soed(hypergraph);
  LOG << "(k-1)          (minimize) =" << metrics::km1(hypergraph);
  LOG << "Absorption     (maximize) =" << metrics::absorption(hypergraph);
  LOG << "Imbalance                 =" << metrics::imbalance(hypergraph, config);
  LOG << "Partition time            =" << elapsed_seconds.count() << " s";

  LOG << "  | initial parallel HE removal  ="
      << Stats::instance().get("InitialParallelHEremoval") << " s [currently not implemented]";
  LOG << "  | initial large HE removal     = "
      << Stats::instance().get("InitialLargeHEremoval") << " s";
  LOG << "  | min hash sparsifier          = "
      << Stats::instance().get("MinHashSparsifier") << " s";
  LOG << "  | coarsening                   = "
      << Stats::instance().get("Coarsening") << " s";
  LOG << "  | initial partitioning         = "
      << Stats::instance().get("InitialPartitioning") << " s";
  LOG << "  | uncoarsening/refinement      = "
      << Stats::instance().get("UncoarseningRefinement") << " s";
  LOG << "  | initial large HE restore     = "
      << Stats::instance().get("InitialLargeHErestore") << " s";
  LOG << "  | initial parallel HE restore  = "
      << Stats::instance().get("InitialParallelHErestore")
      << " s [currently not implemented]";
  if (config.partition.global_search_iterations > 0) {
    LOG << " | v-cycle coarsening              = "
        << Stats::instance().get("VCycleCoarsening") << " s";
    LOG << " | v-cycle uncoarsening/refinement = "
        << Stats::instance().get("VCycleUnCoarseningRefinement") << " s";
  }
  LOG << "Partition sizes and weights: ";
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
  LOG << "*********************************************************************";
}

inline void printPartitioningStatistics() {
  LOG << "*****************************Statistics******************************";
  LOG << "numRemovedParalellHEs: Number of HEs that were removed because they were parallel to some other HE.";
  LOG << "removedSingleNodeHEWeight: Total weight of HEs that were removed because they contained only 1 HN.\n"
      << "This sum includes the weight of previously removed parallel HEs, because we sum over the edge weights";
  LOG << Stats::instance().toConsoleString();
}

inline void printConnectivityStats(const std::vector<PartitionID>& connectivity_stats) {
  LOG << "*************************Connectivity Values*************************";
  for (size_t i = 0; i < connectivity_stats.size(); ++i) {
    LOG << "# HEs with λ=" << i << ": " << connectivity_stats[i];
  }
}
}  // namespace io
}  // namespace kahypar
