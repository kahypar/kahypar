/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/kahypar.h"
#include "kahypar/partition/configuration.h"
#include "kahypar/utils/randomize.h"

namespace kahypar {


TEST(KaHyPar, ComputesDirectKwayCutPartitioning) {
  Configuration config;
  config.partition.k = 8;
  config.partition.epsilon = 0.03;
  config.partition.mode = Mode::direct_kway;
  config.partition.objective = Objective::cut;
  config.partition.seed = 2;
  config.coarsening.algorithm = CoarseningAlgorithm::ml_style;
  config.coarsening.max_allowed_weight_multiplier = 1;
  config.coarsening.contraction_limit_multiplier = 160;
  config.initial_partitioning.tool = InitialPartitioner::KaHyPar;
  config.initial_partitioning.mode = Mode::recursive_bisection;
  config.initial_partitioning.technique = InitialPartitioningTechnique::multilevel;
  config.initial_partitioning.coarsening.algorithm = CoarseningAlgorithm::ml_style;
  config.initial_partitioning.coarsening.max_allowed_weight_multiplier = 1;
  config.initial_partitioning.algo = InitialPartitionerAlgorithm::pool;
  config.initial_partitioning.nruns = 20;
  config.initial_partitioning.local_search.algorithm = RefinementAlgorithm::twoway_fm;
  config.initial_partitioning.local_search.iterations_per_level = std::numeric_limits<int>::max();
  config.local_search.algorithm = RefinementAlgorithm::kway_fm;
  config.local_search.iterations_per_level = std::numeric_limits<int>::max();
  config.local_search.fm.stopping_rule = RefinementStoppingRule::adaptive_opt;
  config.local_search.fm.adaptive_stopping_alpha = 1;

  config.partition.graph_filename = "test_instances/ISPD98_ibm01.hgr";

  kahypar::Randomize::instance().setSeed(config.partition.seed);

  Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(config.partition.graph_filename,
                                          config.partition.k));

  Partitioner partitioner;
  partitioner.partition(hypergraph, config);
  kahypar::io::printPartitioningResults(hypergraph, config, std::chrono::duration<double>(0.0));

  ASSERT_EQ(metrics::hyperedgeCut(hypergraph), 887);
  ASSERT_EQ(metrics::soed(hypergraph), 2134);
  ASSERT_EQ(metrics::km1(hypergraph), 1247);

}


} // namespace kahypar
