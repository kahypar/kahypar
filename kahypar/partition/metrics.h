/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <cmath>

#include <algorithm>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"

namespace kahypar {
struct Metrics {
  HyperedgeWeight cut;
  HyperedgeWeight km1;
  double imbalance;

  void updateMetric(const HyperedgeWeight value, const Mode mode, const Objective objective) {
    if (mode == Mode::direct_kway) {
      switch (objective) {
        case Objective::cut:
          cut = value;
          break;
        case Objective::km1:
          km1 = value;
          break;
        default:
          LOG << "Unknown Objective";
          exit(-1);
      }
    } else if (mode == Mode::recursive_bisection) {
      // in recursive bisection, km1 is also optimized via the cut net metric
      cut = value;
    }
  }

  HyperedgeWeight getMetric(const Mode mode, const Objective objective) {
    if (mode == Mode::direct_kway) {
      switch (objective) {
        case Objective::cut: return cut;
        case Objective::km1: return km1;
        default:
          LOG << "Unknown Objective";
          exit(-1);
      }
    }
    ASSERT(mode == Mode::recursive_bisection);
    // in recursive bisection, km1 is also optimized via the cut net metric
    return cut;
  }
};


namespace metrics {
static constexpr bool debug = false;

static inline HyperedgeWeight hyperedgeCut(const Hypergraph& hg) {
  HyperedgeWeight cut = 0;
  for (const HyperedgeID& he : hg.edges()) {
    if (hg.connectivity(he) > 1) {
      DBG << "Hyperedge" << he << " is cut-edge";
      cut += hg.edgeWeight(he);
    }
  }
  return cut;
}

static inline HyperedgeWeight soed(const Hypergraph& hg) {
  HyperedgeWeight soed = 0;
  for (const HyperedgeID& he : hg.edges()) {
    if (hg.connectivity(he) > 1) {
      soed += hg.connectivity(he) * hg.edgeWeight(he);
    }
  }
  return soed;
}

static inline HyperedgeWeight km1(const Hypergraph& hg) {
  HyperedgeWeight k_minus_1 = 0;
  for (const HyperedgeID& he : hg.edges()) {
    k_minus_1 += std::max(hg.connectivity(he) - 1, 0) * hg.edgeWeight(he);
  }
  return k_minus_1;
}

static inline double absorption(const Hypergraph& hg) {
  double absorption_val = 0.0;
  for (PartitionID part = 0; part < hg.k(); ++part) {
    for (const HyperedgeID& he : hg.edges()) {
      if (hg.pinCountInPart(he, part) > 0 && hg.edgeSize(he) > 1) {
        absorption_val += static_cast<double>((hg.pinCountInPart(he, part) - 1)) / (hg.edgeSize(he) - 1)
                          * hg.edgeWeight(he);
      }
    }
  }
  return absorption_val;
}

static inline HyperedgeWeight objective(const Hypergraph& hg, const Objective& objective) {
  switch (objective) {
    case Objective::cut: return hyperedgeCut(hg);
    case Objective::km1: return km1(hg);
    default:
      LOG << "Unknown Objective";
      exit(-1);
  }
}


// Hide original imbalance definition that assumes Lmax0=Lmax1=Lmax
// This definition should only be used in assertions.
namespace internal {
inline double imbalance(const Hypergraph& hypergraph, const PartitionID k) {
  HypernodeWeight max_weight = hypergraph.partWeight(0);
  for (PartitionID i = 1; i != k; ++i) {
    max_weight = std::max(max_weight, hypergraph.partWeight(i));
  }
  return static_cast<double>(max_weight) /
         ceil(static_cast<double>(hypergraph.totalWeight()) / k) - 1.0;
}
}  // namespace internal

static inline double imbalance(const Hypergraph& hypergraph, const Context& context) {
  ASSERT(!context.partition.perfect_balance_part_weights.empty());
  ASSERT(context.partition.k == 2 || context.partition.use_individual_part_weights ||
         context.partition.perfect_balance_part_weights[0]
         == context.partition.perfect_balance_part_weights[1],
         "Imbalance cannot be calculated correctly");

  double max_balance = (hypergraph.partWeight(0) /
                        static_cast<double>(context.partition.perfect_balance_part_weights[0]));

  for (PartitionID i = 1; i != context.partition.k; ++i) {
    // If k > 2, then perfect_balance_part_weights[0] ==
    // perfect_balance_part_weights[1] == perfect_balance_part_weights[i], because then we
    // to direct k-way partitioning and each part has the same perfect_balance weight and Lmax
    const double balance_i =
      (hypergraph.partWeight(i) /
       static_cast<double>(context.partition.perfect_balance_part_weights[i]));
    max_balance = std::max(max_balance, balance_i);
  }

  // If we are in RB-mode and k!=2^x or we don't want to further partition blocks 0 and 1 into
  // an equal number of blocks, the old, natural imbalance definition does not hold.
  // However if k=2^x or we do partition into an equal number of blocks, this imbalance
  // calculation should give the same result as the old one.
  ASSERT(context.partition.perfect_balance_part_weights[0]
         != context.partition.perfect_balance_part_weights[1] ||
         context.partition.use_individual_part_weights ||
         max_balance - 1.0 == internal::imbalance(hypergraph, context.partition.k),
         "Incorrect Imbalance:" << (max_balance - 1.0) << "!="
                                << V(internal::imbalance(hypergraph, context.partition.k)));
  return max_balance - 1.0;
}

inline double imbalanceFixedVertices(const Hypergraph& hypergraph, const PartitionID k) {
  HypernodeWeight max_weight = hypergraph.fixedVertexPartWeight(0);
  for (PartitionID i = 1; i != k; ++i) {
    max_weight = std::max(max_weight, hypergraph.fixedVertexPartWeight(i));
  }
  return static_cast<double>(max_weight) /
         ceil(static_cast<double>(hypergraph.fixedVertexTotalWeight()) / k) - 1.0;
}

static inline double avgHyperedgeDegree(const Hypergraph& hypergraph) {
  return static_cast<double>(hypergraph.currentNumPins()) / hypergraph.currentNumEdges();
}

static inline double avgHypernodeDegree(const Hypergraph& hypergraph) {
  return static_cast<double>(hypergraph.currentNumPins()) / hypergraph.currentNumNodes();
}

static inline double avgHypernodeWeight(const Hypergraph& hypergraph) {
  HypernodeWeight sum = 0;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    sum += hypergraph.nodeWeight(hn);
  }
  return sum / hypergraph.currentNumNodes();
}

static inline double hypernodeWeightVariance(const Hypergraph& hypergraph) {
  double m = avgHypernodeWeight(hypergraph);

  double accum = 0.0;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    accum += (hypergraph.nodeWeight(hn) - m) * (hypergraph.nodeWeight(hn) - m);
  }

  return accum / (hypergraph.currentNumNodes() - 1);
}

static inline double hypernodeDegreeVariance(const Hypergraph& hypergraph) {
  double m = avgHypernodeDegree(hypergraph);

  double accum = 0.0;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    accum += (hypergraph.nodeDegree(hn) - m) * (hypergraph.nodeDegree(hn) - m);
  }

  return accum / (hypergraph.currentNumNodes() - 1);
}

static inline double hyperedgeSizeVariance(const Hypergraph& hypergraph) {
  double m = avgHyperedgeDegree(hypergraph);

  double accum = 0.0;
  for (const HyperedgeID& he : hypergraph.edges()) {
    accum += (hypergraph.edgeSize(he) - m) * (hypergraph.edgeSize(he) - m);
  }

  return accum / (hypergraph.currentNumEdges() - 1);
}

static inline HypernodeID hyperedgeSizePercentile(const Hypergraph& hypergraph, int percentile) {
  std::vector<HypernodeID> he_sizes;
  he_sizes.reserve(hypergraph.currentNumEdges());
  for (const auto& he : hypergraph.edges()) {
    he_sizes.push_back(hypergraph.edgeSize(he));
  }
  ASSERT(!he_sizes.empty(), "Hypergraph does not contain any hyperedges");
  std::sort(he_sizes.begin(), he_sizes.end());

  size_t rank = ceil(static_cast<double>(percentile) / 100 * (he_sizes.size() - 1));
  return he_sizes[rank];
}

static inline HyperedgeWeight correctMetric(const Hypergraph& hypergraph, const Context& context) {
  switch (context.partition.objective) {
    case Objective::km1:
      return km1(hypergraph);
    case Objective::cut:
      return hyperedgeCut(hypergraph);
    default:
      LOG << "The specified Objective is not listed in the Metrics";
      std::exit(0);
  }
}

static inline HyperedgeID hypernodeDegreePercentile(const Hypergraph& hypergraph, int percentile) {
  std::vector<HyperedgeID> hn_degrees;
  hn_degrees.reserve(hypergraph.currentNumNodes());
  for (const auto& hn : hypergraph.nodes()) {
    hn_degrees.push_back(hypergraph.nodeDegree(hn));
  }
  ASSERT(!hn_degrees.empty(), "Hypergraph does not contain any hypernodes");
  std::sort(hn_degrees.begin(), hn_degrees.end());

  size_t rank = ceil(static_cast<double>(percentile) / 100 * (hn_degrees.size() - 1));
  return hn_degrees[rank];
}

static inline void connectivityStats(const Hypergraph& hypergraph,
                                     std::vector<PartitionID>& connectivity_stats) {
  PartitionID max_connectivity = 0;
  for (const auto& he : hypergraph.edges()) {
    max_connectivity = std::max(max_connectivity, hypergraph.connectivity(he));
  }
  connectivity_stats.resize(max_connectivity, 0);

  for (const auto& he : hypergraph.edges()) {
    ++connectivity_stats[hypergraph.connectivity(he)];
  }
}
}  // namespace metrics
}  // namespace kahypar
