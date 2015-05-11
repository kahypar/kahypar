/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "lib/serializer/SQLPlotToolsSerializer.h"

#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "lib/GitRevision.h"
#include "partition/Metrics.h"

namespace ip = boost::interprocess;

namespace serializer {
void SQLPlotToolsSerializer::serialize(const Configuration& config, const Hypergraph& hypergraph,
                                       const Partitioner& partitioner,
                                       const ICoarsener& coarsener, const IRefiner& refiner,
                                       const std::chrono::duration<double>& elapsed_seconds,
                                       const std::array<std::chrono::duration<double>, 7>& timings,
                                       const std::string& filename) {
  std::ostringstream oss;
  oss << "RESULT"
  << " graph=" << config.partition.graph_filename.substr(
    config.partition.graph_filename.find_last_of("/") + 1)
  << " numHNs=" << hypergraph.initialNumNodes()
  << " numHEs=" << hypergraph.initialNumEdges()
  << " k=" << config.partition.k
  << " epsilon=" << config.partition.epsilon
  << " totalGraphWeight=" << config.partition.total_graph_weight
  << " L_max=" << config.partition.max_part_weight
  << " seed=" << config.partition.seed
  << " hmetisUBFactor=" << config.partition.hmetis_ub_factor
  << " numInitialPartitions=" << config.partition.initial_partitioning_attempts
  << " initialPartitioner=" << (config.partition.initial_partitioner == InitialPartitioner::hMetis ? "hMetis" : "PaToH")
  << " initialPartitionerPath=" << config.partition.initial_partitioner_path
  << " numVCycles=" << config.partition.global_search_iterations
  << " HESizeThreshold=" << config.partition.hyperedge_size_threshold
  << " initiallyRemoveParallelHEs=" << std::boolalpha << config.partition.initial_parallel_he_removal
  << " coarseningScheme=" << config.coarsening.scheme
  << coarsener.policyString()
  << " coarseningMaxAllowedWeightMultiplier=" << config.coarsening.max_allowed_weight_multiplier
  << " coarseningContractionLimitMultiplier=" << config.coarsening.contraction_limit_multiplier
  << " coarseningNodeWeightFraction=" << config.coarsening.hypernode_weight_fraction
  << " coarseningMaximumAllowedNodeWeight=" << config.coarsening.max_allowed_node_weight
  << " coarseningContractionLimit=" << config.coarsening.contraction_limit
  << coarsener.stats().toString()
  << partitioner.stats().toString();
  if (config.fm_local_search.active) {
    oss << " FMactive=" << config.fm_local_search.active
    << " FMNumRepetitions=" << config.fm_local_search.num_repetitions
    << " FMFruitlessMoves=" << config.fm_local_search.max_number_of_fruitless_moves
    << " FMalpha=" << config.fm_local_search.alpha
    << " FMbeta=" << config.fm_local_search.beta;
  }
  if (config.her_fm.active) {
    oss << " herFMactive=" << config.her_fm.active
    << " herFMFruitlessMoves=" << config.her_fm.max_number_of_fruitless_moves;
  }
  oss << refiner.policyString()
  << refiner.stats().toString();
  for (PartitionID i = 0; i != hypergraph.k(); ++i) {
    oss << " part" << i << "=" << hypergraph.partWeight(i);
  }
  oss << " cut=" << metrics::hyperedgeCut(hypergraph)
  << " soed=" << metrics::soed(hypergraph)
  << " kMinusOne=" << metrics::kMinus1(hypergraph)
  << " absorption=" << metrics::absorption(hypergraph)
  << " imbalance=" << metrics::imbalance(hypergraph)
  << " totalPartitionTime=" << elapsed_seconds.count()
  << " initialParallelHEremovalTime=" << timings[0].count()
  << " initialLargeHEremovalTime=" << timings[1].count()
  << " coarseningTime=" << timings[2].count()
  << " initialPartitionTime=" << timings[3].count()
  << " uncoarseningRefinementTime=" << timings[4].count()
  << " initialParallelHErestoreTime=" << timings[5].count()
  << " initialLargeHErestoreTime=" << timings[6].count()
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
