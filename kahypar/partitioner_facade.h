/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2018 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/io/partitioning_output.h"
#include "kahypar/io/sql_plottools_serializer.h"
#include "kahypar/kahypar.h"
#include "kahypar/macros.h"
#include "kahypar/partition/evo_partitioner.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/utils/math.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {
class PartitionerFacade {
 public:
  void partition(Hypergraph& hypergraph, Context& context) {
    io::printBanner(context);

    sanityCheck(hypergraph, context);

    Randomize::instance().setSeed(context.partition.seed);

    if (!context.partition.fixed_vertex_filename.empty()) {
      io::readFixedVertexFile(hypergraph, context.partition.fixed_vertex_filename);
    }

    if (!context.partition.input_partition_filename.empty()) {
      setupVcycleRefinement(hypergraph, context);
    }

    const auto time_and_iteration = performPartitioning(hypergraph, context);
    const std::chrono::duration<double> elapsed_seconds = time_and_iteration.first;
    const size_t iteration = time_and_iteration.second;

#ifdef GATHER_STATS
    io::printStatsBanner();
    io::printPartitioningStatistics();
#endif


    io::printFinalPartitioningResults(hypergraph, context, elapsed_seconds);
    if (context.partition.write_partition_file){
      io::writePartitionFile(hypergraph, context.partition.graph_partition_filename);
    }

    // In case a time limit is used, the last partitioning step is already serialized
    if (context.partition.sp_process_output && context.partition.time_limit == 0) {
      io::serializer::serialize(context, hypergraph, elapsed_seconds, iteration);
    }
  }

 private:
  void setupVcycleRefinement(Hypergraph& hypergraph, Context& context) {
    // We perform direct k-way V-cycle refinements.
    context.partition.vcycle_refinement_for_input_partition = true;

    std::vector<PartitionID> input_partition;
    io::readPartitionFile(context.partition.input_partition_filename, input_partition);
    ASSERT(*std::max_element(input_partition.begin(), input_partition.end()) ==
           context.partition.k - 1);
    ASSERT(input_partition.size() == hypergraph.initialNumNodes());
    ASSERT([&]() {
        std::unordered_set<PartitionID> set;
        for (const PartitionID part : input_partition) {
          set.insert(part);
        }
        for (PartitionID i = 0; i < context.partition.k; ++i) {
          if (set.find(i) == set.end()) {
            return false;
          }
        }
        return true;
      } (), "Partition file is corrupted.");

    for (kahypar::HypernodeID hn = 0; hn != hypergraph.initialNumNodes(); ++hn) {
      hypergraph.setNodePart(hn, input_partition[hn]);
    }

    // Preconditions for direct k-way V-cycle refinement:
    if (context.partition.mode != kahypar::Mode::direct_kway) {
      LOG << "V-cycle refinement of input partitions is only possible in direct k-way mode";
      std::exit(0);
    }
    if (context.preprocessing.enable_min_hash_sparsifier == true) {
      LOG << "Disabling sparsifier for refinement of input partitions.";
      context.preprocessing.enable_min_hash_sparsifier = false;
    }
    if (context.partition.global_search_iterations == 0) {
      LOG << "V-cycle refinement of input partitions needs parameter --vcycles to be >= 1";
      std::exit(0);
    }
    context.setupPartWeights(hypergraph.totalWeight());
    io::printQualityOfInitialSolution(hypergraph, context);
  }

  size_t performTimeLimitedRepeatedPartitioning(Hypergraph& hypergraph, Context& context) {
    size_t iteration = 0;
    std::chrono::duration<double> elapsed_time(0);

    // We are running in time limit mode. Therefore we have to remember the best solution
    std::vector<PartitionID> best_solution(hypergraph.initialNumNodes(), 0);
    HyperedgeWeight best_solution_quality = std::numeric_limits<HyperedgeWeight>::max();
    double best_imbalance = 1.0;

    Partitioner partitioner;
    while (elapsed_time.count() < context.partition.time_limit) {
      const HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
      partitioner.partition(hypergraph, context);
      const HighResClockTimepoint end = std::chrono::high_resolution_clock::now();

      elapsed_time += std::chrono::duration<double>(end - start);

      const HyperedgeWeight current_solution_quality =
        kahypar::metrics::correctMetric(hypergraph, context);
      const double current_imbalance = kahypar::metrics::imbalance(hypergraph, context);

      const bool improved_quality = current_solution_quality < best_solution_quality;
      const bool improved_imbalance = (current_solution_quality == best_solution_quality) &&
                                      (current_imbalance < best_imbalance);

      if (improved_quality || improved_imbalance) {
        best_solution_quality = current_solution_quality;
        best_imbalance = current_imbalance;
        for (const auto& hn : hypergraph.nodes()) {
          best_solution[hn] = hypergraph.partID(hn);
        }
      }

      io::printPartitioningResults(hypergraph, context, elapsed_time);
      io::serializer::serialize(context, hypergraph, elapsed_time, iteration);

      hypergraph.reset();
      ++iteration;
    }
    for (const auto& hn : hypergraph.nodes()) {
      hypergraph.setNodePart(hn, best_solution[hn]);
    }
    return iteration;
  }

  void performEvolutionaryPartitioning(Hypergraph& hypergraph, Context& context) {
    EvoPartitioner evo_partitioner(context);
    evo_partitioner.partition(hypergraph, context);
    const std::vector<PartitionID>& best_partition = evo_partitioner.bestPartition();

    hypergraph.reset();
    for (const auto& hn : hypergraph.nodes()) {
      hypergraph.setNodePart(hn, best_partition[hn]);
    }
  }

  std::pair<std::chrono::duration<double>, size_t> performPartitioning(Hypergraph& hypergraph,
                                                                       Context& context) {
    size_t iteration = 0;
    const HighResClockTimepoint complete_start = std::chrono::high_resolution_clock::now();
    if (context.partition.time_limit != 0 && !context.partition_evolutionary) {
      iteration = performTimeLimitedRepeatedPartitioning(hypergraph, context);
    } else if (context.partition_evolutionary && context.partition.time_limit != 0) {
      performEvolutionaryPartitioning(hypergraph, context);
    } else {
      Partitioner().partition(hypergraph, context);
    }
    const HighResClockTimepoint complete_end = std::chrono::high_resolution_clock::now();
    return { complete_end - complete_start, iteration };
  }
};
}  // namespace kahypar
