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
  std::cout << name << ":   [min: " << std::setw(5) << std::left
            << (vec.empty() ? 0 : vec.front())
            << "Q1: " << std::setw(10) << std::left << (vec.empty() ? 0 : quartiles.first)
            << "med: " << std::setw(10) << std::left << (vec.empty() ? 0 : math::median(vec))
            << "Q3: " << std::setw(10) << std::left << (vec.empty() ? 0 : quartiles.second)
            << "max: " << std::setw(10) << std::left << (vec.empty() ? 0 : vec.back())
            << "avg: " << std::setw(10) << std::left << avg
            << "sd: " << std::setw(10) << std::left << stdev
            << "]" << std::endl;
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

  std::cout << "***********************Hypergraph Information************************" << std::endl;
  std::cout << "Name : " << name << std::endl;
  std::cout << "Type: " << hypergraph.typeAsString() << std::endl;
  std::cout << "# HEs: " << hypergraph.currentNumEdges() << std::endl;
  internal::printStats("HE size  ", he_sizes, avg_he_size, stdev_he_size,
                       math::firstAndThirdQuartile(he_sizes));
  internal::printStats("HE weight", he_weights, avg_he_weight, stdev_he_weight,
                       math::firstAndThirdQuartile(he_weights));
  std::cout << "# HNs: " << hypergraph.currentNumNodes() << std::endl;
  internal::printStats("HN degree", hn_degrees, avg_hn_degree, stdev_hn_degree,
                       math::firstAndThirdQuartile(hn_degrees));
  internal::printStats("HN weight", hn_weights, avg_hn_weight, stdev_hn_weight,
                       math::firstAndThirdQuartile(hn_weights));
}

template <class Configuration>
inline void printPartitionerConfiguration(const Configuration& config) {
  std::cout << "*********************Partitioning Configuration**********************" << std::endl;
  std::cout << toString(config) << std::endl;
}

inline void printPartitioningResults(const Hypergraph& hypergraph,
                                     const Configuration& config,
                                     const std::chrono::duration<double>& elapsed_seconds) {
  std::cout << "***********************" << hypergraph.k()
            << "-way Partition Result************************" << std::endl;
  std::cout << "Hyperedge Cut  (minimize) = " << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "SOED           (minimize) = " << metrics::soed(hypergraph) << std::endl;
  std::cout << "(k-1)          (minimize) = " << metrics::km1(hypergraph) << std::endl;
  std::cout << "Absorption     (maximize) = " << metrics::absorption(hypergraph) << std::endl;
  std::cout << "Imbalance                 = " << metrics::imbalance(hypergraph, config)
            << std::endl;
  std::cout << "Partition time            = " << elapsed_seconds.count() << " s" << std::endl;

  std::cout << "  | initial parallel HE removal  = "
            << Stats::instance().get("InitialParallelHEremoval")
            << " s [currently not implemented]" << std::endl;
  std::cout << "  | initial large HE removal     = "
            << Stats::instance().get("InitialLargeHEremoval") << " s" << std::endl;
  std::cout << "  | min hash sparsifier          = "
            << Stats::instance().get("MinHashSparsifier") << " s" << std::endl;
  std::cout << "  | coarsening                   = "
            << Stats::instance().get("Coarsening") << " s" << std::endl;
  std::cout << "  | initial partitioning         = "
            << Stats::instance().get("InitialPartitioning") << " s" << std::endl;
  std::cout << "  | uncoarsening/refinement      = "
            << Stats::instance().get("UncoarseningRefinement") << " s" << std::endl;
  std::cout << "  | initial large HE restore     = "
            << Stats::instance().get("InitialLargeHErestore") << " s" << std::endl;
  std::cout << "  | initial parallel HE restore  = "
            << Stats::instance().get("InitialParallelHErestore")
            << " s [currently not implemented]" << std::endl;
  if (config.partition.global_search_iterations > 0) {
    std::cout << " | v-cycle coarsening              = "
              << Stats::instance().get("VCycleCoarsening") << " s" << std::endl;
    std::cout << " | v-cycle uncoarsening/refinement = "
              << Stats::instance().get("VCycleUnCoarseningRefinement") << " s" << std::endl;
  }
  std::cout << "Partition sizes and weights: " << std::endl;
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    std::cout << "|part" << i << "| = " << std::setw(10) << std::left << hypergraph.partSize(i)
              << " w(" << i << ") = " << hypergraph.partWeight(i) << std::endl;
  }
}

inline void printPartitioningStatistics() {
  std::cout << "*****************************Statistics******************************" << std::endl;
  std::cout << "numRemovedParalellHEs: Number of HEs that were removed because they were parallel to some other HE." << std::endl;
  std::cout << "removedSingleNodeHEWeight: Total weight of HEs that were removed because they contained only 1 HN.\n"
            << "This sum includes the weight of previously removed parallel HEs, because we sum over the edge weights" << std::endl;
  std::cout << Stats::instance().toConsoleString();
}

inline void printConnectivityStats(const std::vector<PartitionID>& connectivity_stats) {
  std::cout << "*************************Connectivity Values*************************" << std::endl;
  for (size_t i = 0; i < connectivity_stats.size(); ++i) {
    std::cout << "# HEs with λ=" << i << ": " << connectivity_stats[i] << std::endl;
  }
}
}  // namespace io
}  // namespace kahypar
