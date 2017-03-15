/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <vector>

#include "kahypar/definitions.h"

namespace kahypar {
class SingleNodeHyperedgeRemover {
 public:
  struct RemovalResult {
    HyperedgeID num_removed_single_node_hes;
    HypernodeID num_unconnected_hns;
  };

  SingleNodeHyperedgeRemover() :
    _removed_hes() { }

  SingleNodeHyperedgeRemover(const SingleNodeHyperedgeRemover&) = delete;
  SingleNodeHyperedgeRemover& operator= (const SingleNodeHyperedgeRemover&) = delete;

  SingleNodeHyperedgeRemover(SingleNodeHyperedgeRemover&&) = delete;
  SingleNodeHyperedgeRemover& operator= (SingleNodeHyperedgeRemover&&) = delete;

  ~SingleNodeHyperedgeRemover() = default;

  RemovalResult removeSingleNodeHyperedges(Hypergraph& hypergraph) {
    RemovalResult result { 0, 0 };
    for (const HyperedgeID& he : hypergraph.edges()) {
      if (hypergraph.edgeSize(he) == 1) {
        ++result.num_removed_single_node_hes;
        if (hypergraph.nodeDegree(*hypergraph.pins(he).first) == 1) {
          ++result.num_unconnected_hns;
        }
        hypergraph.removeEdge(he);
        _removed_hes.push_back(he);
      }
    }
    return result;
  }

  void restoreSingleNodeHyperedges(Hypergraph& hypergraph) {
    for (auto he_it = _removed_hes.rbegin(); he_it != _removed_hes.rend(); ++he_it) {
      hypergraph.restoreEdge(*he_it);
    }
    _removed_hes.clear();
  }

 private:
  std::vector<HyperedgeID> _removed_hes;
};
}  // namespace kahypar
