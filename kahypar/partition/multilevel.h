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

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/initial_partition.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/i_refiner.h"

namespace kahypar {
namespace multilevel {
static inline void partition(Hypergraph& hypergraph,
                             ICoarsener& coarsener,
                             IRefiner& refiner,
                             const Context& context) {
  if (context.partition.verbose_output && context.type == ContextType::main) {
    LOG << "********************************************************************************";
    LOG << "*                                Coarsening...                                 *";
    LOG << "********************************************************************************";
  }
  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(context.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  context.stats.coarsening("Time") += std::chrono::duration<double>(end - start).count();
  if (context.isMainRecursiveBisection()) {
    context.stats.topLevel().coarsening("Time") +=
      std::chrono::duration<double>(end - start).count();
  }

  if (context.partition.verbose_output && context.type == ContextType::main) {
    io::printHypergraphInfo(hypergraph, "Coarsened Hypergraph");
  }
  if (context.type == ContextType::main && (context.partition.verbose_output ||
                                            context.initial_partitioning.verbose_output)) {
    LOG << "\n********************************************************************************";
    LOG << "*                           Initial Partitioning...                            *";
    LOG << "********************************************************************************";
  }
  start = std::chrono::high_resolution_clock::now();
  initial::partition(hypergraph, context);
  end = std::chrono::high_resolution_clock::now();
  context.stats.initialPartitioning("Time") += std::chrono::duration<double>(end - start).count();
  if (context.isMainRecursiveBisection()) {
    context.stats.topLevel().initialPartitioning("Time") +=
      std::chrono::duration<double>(end - start).count();
  }

  hypergraph.initializeNumCutHyperedges();
  if (context.partition.verbose_output && context.type == ContextType::main) {
    LOG << "Initial Partitioning Result:";
    LOG << "Initial" << toString(context.partition.objective) << "      ="
        << (context.partition.objective == Objective::cut ? metrics::hyperedgeCut(hypergraph) :
        metrics::km1(hypergraph));
    LOG << "Initial imbalance =" << metrics::imbalance(hypergraph, context);
    LOG << "Initial part sizes and weights:";
    io::printPartSizesAndWeights(hypergraph);
    LLOG << "Target weights:";
    if (context.partition.mode == Mode::direct_kway) {
      LLOG << "w(*) =" << context.partition.max_part_weights[0] << "\n";
    } else {
      LLOG << "(RB): w(0)=" << context.partition.max_part_weights[0]
           << "w(1)=" << context.partition.max_part_weights[1] << "\n";
    }
    LOG << "\n********************************************************************************";
    LOG << "*                               Local Search...                                *";
    LOG << "********************************************************************************";
  }
  start = std::chrono::high_resolution_clock::now();
  coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();
  context.stats.localSearch("Time") += std::chrono::duration<double>(end - start).count();
  if (context.isMainRecursiveBisection()) {
    context.stats.topLevel().localSearch("Time") +=
      std::chrono::duration<double>(end - start).count();
  }

  if (context.partition.verbose_output && context.type == ContextType::main) {
    LOG << "Local Search Result:";
    LOG << "Final" << toString(context.partition.objective) << "      ="
        << (context.partition.objective == Objective::cut ? metrics::hyperedgeCut(hypergraph) :
        metrics::km1(hypergraph));
    LOG << "Final imbalance =" << metrics::imbalance(hypergraph, context);
    LOG << "Final part sizes and weights:";
    io::printPartSizesAndWeights(hypergraph);
    LOG << "";
  }
}
}  // namespace multilevel
}  // namespace kahypar
