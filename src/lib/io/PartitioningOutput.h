#ifndef LIB_IO_PARTITIONINGOUTPUT_H_
#define LIB_IO_PARTITIONINGOUTPUT_H_

#include <chrono>
#include <iostream>
#include <string>

#include "partition/Configuration.h"
#include "partition/Metrics.h"
#include "lib/GitRevision.h"
#include "partition/Metrics.h"

using partition::CoarseningScheme;
using partition::StoppingRule;

namespace io {
template <class Hypergraph>
void printHypergraphInfo(const Hypergraph& hypergraph, const std::string& name) {
  std::cout << "***********************Hypergraph Information************************" << std::endl;
  std::cout << "Name : " << name << std::endl;
  std::cout << "# HEs: " << hypergraph.numEdges() << "\t [avg HE size  : "
            << metrics::avgHyperedgeDegree(hypergraph) << "]" << std::endl;
  std::cout << "# HNs: " << hypergraph.numNodes() << "\t [avg HN degree: "
            << metrics::avgHypernodeDegree(hypergraph) << "]" << std::endl;
}

template <class Configuration>
void printPartitionerConfiguration(const Configuration& config) {
  std::cout << "*********************Partitioning Configuration**********************" << std::endl;
  std::cout << toString(config) << std::endl;
}

template <class Hypergraph>
void printPartitioningResults(const Hypergraph& hypergraph,
                              const std::chrono::duration<double>& elapsed_seconds) {
  HypernodeWeight partition_weights[2] = { 0, 0 };
  metrics::partitionWeights(hypergraph, partition_weights);
  std::cout << "***********************2-way Partition Result************************" << std::endl;
  std::cout << "Hyperedge Cut   = " << metrics::hyperedgeCut(hypergraph) << std::endl;
  std::cout << "Imbalance       = " << metrics::imbalance(hypergraph) << std::endl;
  std::cout << "| partition 0 | = " << partition_weights[0] << std::endl;
  std::cout << "| partition 1 | = " << partition_weights[1] << std::endl;
  std::cout << "partition time  = " << elapsed_seconds.count() << " s" << std::endl;
}

template <class Configuration, class Hypergraph, class Refiner, class Coarsener>
void printResultString(const Configuration& config, const Hypergraph& hypergraph,
                       const Coarsener& coarsener, const Refiner& refiner,
                       const std::chrono::duration<double>& elapsed_seconds) {
  HypernodeWeight partition_weights[2] = { 0, 0 };
  metrics::partitionWeights(hypergraph, partition_weights);
  std::cout << "RESULT"
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
            << " coarseningScheme=" << (config.coarsening.scheme == CoarseningScheme::HEAVY_EDGE_FULL ?
                                        "heavy_full" : "heavy_heuristic")
            << " coarseningNodeWeightFraction=" << config.coarsening.hypernode_weight_fraction
            << " coarseningNodeWeightThreshold=" << config.coarsening.threshold_node_weight
            << " coarseningMinNodeCount=" << config.coarsening.minimal_node_count
            << " coarseningRating=" << "heavy_edge"
            << " twowayFMactive=" << config.two_way_fm.active
            << " twowayFMStoppingRule=" <<  (config.two_way_fm.stopping_rule == StoppingRule::SIMPLE ?
                               "simple" : (config.two_way_fm.stopping_rule == StoppingRule::ADAPTIVE1
                                           ? "adaptive1" : "adaptive2"))
            << " twowayFMNumRepetitions=" << config.two_way_fm.num_repetitions
            << " twowayFMFruitlessMoves=" << config.two_way_fm.max_number_of_fruitless_moves
            << " twowayFMalpha=" <<  config.two_way_fm.alpha
            << " twowayFMbeta=" << config.two_way_fm.beta
            << " herFMactive=" << config.her_fm.active
            << " herFMStoppingRule=" <<    (config.her_fm.stopping_rule == StoppingRule::SIMPLE ?
                                            "simple" : (config.her_fm.stopping_rule == StoppingRule::ADAPTIVE1
                                                        ? "adaptive1" : "adaptive2"))
            << " herFMFruitlessMoves=" << config.her_fm.max_number_of_fruitless_moves
            << refiner.policyString()
            << " cut=" << metrics::hyperedgeCut(hypergraph)
            << " part0=" << partition_weights[0]
            << " part1=" << partition_weights[1]
            << " imbalance=" << metrics::imbalance(hypergraph)
            << " time=" << elapsed_seconds.count()
            << " git=" << STR(KaHyPar_BUILD_VERSION)
            << std::endl;
}

} // namespace io
#endif  // LIB_IO_PARTITIONINGOUTPUT_H_
