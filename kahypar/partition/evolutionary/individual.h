/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2017 Robin Andre <robinandre1995@web.de>
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
#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/partition/metrics.h"

namespace kahypar {
class Individual {
 private:
  static constexpr bool debug = false;

 public:
  Individual() :
    _partition(),
    _cut_edges(),
    _strong_cut_edges(),
    _fitness() { }

  explicit Individual(const HyperedgeWeight fitness) :
    _partition(),
    _cut_edges(),
    _strong_cut_edges(),
    _fitness(fitness) { }

  explicit Individual(const std::vector<PartitionID>& partition) :
    _partition(std::move(partition)),
    _cut_edges(),
    _strong_cut_edges(),
    _fitness(std::numeric_limits<HyperedgeWeight>::max()) { }

  explicit Individual(const Hypergraph& hypergraph, const Context& context) :
    _partition(),
    _cut_edges(),
    _strong_cut_edges(),
    _fitness() {
    for (const HypernodeID& hn : hypergraph.nodes()) {
      _partition.push_back(hypergraph.partID(hn));
    }

    _fitness = metrics::correctMetric(hypergraph, context);

    for (const HyperedgeID& he : hypergraph.edges()) {
      if (hypergraph.connectivity(he) > 1) {
        _cut_edges.push_back(he);
        // The general idea is to add the connectivity (#blocks - 1)
        // instead of the # of blocks (However there should not be that much of a difference)
        // Edit: For Test Purposes this loop starts with 1.
        for (PartitionID i = 1; i < hypergraph.connectivity(he); ++i) {
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
std::ostream& operator<< (std::ostream& os, const Individual& individual) {
  os << "Fitness: " << individual.fitness() << std::endl;
  os << "Partition:------------------------------------" << std::endl;
  for (size_t i = 0; i < individual.partition().size(); ++i) {
    os << individual.partition()[i] << " ";
  }
  return os;
}
using Individuals = std::vector<std::reference_wrapper<const Individual> >;
using Parents = std::pair<const Individual&, const Individual&>;
}  // namespace kahypar
