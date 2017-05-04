/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/definitions.h"
#include "kahypar/partition/coarsening/hypergraph_pruner.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/factories.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/multilevel.h"
#include "kahypar/partition/refinement/i_refiner.h"

namespace kahypar {
namespace direct_kway {
static inline bool partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener,
                                   IRefiner& refiner, const Context& context) {
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(context.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  context.stats.coarsening("Time") += std::chrono::duration<double>(end - start).count();

  hypergraph.initializeNumCutHyperedges();

  start = std::chrono::high_resolution_clock::now();
  const bool found_improved_cut = coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  context.stats.localSearch("Time") += std::chrono::duration<double>(end - start).count();
  return found_improved_cut;
}


static inline void partition(Hypergraph& hypergraph, const Context& context) {
  std::unique_ptr<ICoarsener> coarsener(
    CoarsenerFactory::getInstance().createObject(
      context.coarsening.algorithm, hypergraph, context,
      hypergraph.weightOfHeaviestNode()));

  std::unique_ptr<IRefiner> refiner(
    RefinerFactory::getInstance().createObject(
      context.local_search.algorithm, hypergraph, context));

  multilevel::partition(hypergraph, *coarsener, *refiner, context);

  DBG << "PartitioningResult: cut=" << metrics::hyperedgeCut(hypergraph);
#ifndef NDEBUG
  HyperedgeWeight initial_cut = std::numeric_limits<HyperedgeWeight>::max();
#endif

  for (int vcycle = 1; vcycle <= context.partition.global_search_iterations; ++vcycle) {
    const bool found_improved_cut = partitionVCycle(hypergraph, *coarsener, *refiner, context);

    DBG << V(vcycle) << V(metrics::hyperedgeCut(hypergraph));
    if (!found_improved_cut) {
      LOG << "Cut could not be decreased in v-cycle" << vcycle << ". Stopping global search.";
      break;
    }

    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut,
           "Uncoarsening worsened cut:" << metrics::hyperedgeCut(hypergraph) << ">" << initial_cut);
#ifndef NDEBUG
    initial_cut = metrics::hyperedgeCut(hypergraph);
#endif
  }
}
}  // namespace direct_kway
}  // namespace kahypar
