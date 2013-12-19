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
    config.coarsening_limit = 2;
    config.threshold_node_weight = 5;
    config.graph_filename = "Test";
    config.coarse_graph_filename = "coarse_test.hgr";
    config.partition_filename = "coarse_test.hgr.part.2";
    partitioner.partition(hypergraph);
  }

  HypergraphType hypergraph;
  PartitionConfig config;
  HypergraphPartitioner partitioner;
};

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
