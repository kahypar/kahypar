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

namespace metrics {
static const bool dbg_metrics_hyperedge_cut = false;

inline HyperedgeWeight hyperedgeCut(const Hypergraph& hg) {
  HyperedgeWeight cut = 0;
  for (const auto && he : hg.edges()) {
    IncidenceIterator begin = hg.pins(he).begin();
    IncidenceIterator end = hg.pins(he).end();
    if (begin == end) {
      continue;
    }
    ASSERT(begin != end, "Accessing empty hyperedge");

    PartitionID partition = hg.partitionIndex(*begin);
    ++begin;

    for (IncidenceIterator pin_it = begin; pin_it != end; ++pin_it) {
      if (partition != hg.partitionIndex(*pin_it)) {
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
                                    hg_to_hmetis, Partition& partitioning) {
  HyperedgeWeight cut = 0;
  for (const auto && he : hg.edges()) {
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
  std::vector<HypernodeWeight> partition_sizes(hypergraph.k(), 0);
  HypernodeWeight total_weight = 0;
  for (const auto && hn : hypergraph.nodes()) {
    ASSERT(hypergraph.partitionIndex(hn) < hypergraph.k() &&
           hypergraph.partitionIndex(hn) != Hypergraph::kInvalidPartition,
           "Invalid partition index for hypernode " << hn << ": " << hypergraph.partitionIndex(hn));
    partition_sizes[hypergraph.partitionIndex(hn)] += hypergraph.nodeWeight(hn);
    total_weight += hypergraph.nodeWeight(hn);
  }

  HypernodeWeight max_weight = 0;
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    max_weight = std::max(max_weight, partition_sizes[i]);
  }
  return 1.0 * max_weight * hypergraph.k() / total_weight - 1.0;
}

inline double avgHyperedgeDegree(const Hypergraph& hypergraph) {
  return static_cast<double>(hypergraph.numPins()) / hypergraph.numEdges();
}

inline double avgHypernodeDegree(const Hypergraph& hypergraph) {
  return static_cast<double>(hypergraph.numPins()) / hypergraph.numNodes();
}

template <class Weights>
inline void partitionWeights(const Hypergraph& hypergraph, Weights& weights) {
  for (const auto && hn : hypergraph.nodes()) {
    weights[hypergraph.partitionIndex(hn)] += hypergraph.nodeWeight(hn);
  }
}
} // namespace metrics

#endif  // SRC_PARTITION_METRICS_H_
