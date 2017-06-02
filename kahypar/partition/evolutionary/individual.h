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
class Individual {
 public:
  Individual() :
    _partition(),
    _cutedges(),
    _strongcutedges(),
    _fitness() { }
  Individual(std::vector<PartitionID> partition, std::vector<HyperedgeID> cut_edges, std::vector<HyperedgeID> strong_edges, HyperedgeWeight fitness) :
    _partition(partition),
    _cutedges(cut_edges),
    _strongcutedges(strong_edges),
    _fitness(fitness) { }

  explicit Individual(const Hypergraph& hypergraph) :
    _partition(),
    _cutedges(),
    _strongcutedges(),
    _fitness() {
    for (HypernodeID u : hypergraph.nodes()) {
      std::cout << hypergraph.partID(u) << " ";
      _partition.push_back(hypergraph.partID(u));
    }
    _fitness = metrics::km1(hypergraph);

    for (HyperedgeID v : hypergraph.edges()) {
      auto km1 = hypergraph.connectivity(v) - 1;
      if (km1 > 0) {
        _cutedges.push_back(v);
        for (unsigned i = 1; i < hypergraph.connectivity(v); i++) {
          _strongcutedges.push_back(v);
        }
      }
    }
  }
  inline HyperedgeWeight fitness() const {
    return _fitness;
  }


  inline const std::vector<PartitionID> & partition() const {
    return _partition;
  }
  inline const std::vector<HyperedgeID> & cutEdges() const {
    return _cutedges;
  }

  inline const std::vector<HyperedgeID> & strongCutEdges() const {
    return _strongcutedges;
  }
  inline void print() const {
    std::cout << "Fitness: " << _fitness << std::endl;
  }
  inline void printDebug() const {
    std::cout << "Fitness: " << _fitness << std::endl;
    std::cout << "Partition :---------------------------------------" << std::endl;
    for (std::size_t i = 0; i < _partition.size(); ++i) {
      std::cout << _partition[i] << " ";
    }
    std::cout << std::endl << "--------------------------------------------------" << std::endl;
    std::cout << "Cut Edges :---------------------------------------" << std::endl;
    for (std::size_t i = 0; i < _cutedges.size(); ++i) {
      std::cout << _cutedges[i] << " ";
    }
    std::cout << std::endl << "--------------------------------------------------" << std::endl;
    std::cout << "Strong Cut Edges :--------------------------------" << std::endl;
    for (std::size_t i = 0; i < _strongcutedges.size(); ++i) {
      std::cout << _strongcutedges[i] << " ";
    }
    std::cout << std::endl << "--------------------------------------------------" << std::endl;
  }

 private:
  std::vector<PartitionID> _partition;
  std::vector<HyperedgeID> _cutedges;
  std::vector<HyperedgeID> _strongcutedges;
  HyperedgeWeight _fitness;
};
}  // namespace kahypar
