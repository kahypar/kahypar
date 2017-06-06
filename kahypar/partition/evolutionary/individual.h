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

#include <limits>
#include <vector>

#include "kahypar/definitions.h"

namespace kahypar {
class Individual {
 private:
  static constexpr bool debug = true;

 public:
  Individual() :
    _partition(),
    _cut_edges(),
    _strong_cut_edges(),
    _fitness() { }

  Individual(std::vector<PartitionID>&& partition, std::vector<HyperedgeID>&& cut_edges,
             std::vector<HyperedgeID>&& strong_edges, HyperedgeWeight fitness) :
    _partition(std::move(partition)),
    _cut_edges(std::move(cut_edges)),
    _strong_cut_edges(std::move(strong_edges)),
    _fitness(fitness) { }

  explicit Individual(const std::vector<PartitionID>& partition) :
    _partition(std::move(partition)),
    _cut_edges(),
    _strong_cut_edges(),
    _fitness(std::numeric_limits<HyperedgeWeight>::max()) { }

  explicit Individual(const Hypergraph& hypergraph) :
    _partition(),
    _cut_edges(),
    _strong_cut_edges(),
    _fitness() {
    for (const HypernodeID& hn : hypergraph.nodes()) {
      _partition.push_back(hypergraph.partID(hn));
    }
    _fitness = metrics::km1(hypergraph);

    for (const HyperedgeID& he : hypergraph.edges()) {
      if (hypergraph.connectivity(he) > 1) {
        _cut_edges.push_back(he);
        // TODO(robin): why did this loop start from 1?
        for (PartitionID i = 0; i < hypergraph.connectivity(he); ++i) {
          _strong_cut_edges.push_back(he);
        }
      }
    }
    DBG << "New individual" << V(_fitness);
  }

  Individual(const Individual&) = delete;
  Individual& operator= (const Individual&) = delete;

  Individual(Individual&&) = default;
  Individual& operator= (Individual&&) = default;

  inline HyperedgeWeight fitness() const {
    ASSERT(_fitness != std::numeric_limits<HyperedgeWeight>::max());
    return _fitness;
  }


  inline const std::vector<PartitionID> & partition() const {
    ASSERT(!_partition.empty());
    return _partition;
  }
  inline const std::vector<HyperedgeID> & cutEdges() const {
    ASSERT(!_cut_edges.empty());
    return _cut_edges;
  }

  inline const std::vector<HyperedgeID> & strongCutEdges() const {
    ASSERT(!_strong_cut_edges.empty());
    return _strong_cut_edges;
  }
  inline void print() const {
    LOG << "Fitness:" << _fitness;
  }
  inline void printDebug() const {
    LOG << "Fitness:" << _fitness;
    LOG << "Partition :---------------------------------------";
    for (const PartitionID part : _partition) {
      LLOG << part;
    }
    LOG << "\n--------------------------------------------------";
    LOG << "Cut Edges :---------------------------------------";
    for (const HyperedgeID cut_edge : _cut_edges) {
      LLOG << cut_edge;
    }
    LOG << "\n--------------------------------------------------";
    LOG << "Strong Cut Edges :--------------------------------";
    for (const HyperedgeID strong_cut_edge :  _strong_cut_edges) {
      LLOG << strong_cut_edge;
    }
    LOG << "\n--------------------------------------------------";
  }

 private:
  std::vector<PartitionID> _partition;
  std::vector<HyperedgeID> _cut_edges;
  std::vector<HyperedgeID> _strong_cut_edges;
  HyperedgeWeight _fitness;
};

using Individuals = std::vector<std::reference_wrapper<const Individual> >;
}  // namespace kahypar
