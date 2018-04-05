/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2018 Tobias Heuer <tobias.heuer@gmx.net>
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
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/context.h"

namespace kahypar {
namespace fixed_vertices {
static constexpr bool debug = false;

static inline void partition(Hypergraph& input_hypergraph,
                             const Context& original_context) {
  ASSERT([&]() {
      for (const HypernodeID& hn : input_hypergraph.nodes()) {
        if (input_hypergraph.isFixedVertex(hn) &&
            input_hypergraph.partID(hn) != Hypergraph::kInvalidPartition) {
          LOG << "Hypernode" << hn << "is a fixed vertex and already partitioned";
          return false;
        } else if (!input_hypergraph.isFixedVertex(hn) &&
                   input_hypergraph.partID(hn) == Hypergraph::kInvalidPartition) {
          LOG << "Hypernode" << hn << "is not a fixed vertex and unpartitioned";
          return false;
        }
      }
      return true;
    } (), "Precondition check for fixed vertex assignment failed");

  for (const HypernodeID& hn : input_hypergraph.fixedVertices()) {
    input_hypergraph.setNodePart(hn, input_hypergraph.fixedVertexPartID(hn));
  }
}
}  // namespace fixed_vertices
}  // namespace kahypar
