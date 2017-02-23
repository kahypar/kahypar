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

using namespace kahypar;

// ! \addtogroup tools
// ! \{

/*!
 * An Edge represents the adjacency between a hypernode vertex and
 * an hyperedge vertex in the bipartite graph representation.
 * The source of an edge is alway a hypernode vertex.
 * The destination of an edge is always an hyperedge vertex.
 */
struct Edge {
  HypernodeID src;
  HyperedgeID dest;

  Edge(const HypernodeID s, const HyperedgeID d) :
    src(s),
    dest(d) { }
  friend bool operator== (const Edge& lhs, const Edge& rhs) {
    return lhs.src == rhs.src && lhs.dest == rhs.dest;
  }
};

using EdgeVector = std::vector<Edge>;

/*!
 * Serializes the hypergraph as an edge list of the underlying bipartite
 * graph. Given a hypergraph with n hypernodes and m hyperedges, the hypernode vertices
 * are numbered 0, ..., n-1, while hyperedge vertices are numbered n, ..., n+m-1.
 * The size of the edge list equals the number of pins in the hypergraph.
 * Serialization is intended to be used for community detection with the code
 * accompanying the article "Fast unfolding of community hierarchies in large networks"
 * by V. Blondel, J.-L. Guillaume, R. Lambiotte, E. Lefebvre.
 * The code can be found here: https://sites.google.com/site/findcommunities/
 */
static EdgeVector createEdgeVector(const Hypergraph& hypergraph) {
  EdgeVector edges;
  for (const HypernodeID& hn : hypergraph.nodes()) {
    // Hypernode-vertex IDs start with 0
    // Hyperedge-vertex IDs start with |V|
    for (const HyperedgeID& he : hypergraph.incidentEdges(hn)) {
      const HyperedgeID he_id = hypergraph.initialNumNodes() + he;
      edges.emplace_back(hn, he_id);
    }
  }
  return edges;
}

// ! \}
