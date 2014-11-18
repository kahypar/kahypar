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

inline HyperedgeWeight hyperedgeCut(const Hypergraph& hg) {
  HyperedgeWeight cut = 0;
  for (const HyperedgeID he : hg.edges()) {
    IncidenceIterator begin = hg.pins(he).begin();
    IncidenceIterator end = hg.pins(he).end();
    if (begin == end) {
      continue;
    }
    ASSERT(begin != end, "Accessing empty hyperedge");

    PartitionID partition = hg.partID(*begin);
    ++begin;

    for (IncidenceIterator pin_it = begin; pin_it != end; ++pin_it) {
      if (partition != hg.partID(*pin_it)) {
        DBG(dbg_metrics_hyperedge_cut, "Hyperedge " << he << " is cut-edge");
        cut += hg.edgeWeight(he);
        break;
      }
    }
  }
  return cut;
}

template <typename CoarsendToHmetisMapping, typename Partition>
inline HyperedgeWeight hyperedgeCut(const Hypergraph& hg, CoarsendToHmetisMapping&
                                    hg_to_hmetis, const Partition& partitioning) {
  HyperedgeWeight cut = 0;
  for (const HyperedgeID he : hg.edges()) {
    IncidenceIterator begin = hg.pins(he).begin();
    IncidenceIterator end = hg.pins(he).end();
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

inline double imbalance(const Hypergraph& hypergraph) {
  HypernodeWeight total_weight = 0;
  HypernodeWeight max_weight = 0;
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    total_weight += hypergraph.partWeight(i);
    max_weight = std::max(max_weight, hypergraph.partWeight(i));
  }
  return static_cast<double>(max_weight) /
      ceil(static_cast<double>(total_weight) / hypergraph.k()) - 1.0;
}

template <typename CoarsendToHmetisMapping, typename Partition>
inline double imbalance(const Hypergraph& hypergraph, CoarsendToHmetisMapping&
                        hg_to_hmetis, const Partition& partitioning) {
  std::vector<HypernodeWeight> part_weights(hypergraph.k(), 0);

  for (const HypernodeID hn : hypergraph.nodes()) {
    part_weights[partitioning[hg_to_hmetis[hn]]] += hypergraph.nodeWeight(hn);
  }

  HypernodeWeight total_weight = 0;
  HypernodeWeight max_weight = 0;
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    total_weight += part_weights[i];
    max_weight = std::max(max_weight, part_weights[i]);
  }
  return static_cast<double>(max_weight) /
      ceil(static_cast<double>(total_weight) / hypergraph.k()) - 1.0;
}

inline double avgHyperedgeDegree(const Hypergraph& hypergraph) {
  return static_cast<double>(hypergraph.numPins()) / hypergraph.numEdges();
}

inline double avgHypernodeDegree(const Hypergraph& hypergraph) {
  return static_cast<double>(hypergraph.numPins()) / hypergraph.numNodes();
}

inline HypernodeID rank(const Hypergraph& hypergraph) {
  HypernodeID rank = 0;
  for (const HyperedgeID he : hypergraph.edges()) {
    rank = std::max(rank, hypergraph.edgeSize(he));
  }
  return rank;
}

inline HypernodeID hyperedgeSizePercentile(const Hypergraph& hypergraph, int percentile) {
  std::vector<HypernodeID> he_sizes;
  he_sizes.reserve(hypergraph.numEdges());
  for (auto he : hypergraph.edges()) {
    he_sizes.push_back(hypergraph.edgeSize(he));
  }
  std::sort(he_sizes.begin(), he_sizes.end());

  size_t rank = ceil(static_cast<double>(percentile) / 100 * he_sizes.size());
  return he_sizes[rank];
}


inline HyperedgeID hypernodeDegreePercentile(const Hypergraph& hypergraph, int percentile) {
  std::vector<HyperedgeID> hn_degrees;
  hn_degrees.reserve(hypergraph.numNodes());
  for (auto hn : hypergraph.nodes()) {
    hn_degrees.push_back(hypergraph.nodeDegree(hn));
  }
  std::sort(hn_degrees.begin(), hn_degrees.end());

  size_t rank = ceil(static_cast<double>(percentile) / 100 * hn_degrees.size());
  return hn_degrees[rank];
}

inline void connectivityStats(const Hypergraph& hypergraph,
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
} // namespace metrics

#endif  // SRC_PARTITION_METRICS_H_
