/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/io/partitioning_output.h"
#include "kahypar/meta/reversed_iterable.h"
#include "kahypar/partition/context.h"

namespace kahypar {
class HypergraphDeduplicator {
  static constexpr bool debug = false;

 public:
  HypergraphDeduplicator() :
    _removed_parallel_hes(),
    _removed_identical_nodes() { }

  HypergraphDeduplicator(const HypergraphDeduplicator&) = delete;
  HypergraphDeduplicator& operator= (const HypergraphDeduplicator&) = delete;

  HypergraphDeduplicator(HypergraphDeduplicator&&) = delete;
  HypergraphDeduplicator& operator= (HypergraphDeduplicator&&) = delete;

  ~HypergraphDeduplicator() = default;

  void removeParallelHyperedges(Hypergraph& hypergraph) {
    _removed_parallel_hes = std::move(kahypar::ds::removeParallelHyperedges(hypergraph));
  }

  void removeIdenticalVertices(Hypergraph& hypergraph) {
    _removed_identical_nodes = std::move(kahypar::ds::removeIdenticalNodes(hypergraph));
  }

  void deduplicate(Hypergraph& hypergraph, const Context& context) {
    if (context.partition.verbose_output) {
      LOG << "Performing deduplication:";
    }
    removeIdenticalVertices(hypergraph);
    removeParallelHyperedges(hypergraph);
    if (context.partition.verbose_output) {
      LOG << "  # removed parallel hyperedges =" << _removed_parallel_hes.size() << " ";
      LOG << "  # removed identical vertices  =" << _removed_identical_nodes.size() << " ";
      io::printStripe();
    }
  }

  void restoreRedundancy(Hypergraph& hypergraph) {
    restoreParallelHyperedges(hypergraph);
    restoreIdenticalVertices(hypergraph);
  }

  void restoreIdenticalVertices(Hypergraph& hypergraph) {
    for (const auto& memento : reverse(_removed_identical_nodes)) {
      DBG << "restoring identical vertex: (" << memento.u << "," << memento.v << ")";
      hypergraph.uncontract(memento);
    }
  }

  void restoreParallelHyperedges(Hypergraph& hypergraph) {
    for (const auto& memento : reverse(_removed_parallel_hes)) {
      DBG << "restoring parallel HE: (" << memento.second << "," << memento.first << ")";
      hypergraph.restoreEdge(memento.first, memento.second);
      hypergraph.setEdgeWeight(memento.second,
                               hypergraph.edgeWeight(memento.second) -
                               hypergraph.edgeWeight(memento.first));
    }
  }

 private:
  std::vector<std::pair<HyperedgeID, HyperedgeID> > _removed_parallel_hes;
  std::vector<typename Hypergraph::ContractionMemento> _removed_identical_nodes;
};
}  // namespace kahypar
