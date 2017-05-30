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
#include "kahypar/utils/timer.h"

namespace kahypar {
namespace multilevel {
static constexpr bool debug = false;

static inline void partition(Hypergraph& hypergraph,
                             ICoarsener& coarsener,
                             IRefiner& refiner,
                             const Context& context) {
  io::printCoarseningBanner(context);

  HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
  coarsener.coarsen(context.coarsening.contraction_limit);
  HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
  Timer::instance().add(context, Timepoint::coarsening,
                        std::chrono::duration<double>(end - start).count());

  if (context.partition.verbose_output && context.type == ContextType::main) {
    io::printHypergraphInfo(hypergraph, "Coarsened Hypergraph");
  }
  if(!context.partition_evolutionary || context.evo_flags.action.requires.initial_partitioning) {
  if (context.type == ContextType::main && (context.partition.verbose_output ||
                                            context.initial_partitioning.verbose_output)) {
    io::printInitialPartitioningBanner(context);
  }
  start = std::chrono::high_resolution_clock::now();
  initial::partition(hypergraph, context);
  end = std::chrono::high_resolution_clock::now();
  Timer::instance().add(context, Timepoint::initial_partitioning,
                        std::chrono::duration<double>(end - start).count());

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
     io::printLocalSearchBanner(context);
  }
  }

  if(context.partition_evolutionary && context.evo_flags.action.requires.evolutionary_parent_contraction) {
    ASSERT(!context.evo_flags.action.requires.initial_partitioning);
    //There is currently no reason why an evolutionary contraction should be used 
    //in conjunction with initial partitioning ... Yet
    hypergraph.setPartitionVector(context.evo_flags.parent1);
    HyperedgeWeight parentWeight1 = metrics::km1(hypergraph);
    HyperedgeWeight parentWeight2;
    
    if(!context.evo_flags.action.requires.invalidation_of_second_partition) {
      hypergraph.reset();
      hypergraph.setPartitionVector(context.evo_flags.parent2);
      parentWeight2 = metrics::km1(hypergraph);
    }

    if(parentWeight1 < parentWeight2) {
			hypergraph.reset();
			hypergraph.setPartitionVector(context.evo_flags.parent1);
		}
		else {
			//Just for correctness, because the Hypergraph has partition 2
			//which is already better
		}
  }

  else {
    // TODO(robin): remove
    std::cout << "skipping Initial Partitioning" << std::endl;
    ASSERT(context.evo_flags.parent1.size != 0);

  }
  
  std::vector<HyperedgeID> stable_net_before_uncoarsen;
  std::vector<HyperedgeID> stable_net_after_uncoarsen;
  if(context.partition_evolutionary && context.evo_flags.action.requires.vcycle_stable_net_collection) {
    for(HyperedgeID u : hypergraph.edges()) {
      if(hypergraph.connectivity(u) > 1) {
        stable_net_before_uncoarsen.push_back(u);
      }
    } 
  }
  
  
  start = std::chrono::high_resolution_clock::now();
  coarsener.uncoarsen(refiner);
  end = std::chrono::high_resolution_clock::now();

  if(context.partition_evolutionary &&context.evo_flags.action.requires.vcycle_stable_net_collection) {
    for(HyperedgeID u : hypergraph.edges()) {
      if(hypergraph.connectivity(u) > 1) {
        stable_net_after_uncoarsen.push_back(u);
      }
    }
    std::vector<HyperedgeID> in_cut_before_and_after;
    std::set_intersection(stable_net_before_uncoarsen.begin(),
                          stable_net_before_uncoarsen.end(),
                          stable_net_after_uncoarsen.begin(),
                          stable_net_after_uncoarsen.end(),
                          back_inserter(in_cut_before_and_after));
    context.evo_flags.stable_net_edges_final = in_cut_before_and_after;
  }

  Timer::instance().add(context, Timepoint::local_search,
                        std::chrono::duration<double>(end - start).count());

  io::printLocalSearchResults(context, hypergraph);
}
}  // namespace multilevel
}  // namespace kahypar
