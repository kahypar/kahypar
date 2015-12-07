/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "lib/serializer/SQLPlotToolsSerializer.h"

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "lib/GitRevision.h"
#include "partition/Metrics.h"

namespace ip = boost::interprocess;

using partition::toString;
using partition::RefinementAlgorithm;

namespace serializer {
void SQLPlotToolsSerializer::serialize(const Configuration& config, const Hypergraph& hypergraph,
                                       const Partitioner& partitioner,
                                       const std::chrono::duration<double>& elapsed_seconds,
                                       const std::string& filename) {
  std::ostringstream oss;
  oss << "RESULT"
  << " graph=" << config.partition.graph_filename.substr(
    config.partition.graph_filename.find_last_of("/") + 1)
  << " numHNs=" << hypergraph.initialNumNodes()
  << " numHEs=" << hypergraph.initialNumEdges()
  << " " << hypergraph.typeAsString()
  << " k=" << config.partition.k
  << " epsilon=" << config.partition.epsilon
  << " totalGraphWeight=" << config.partition.total_graph_weight
  << " L_opt0=" << config.partition.perfect_balance_part_weights[0]
  << " L_opt1=" << config.partition.perfect_balance_part_weights[1]
  << " L_max0=" << config.partition.max_part_weights[0]
  << " L_max1=" << config.partition.max_part_weights[1]
  << " seed=" << config.partition.seed
  << " hmetisUBFactor=" << config.partition.hmetis_ub_factor
  << " numInitialPartitions=" << config.initial_partitioning.nruns
  << " initialPartitioner=" << toString(config.partition.initial_partitioner)
  << " initialPartitioningMode=" << toString(config.initial_partitioning.mode)
  << " initialPartitioningTechnique=" << toString(config.initial_partitioning.technique)
  << " initialPartitioningAlgorithm=" << toString(config.initial_partitioning.algo)
  << " poolType=" << config.initial_partitioning.pool_type
  << " InitialFMNumRepetitions=" << config.initial_partitioning.local_search_repetitions
  << " initialPartitionerPath=" << config.partition.initial_partitioner_path
  << " numVCycles=" << config.partition.global_search_iterations
  << " HESizeThreshold=" << config.partition.hyperedge_size_threshold
  << " initiallyRemoveParallelHEs=" << std::boolalpha << config.partition.initial_parallel_he_removal
  << " removeHEsThatAlwaysWillBeCut=" << std::boolalpha << config.partition.remove_hes_that_always_will_be_cut
  << " mode=" << toString(config.partition.mode)
  << " coarseningAlgo=" << toString(config.partition.coarsening_algorithm)
  << " coarseningMaxAllowedWeightMultiplier=" << config.coarsening.max_allowed_weight_multiplier
  << " coarseningContractionLimitMultiplier=" << config.coarsening.contraction_limit_multiplier
  << " coarseningNodeWeightFraction=" << config.coarsening.hypernode_weight_fraction
  << " coarseningMaximumAllowedNodeWeight=" << config.coarsening.max_allowed_node_weight
  << " coarseningContractionLimit=" << config.coarsening.contraction_limit
  << Stats::instance().toString()
  << " refinementAlgo=" << toString(config.partition.refinement_algorithm);
  if (config.partition.refinement_algorithm == RefinementAlgorithm::twoway_fm ||
      config.partition.refinement_algorithm == RefinementAlgorithm::kway_fm) {
    oss << " FMNumRepetitions=" << config.fm_local_search.num_repetitions
    << " FMFruitlessMoves=" << config.fm_local_search.max_number_of_fruitless_moves
    << " FMalpha=" << config.fm_local_search.alpha
    << " FMbeta=" << config.fm_local_search.beta;
  }
  if (config.partition.refinement_algorithm == RefinementAlgorithm::hyperedge) {
    oss << " herFMFruitlessMoves=" << config.her_fm.max_number_of_fruitless_moves;
  }
  if (config.partition.refinement_algorithm == RefinementAlgorithm::label_propagation) {
    oss << " lpMaxNumIterations=" << config.lp_refiner.max_number_iterations;
  }
  oss << partitioner.internals();
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    oss << " partSize" << i << "=" << hypergraph.partSize(i);
  }
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    oss << " partWeight" << i << "=" << hypergraph.partWeight(i);
  }
  oss << " cut=" << metrics::hyperedgeCut(hypergraph)
  << " soed=" << metrics::soed(hypergraph)
  << " kMinusOne=" << metrics::kMinus1(hypergraph)
  << " absorption=" << metrics::absorption(hypergraph)
  << " imbalance=" << metrics::imbalance(hypergraph, config)
  << " totalPartitionTime=" << elapsed_seconds.count()
  << " initialParallelHEremovalTime=" << Stats::instance().get("InitialParallelHEremoval")
  << " initialLargeHEremovalTime=" << Stats::instance().get("InitialLargeHEremoval")
  << " coarseningTime=" << Stats::instance().get("Coarsening")
  << " initialPartitionTime=" << Stats::instance().get("InitialPartitioning")
  << " uncoarseningRefinementTime=" << Stats::instance().get("UncoarseningRefinement")
  << " initialParallelHErestoreTime=" << Stats::instance().get("InitialParallelHErestore")
  << " initialLargeHErestoreTime=" << Stats::instance().get("InitialLargeHErestore")
  << " git=" << STR(KaHyPar_BUILD_VERSION)
  << std::endl;
  if (!filename.empty()) {
    std::ofstream out_stream(filename.c_str(), std::ofstream::app);
    ip::file_lock f_lock(filename.c_str());
    {
      ip::scoped_lock<ip::file_lock> s_lock(f_lock);
      out_stream << oss.str();
      out_stream.flush();
    }
  } else {
    std::cout << oss.str() << std::endl;
  }
}
}  // namespace serializer
