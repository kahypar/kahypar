/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014-2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include <memory>
#include <string>

#include "kahypar/application/command_line_options.h"
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

using kahypar::HighResClockTimepoint;
using kahypar::Partitioner;
using kahypar::partition::EvoPartitioner;
using kahypar::Context;
using kahypar::PartitionID;
using kahypar::HyperedgeWeight;
using kahypar::HypernodeWeight;

int main(int argc, char* argv[]) {
  Context context;

  kahypar::processCommandLineInput(context, argc, argv);
  kahypar::sanityCheck(context);

  if (!context.partition.quiet_mode) {
    kahypar::io::printBanner();
  }

  if (context.partition.global_search_iterations != 0 &&
      context.partition.mode == kahypar::Mode::recursive_bisection) {
    std::cerr << "V-Cycles are not supported in recursive bisection mode." << std::endl;
    std::exit(-1);
  }

  kahypar::Randomize::instance().setSeed(context.partition.seed);

  kahypar::Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));

  if (!context.partition.fixed_vertex_filename.empty()) {
    kahypar::io::readFixedVertexFile(hypergraph, context.partition.fixed_vertex_filename);
  }

  if (context.partition.use_individual_part_weights) {
    if (context.partition.max_part_weights.size() != static_cast<size_t>(context.partition.k)) {
      LOG << "k=" << context.partition.k << ",but # part weights ="
          << context.partition.max_part_weights.size();
      std::exit(-1);
    }

    HypernodeWeight sum_part_weights = 0;
    for (const HypernodeWeight& part_weight : context.partition.max_part_weights) {
      sum_part_weights += part_weight;
    }
    if (sum_part_weights < hypergraph.totalWeight()) {
      LOG << "Sum of individual part weights is less than sum of vertex weights";
      std::exit(-1);
    }
  }

  size_t iteration = 0;
  std::chrono::duration<double> elapsed_time(0);

  const HighResClockTimepoint complete_start = std::chrono::high_resolution_clock::now();

  if (context.partition.time_limit != 0 && !context.partition_evolutionary) {
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

      if (!context.partition.quiet_mode) {
        kahypar::io::printPartitioningResults(hypergraph, context, elapsed_time);
        LOG << "";
      }
      if (context.partition.sp_process_output) {
        kahypar::io::serializer::serialize(context, hypergraph, elapsed_time, iteration);
      }
      hypergraph.reset();
      ++iteration;
    }

    for (const auto& hn : hypergraph.nodes()) {
      hypergraph.setNodePart(hn, best_solution[hn]);
    }
  } else if (context.partition_evolutionary && context.partition.time_limit != 0) {
    EvoPartitioner partitioner(context);
    partitioner.partition(hypergraph, context);

    std::vector<PartitionID> best_partition = partitioner.bestPartition();
    hypergraph.reset();

    for (const auto& hn : hypergraph.nodes()) {
      hypergraph.setNodePart(hn, best_partition[hn]);
    }
  } else {
    Partitioner().partition(hypergraph, context);
  }

  const HighResClockTimepoint complete_end = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double> elapsed_seconds = complete_end - complete_start;

#ifdef GATHER_STATS
  LOG << "*******************************";
  LOG << "***** GATHER_STATS ACTIVE *****";
  LOG << "*******************************";
  kahypar::io::printPartitioningStatistics();
#endif

  if (!context.partition.quiet_mode) {
    if (context.partition.time_limit != 0) {
      LOG << "********************************************************************************";
      LOG << "*                          FINAL Partitioning Result                           *";
      LOG << "********************************************************************************";
    }
    kahypar::io::printPartitioningResults(hypergraph, context, elapsed_seconds);
    LOG << "";
  }
  kahypar::io::writePartitionFile(hypergraph,
                                  context.partition.graph_partition_filename);

  // In case a time limit is used, the last partitioning step is already serialized
  if (context.partition.sp_process_output && context.partition.time_limit == 0) {
    kahypar::io::serializer::serialize(context, hypergraph, elapsed_seconds, iteration);
  }

  return 0;
}
