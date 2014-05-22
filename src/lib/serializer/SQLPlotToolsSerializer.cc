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
void SQLPlotToolsSerializer::serialize(const Configuration& config, const HypergraphType& hypergraph,
                                       const ICoarsener& UNUSED(coarsener), const IRefiner& refiner,
                                       const std::chrono::duration<double>& elapsed_seconds,
                                       const std::string& filename) {
  HypernodeWeight partition_weights[2] = { 0, 0 };
  metrics::partitionWeights(hypergraph, partition_weights);

  std::ofstream out_stream(filename.c_str(), std::ofstream::app);
  ip::file_lock f_lock(filename.c_str());
  {
    ip::scoped_lock<ip::file_lock> s_lock(f_lock);
    out_stream << "RESULT"
    << " graph=" << config.partitioning.graph_filename.substr(
      config.partitioning.graph_filename.find_last_of("/") + 1)
    << " numHNs=" << hypergraph.initialNumNodes()
    << " numHEs=" << hypergraph.initialNumEdges()
    << " k=" << config.partitioning.k
    << " epsilon=" << config.partitioning.epsilon
    << " L_max=" << config.partitioning.partition_size_upper_bound
    << " seed=" << config.partitioning.seed
    << " numInitialPartitionings=" << config.partitioning.initial_partitioning_attempts
    << " numVCycles=" << config.partitioning.global_search_iterations
    << " HESizeThreshold=" << config.partitioning.hyperedge_size_threshold
    << " coarseningScheme=";
    switch (config.coarsening.scheme) {
      case CoarseningScheme::HEAVY_EDGE_FULL:
      out_stream << "heavy_full";
      out_stream << " coarseningRating=heavy_edge";
      break;
    case CoarseningScheme::HEAVY_EDGE_HEURISTIC:
      out_stream << "heavy_heuristic";
      out_stream << " coarseningRating=heavy_edge";
      break;
    case CoarseningScheme::HYPEREDGE:
      out_stream << "hyperedge";
      out_stream << " coarseningRating=rater1";
      break;

    }
    out_stream << " coarseningNodeWeightFraction=" << config.coarsening.hypernode_weight_fraction
    << " coarseningNodeWeightThreshold=" << config.coarsening.threshold_node_weight
    << " coarseningMinNodeCount=" << config.coarsening.minimal_node_count
    << " twowayFMactive=" << config.two_way_fm.active
    << " twowayFMStoppingRule=" << (config.two_way_fm.stopping_rule == StoppingRule::SIMPLE ?
                                    "simple" : (config.two_way_fm.stopping_rule == StoppingRule::ADAPTIVE1 ?
                                                "adaptive1" : "adaptive2"))
    << " twowayFMNumRepetitions=" << config.two_way_fm.num_repetitions
    << " twowayFMFruitlessMoves=" << config.two_way_fm.max_number_of_fruitless_moves
    << " twowayFMalpha=" << config.two_way_fm.alpha
    << " twowayFMbeta=" << config.two_way_fm.beta
    << " herFMactive=" << config.her_fm.active
    << " herFMStoppingRule=" << (config.her_fm.stopping_rule == StoppingRule::SIMPLE ?
                                 "simple" : (config.her_fm.stopping_rule == StoppingRule::ADAPTIVE1 ?
                                             "adaptive1" : "adaptive2"))
    << " herFMFruitlessMoves=" << config.her_fm.max_number_of_fruitless_moves
    << refiner.policyString()
    << " cut=" << metrics::hyperedgeCut(hypergraph)
    << " part0=" << partition_weights[0]
    << " part1=" << partition_weights[1]
    << " imbalance=" << metrics::imbalance(hypergraph)
    << " time=" << elapsed_seconds.count()
    << " git=" << STR(KaHyPar_BUILD_VERSION)
    << std::endl;
    out_stream.flush();
  }
}
} // namespace serializer
