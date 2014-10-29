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
                                       const ICoarsener& coarsener, const IRefiner& refiner,
                                       const std::chrono::duration<double>& elapsed_seconds,
                                       const std::array<std::chrono::duration<double>, 3>& timings,
                                       const std::string& filename) {
  std::ofstream out_stream(filename.c_str(), std::ofstream::app);
  ip::file_lock f_lock(filename.c_str());
  {
    ip::scoped_lock<ip::file_lock> s_lock(f_lock);
    out_stream << "RESULT"
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
    << " numVCycles=" << config.partition.global_search_iterations
    << " HESizeThreshold=" << config.partition.hyperedge_size_threshold
    << " coarseningScheme=" << config.coarsening.scheme
    << coarsener.policyString()
    << " coarseningMaxAllowedWeightMultiplier=" << config.coarsening.max_allowed_weight_multiplier
    << " coarseningContractionLimitMultiplier=" << config.coarsening.contraction_limit_multiplier
    << " coarseningNodeWeightFraction=" << config.coarsening.hypernode_weight_fraction
    << " coarseningMaximumAllowedNodeWeight=" << config.coarsening.max_allowed_node_weight
    << " coarseningContractionLimit=" << config.coarsening.contraction_limit
    << coarsener.stats().toString();
    if (config.two_way_fm.active) {
      out_stream << " twowayFMactive=" << config.two_way_fm.active
      << " twowayFMNumRepetitions=" << config.two_way_fm.num_repetitions
      << " twowayFMFruitlessMoves=" << config.two_way_fm.max_number_of_fruitless_moves
      << " twowayFMalpha=" << config.two_way_fm.alpha
      << " twowayFMbeta=" << config.two_way_fm.beta;
    }
    if (config.her_fm.active) {
      out_stream << " herFMactive=" << config.her_fm.active
      << " herFMFruitlessMoves=" << config.her_fm.max_number_of_fruitless_moves;
    }
    out_stream << refiner.policyString()
    << refiner.stats().toString();
    for (PartitionID i = 0; i != hypergraph.k(); ++i) {
      out_stream << " part" << i << "=" << hypergraph.partWeight(i);
    }
    out_stream << " cut=" << metrics::hyperedgeCut(hypergraph)
    << " imbalance=" << metrics::imbalance(hypergraph)
    << " totalPartitionTime=" << elapsed_seconds.count()
    << " coarseningTime=" << timings[0].count()
    << " initialPartitionTime=" << timings[1].count()
    << " uncoarseningRefinementTime=" << timings[2].count()
    << " git=" << STR(KaHyPar_BUILD_VERSION)
    << std::endl;
    out_stream.flush();
  }
}
} // namespace serializer
