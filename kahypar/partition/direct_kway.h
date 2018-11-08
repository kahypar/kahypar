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
static constexpr bool debug = false;

static inline bool partitionVCycle(Hypergraph& hypergraph, ICoarsener& coarsener,
                                   IRefiner& refiner, const Context& context) {
  // In order to perform parallel net detection, we have to reset the edge hashes
  // before coarsening.
  hypergraph.resetEdgeHashes();

  io::printVcycleBanner(context);
  io::printCoarseningBanner(context);

  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(context.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  Timer::instance().add(context, Timepoint::v_cycle_coarsening,
                        std::chrono::duration<double>(end - start).count());

  if (context.partition.verbose_output && context.type == ContextType::main) {
    io::printHypergraphInfo(hypergraph, "Coarsened Hypergraph");
  }

  hypergraph.initializeNumCutHyperedges();

  io::printLocalSearchBanner(context);

  start = std::chrono::high_resolution_clock::now();
  const bool improved_quality = coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  Timer::instance().add(context, Timepoint::v_cycle_local_search,
                        std::chrono::duration<double>(end - start).count());

  io::printLocalSearchResults(context, hypergraph);
  return improved_quality;
}


static inline void partition(Hypergraph& hypergraph, const Context& context) {
  std::unique_ptr<ICoarsener> coarsener(
    CoarsenerFactory::getInstance().createObject(
      context.coarsening.algorithm, hypergraph, context,
      hypergraph.weightOfHeaviestNode()));

  std::unique_ptr<IRefiner> refiner(
    RefinerFactory::getInstance().createObject(
      context.local_search.algorithm, hypergraph, context));

  if (!context.partition.vcycle_refinement_for_input_partition) {
    multilevel::partition(hypergraph, *coarsener, *refiner, context);
  }

#ifdef KAHYPAR_USE_ASSERTIONS
  HyperedgeWeight initial_cut = std::numeric_limits<HyperedgeWeight>::max();
  HyperedgeWeight initial_km1 = std::numeric_limits<HyperedgeWeight>::max();
#endif

  for (uint32_t vcycle = 1; vcycle <= context.partition.global_search_iterations; ++vcycle) {
    context.partition.current_v_cycle = vcycle;
    const bool improved_quality = partitionVCycle(hypergraph, *coarsener, *refiner, context);

    if (!improved_quality) {
      LOG << "No improvement in V-cycle" << vcycle << ". Stopping global search.";
      break;
    }

    ASSERT(metrics::hyperedgeCut(hypergraph) <= initial_cut,
           metrics::hyperedgeCut(hypergraph) << ">" << initial_cut);
    ASSERT(metrics::km1(hypergraph) <= initial_km1,
           metrics::km1(hypergraph) << ">" << initial_km1);
#ifdef KAHYPAR_USE_ASSERTIONS
    initial_cut = metrics::hyperedgeCut(hypergraph);
    initial_cut = metrics::km1(hypergraph);
#endif
  }
}
}  // namespace direct_kway
}  // namespace kahypar
