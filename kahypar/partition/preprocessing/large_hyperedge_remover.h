/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <algorithm>
#include <vector>

#include "kahypar/definitions.h"

namespace kahypar {
static const bool dbg_partition_large_he_removal = false;
static const bool dbg_partition_large_he_restore = false;

class LargeHyperedgeRemover {
 public:
  LargeHyperedgeRemover() :
    _removed_hes() { }

  LargeHyperedgeRemover(const LargeHyperedgeRemover&) = delete;
  LargeHyperedgeRemover& operator= (const LargeHyperedgeRemover&) = delete;

  LargeHyperedgeRemover(LargeHyperedgeRemover&&) = delete;
  LargeHyperedgeRemover& operator= (LargeHyperedgeRemover&&) = delete;

  void removeLargeHyperedges(Hypergraph& hypergraph, const Configuration& config) {
    // Hyperedges with |he| > max(Lmax0,Lmax1) will always be cut edges, we therefore
    // remove them from the graph, to make subsequent partitioning easier.
    // In case of direct k-way partitioning, Lmaxi=Lmax0=Lmax1 for all i in (0..k-1).
    // In case of rb-based k-way partitioning however, Lmax0 might be different than Lmax1,
    // depending on how block 0 and block1 will be partitioned further.
    const HypernodeWeight max_part_weight =
      std::max(config.partition.max_part_weights[0], config.partition.max_part_weights[1]);
    if (hypergraph.type() == Hypergraph::Type::Unweighted) {
      for (const HyperedgeID he : hypergraph.edges()) {
        if (hypergraph.edgeSize(he) > max_part_weight) {
          DBG(dbg_partition_large_he_removal,
              "Hyperedge " << he << ": |pins|=" << hypergraph.edgeSize(he) << "   exceeds Lmax: "
              << max_part_weight);
          _removed_hes.push_back(he);
          hypergraph.removeEdge(he);
        }
      }
    } else if (hypergraph.type() == Hypergraph::Type::NodeWeights ||
               hypergraph.type() == Hypergraph::Type::EdgeAndNodeWeights) {
      for (const HyperedgeID he : hypergraph.edges()) {
        HypernodeWeight sum_pin_weights = 0;
        for (const HypernodeID pin : hypergraph.pins(he)) {
          sum_pin_weights += hypergraph.nodeWeight(pin);
        }
        if (sum_pin_weights > max_part_weight) {
          DBG(dbg_partition_large_he_removal,
              "Hyperedge " << he << ": w(pins) (" << sum_pin_weights << ")   exceeds Lmax: "
              << max_part_weight);
          _removed_hes.push_back(he);
          hypergraph.removeEdge(he);
        }
      }
    }
    Stats::instance().add(config, "numInitiallyRemovedLargeHEs",
                          _removed_hes.size());
    LOG("removed " << _removed_hes.size() << " HEs that had more than "
        << config.partition.hyperedge_size_threshold
        << " pins or weight of pins was greater than Lmax=" << max_part_weight);
  }

  void restoreLargeHyperedges(Hypergraph& hypergraph) {
    for (auto edge = _removed_hes.rbegin(); edge != _removed_hes.rend(); ++edge) {
      DBG(dbg_partition_large_he_removal, " restore Hyperedge " << *edge);
      hypergraph.restoreEdge(*edge);
    }
    _removed_hes.clear();
  }

 private:
  std::vector<HyperedgeID> _removed_hes;
};
}  // namespace kahypar
