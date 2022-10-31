/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2022 Tobias Heuer <tobias.heuer@kit.edu>
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
#include "kahypar/partition/context.h"

namespace kahypar {

class LargeHyperedgeRemover {


 public:
  LargeHyperedgeRemover() :
    _removed_hes() { }

  LargeHyperedgeRemover(const LargeHyperedgeRemover&) = delete;
  LargeHyperedgeRemover & operator= (const LargeHyperedgeRemover &) = delete;

  LargeHyperedgeRemover(LargeHyperedgeRemover&&) = delete;
  LargeHyperedgeRemover & operator= (LargeHyperedgeRemover &&) = delete;

  // ! Removes large hyperedges from the hypergraph
  // ! Returns the number of removed large hyperedges.
  HypernodeID removeLargeHyperedges(Hypergraph& hypergraph, const Context& context) {
    _removed_hes.clear();
    HypernodeID num_removed_large_hyperedges = 0;
    for ( const HyperedgeID& he : hypergraph.edges() ) {
      if ( hypergraph.edgeSize(he) > context.partition.max_he_size_threshold ) {
        hypergraph.removeEdge(he);
        _removed_hes.push_back(he);
        ++num_removed_large_hyperedges;
      }
    }
    return num_removed_large_hyperedges;
  }

  // ! Restores all previously removed large hyperedges
  void restoreLargeHyperedges(Hypergraph& hypergraph, const Context& context) {
    HyperedgeWeight delta = 0;
    for ( const HyperedgeID& he : _removed_hes ) {
      hypergraph.restoreEdge(he);
      if ( context.partition.objective == kahypar::Objective::cut ) {
         delta += (hypergraph.connectivity(he) > 1 ? hypergraph.edgeWeight(he) : 0);
       } else {
         delta += (hypergraph.connectivity(he) - 1) * hypergraph.edgeWeight(he);
       }
    }

    if ( context.partition.verbose_output && delta > 0 ) {
      LOG << RED << "Restoring of" << _removed_hes.size() << "large hyperedges (|e| >"
          << context.partition.max_he_size_threshold << ") increased" << context.partition.objective
          << "by" << delta << END;
    }
  }

 private:
  std::vector<HypernodeID> _removed_hes;
};

}  // namespace kahypar
