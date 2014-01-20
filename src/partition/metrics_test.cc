#include <gmock/gmock.h>

#include "../lib/datastructure/Hypergraph.h"
#include "Coarsener.h"
#include "Configuration.h"
#include "Partitioner.h"
#include "Metrics.h"
#include "Rater.h"

namespace metrics {
using ::testing::Test;
using ::testing::Eq;
using ::testing::DoubleEq;

using datastructure::HypergraphType;
using datastructure::HypernodeID;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;
using datastructure::HyperedgeWeight;

using partition::Rater;
using partition::FirstRatingWins;
using partition::Coarsener;
using partition::Configuration;
using partition::Partitioner;

typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef Coarsener<HypergraphType, FirstWinsRater> FirstWinsCoarsener;
typedef Configuration<HypergraphType> PartitionConfig;
typedef Partitioner<HypergraphType, FirstWinsCoarsener> HypergraphPartitioner;

class AnUnPartitionedHypergraph : public Test {
 public:
  AnUnPartitionedHypergraph() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}) {}
  
  HypergraphType hypergraph;
};

class TheDemoHypergraph : public AnUnPartitionedHypergraph {
 public:
  TheDemoHypergraph() : AnUnPartitionedHypergraph() {}
};

class APartitionedHypergraph : public Test {
 public:
  APartitionedHypergraph() :
      hypergraph(7, 4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      config(),
      partitioner(config) {
    config.coarsening.minimal_node_count = 2;
    config.coarsening.threshold_node_weight = 5;
    config.partitioning.graph_filename = "Test";
    config.partitioning.coarse_graph_filename = "coarse_test.hgr";
    config.partitioning.partition_filename = "coarse_test.hgr.part.2";
    config.partitioning.balance_constraint = 0.15;
    partitioner.partition(hypergraph);
  }

  HypergraphType hypergraph;
  PartitionConfig config;
  HypergraphPartitioner partitioner;
};

class TheHyperedgeCutCalculationForInitialPartitioning : public AnUnPartitionedHypergraph {
 public:
  TheHyperedgeCutCalculationForInitialPartitioning() :
      AnUnPartitionedHypergraph(),
      config(),
      coarsener(hypergraph, config),
      hg_to_hmetis(),
      partition() {
    config.coarsening.minimal_node_count = 2;
    config.coarsening.threshold_node_weight = 5;
    config.partitioning.graph_filename = "Test";
    config.partitioning.coarse_graph_filename = "coarse_cutCalc_test.hgr";
    config.partitioning.partition_filename = "coarse_cutCalc_test.hgr.part.2";
    config.partitioning.balance_constraint = 0.15;
    hg_to_hmetis[1] = 0;
    hg_to_hmetis[3] = 1;
    partition.push_back(1);
    partition.push_back(0);
  }
  PartitionConfig config;
  FirstWinsCoarsener coarsener;
  std::unordered_map<HypernodeID, HypernodeID> hg_to_hmetis;
  std::vector<PartitionID> partition;
};

TEST_F(TheHyperedgeCutCalculationForInitialPartitioning, ReturnsCorrectResult) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.nodeDegree(1), Eq(1));
  ASSERT_THAT(hypergraph.nodeDegree(3), Eq(1));
  hypergraph.changeNodePartition(3,0,1);
  
  ASSERT_THAT(hyperedgeCut(hypergraph, hg_to_hmetis, partition), Eq(hyperedgeCut(hypergraph)));
}

TEST_F(AnUnPartitionedHypergraph, HasHyperedgeCutZero) {
  ASSERT_THAT(hyperedgeCut(hypergraph), Eq(0));
}

TEST_F(APartitionedHypergraph, HasCorrectHyperedgeCut) {
  ASSERT_THAT(hyperedgeCut(hypergraph), Eq(2));
}

TEST_F(TheDemoHypergraph, HasAvgHyperedgeDegree3) {
  ASSERT_THAT(avgHyperedgeDegree(hypergraph), DoubleEq(3.0));
}

TEST_F(TheDemoHypergraph, HasAvgHypernodeDegree12Div7) {
  ASSERT_THAT(avgHypernodeDegree(hypergraph), DoubleEq(12.0 / 7));
}

} // namespace metrics
