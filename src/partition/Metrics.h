/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_METRICS_H_
#define SRC_PARTITION_METRICS_H_
#include <cmath>

#include <algorithm>
#include <vector>

#include "lib/definitions.h"

using defs::HyperedgeWeight;
using defs::HypernodeWeight;
using defs::IncidenceIterator;
using defs::Hypergraph;
using defs::PartitionID;
using defs::HypernodeID;
using defs::HyperedgeID;

namespace metrics {
static const bool dbg_metrics_hyperedge_cut = false;

static inline HyperedgeWeight hyperedgeCut(const Hypergraph& hg) {
  HyperedgeWeight cut = 0;
  for (const HyperedgeID he : hg.edges()) {
    if (hg.connectivity(he) > 1) {
      DBG(dbg_metrics_hyperedge_cut, "Hyperedge " << he << " is cut-edge");
      cut += hg.edgeWeight(he);
    }
  }
  return cut;
}

static inline HyperedgeWeight soed(const Hypergraph& hg) {
  HyperedgeWeight soed = 0;
  for (const HyperedgeID he : hg.edges()) {
    if (hg.connectivity(he) > 1) {
      soed += hg.connectivity(he) * hg.edgeWeight(he);
    }
  }
  return soed;
}

static inline HyperedgeWeight kMinus1(const Hypergraph& hg) {
  HyperedgeWeight k_minus_1 = 0;
  for (const HyperedgeID he : hg.edges()) {
    k_minus_1 += (hg.connectivity(he) - 1) * hg.edgeWeight(he);
  }
  return k_minus_1;
}

static inline double absorption(const Hypergraph& hg) {
  double absorption_val = 0.0;
  for (PartitionID part = 0; part < hg.k(); ++part) {
    for (const HyperedgeID he : hg.edges()) {
      if (hg.pinCountInPart(he, part) > 0 && hg.edgeSize(he) > 1) {
        absorption_val += static_cast<double>((hg.pinCountInPart(he, part) - 1)) / (hg.edgeSize(he) - 1)
                          * hg.edgeWeight(he);
      }
    }
  }
  return absorption_val;
}

template <typename CoarsendToHmetisMapping, typename Partition>
static inline HyperedgeWeight hyperedgeCut(const Hypergraph& hg, CoarsendToHmetisMapping&
                                           hg_to_hmetis, const Partition& partitioning) {
  HyperedgeWeight cut = 0;
  for (const HyperedgeID he : hg.edges()) {
    auto begin = hg.pins(he).first;
    auto end = hg.pins(he).second;
    if (begin == end) {
      continue;
    }
    ASSERT(begin != end, "Accessing empty hyperedge");

    PartitionID partition = partitioning[hg_to_hmetis[*begin]];
    ++begin;

    for (IncidenceIterator pin_it = begin; pin_it != end; ++pin_it) {
      if (partition != partitioning[hg_to_hmetis[*pin_it]]) {
        DBG(dbg_metrics_hyperedge_cut, "Hyperedge " << he << " is cut-edge");
        cut += hg.edgeWeight(he);
        break;
      }
    }
  }
  return cut;
}

static inline double imbalance(const Hypergraph& hypergraph, const PartitionID k) {
  HypernodeWeight total_weight = 0;
  HypernodeWeight max_weight = 0;
  for (PartitionID i = 0; i != k; ++i) {
    total_weight += hypergraph.partWeight(i);
    max_weight = std::max(max_weight, hypergraph.partWeight(i));
  }
  return static_cast<double>(max_weight) /
         ceil(static_cast<double>(total_weight) / k) - 1.0;
}

template <typename CoarsendToHmetisMapping, typename Partition>
static inline double imbalance(const Hypergraph& hypergraph, CoarsendToHmetisMapping&
                               hg_to_hmetis, const Partition& partitioning,
                               const PartitionID k) {
  std::vector<HypernodeWeight> part_weights(k, 0);

  for (const HypernodeID hn : hypergraph.nodes()) {
    part_weights[partitioning[hg_to_hmetis[hn]]] += hypergraph.nodeWeight(hn);
  }

  HypernodeWeight total_weight = 0;
  HypernodeWeight max_weight = 0;
  for (PartitionID i = 0; i != k; ++i) {
    total_weight += part_weights[i];
    max_weight = std::max(max_weight, part_weights[i]);
  }

  return static_cast<double>(max_weight) /
         ceil(static_cast<double>(total_weight) / k) - 1.0;
}

static inline double avgHyperedgeDegree(const Hypergraph& hypergraph) {
  return static_cast<double>(hypergraph.numPins()) / hypergraph.numEdges();
}

static inline double avgHypernodeDegree(const Hypergraph& hypergraph) {
  return static_cast<double>(hypergraph.numPins()) / hypergraph.numNodes();
}

static inline double avgHypernodeWeight(const Hypergraph& hypergraph) {
  HypernodeWeight sum = 0;
  for (const HypernodeID hn : hypergraph.nodes()) {
    sum += hypergraph.nodeWeight(hn);
  }
  return sum / hypergraph.numNodes();
}

static inline double hypernodeWeightVariance(const Hypergraph& hypergraph) {
  double m = avgHypernodeWeight(hypergraph);

  double accum = 0.0;
  for (const HypernodeID hn : hypergraph.nodes()) {
    accum += (hypergraph.nodeWeight(hn) - m) * (hypergraph.nodeWeight(hn) - m);
  }

  return accum / (hypergraph.numNodes() - 1);
}

static inline double hypernodeDegreeVariance(const Hypergraph& hypergraph) {
  double m = avgHypernodeDegree(hypergraph);

  double accum = 0.0;
  for (const HypernodeID hn : hypergraph.nodes()) {
    accum += (hypergraph.nodeDegree(hn) - m) * (hypergraph.nodeDegree(hn) - m);
  }

  return accum / (hypergraph.numNodes() - 1);
}

static inline double hyperedgeSizeVariance(const Hypergraph& hypergraph) {
  double m = avgHyperedgeDegree(hypergraph);

  double accum = 0.0;
  for (const HyperedgeID he : hypergraph.edges()) {
    accum += (hypergraph.edgeSize(he) - m) * (hypergraph.edgeSize(he) - m);
  }

  return accum / (hypergraph.numEdges() - 1);
}

static inline HypernodeID hyperedgeSizePercentile(const Hypergraph& hypergraph, int percentile) {
  std::vector<HypernodeID> he_sizes;
  he_sizes.reserve(hypergraph.numEdges());
  for (auto he : hypergraph.edges()) {
    he_sizes.push_back(hypergraph.edgeSize(he));
  }
  ASSERT(!he_sizes.empty(), "Hypergraph does not contain any hyperedges");
  std::sort(he_sizes.begin(), he_sizes.end());

  size_t rank = ceil(static_cast<double>(percentile) / 100 * (he_sizes.size() - 1));
  return he_sizes[rank];
}


static inline HyperedgeID hypernodeDegreePercentile(const Hypergraph& hypergraph, int percentile) {
  std::vector<HyperedgeID> hn_degrees;
  hn_degrees.reserve(hypergraph.numNodes());
  for (auto hn : hypergraph.nodes()) {
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
  for (auto he : hypergraph.edges()) {
    max_connectivity = std::max(max_connectivity, hypergraph.connectivity(he));
  }
  connectivity_stats.resize(max_connectivity, 0);

  for (auto he : hypergraph.edges()) {
    ++connectivity_stats[hypergraph.connectivity(he)];
  }
}
}  // namespace metrics

#endif  // SRC_PARTITION_METRICS_H_
