#include "gmock/gmock.h"

#include "../lib/macros.h"
#include "../lib/datastructure/Hypergraph.h"
#include "Coarsener.h"
#include "Configuration.h"
#include "Partitioner.h"

namespace partition {
using ::testing::Test;
using ::testing::Eq;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;

typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef Coarsener<HypergraphType, FirstWinsRater> FirstWinsCoarsener;
typedef Configuration<HypergraphType> PartitionConfig;
typedef Partitioner<HypergraphType, FirstWinsCoarsener> HypergraphPartitioner;

class APartitioner : public Test {
 public:
  APartitioner() :
      hypergraph(7, 4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      config(),
      partitioner(config) {
    config.coarsening_limit = 2;
    config.threshold_node_weight = 5;
    config.graph_filename = "Test";
    config.coarse_graph_filename = "coarse_test.hgr";
    config.partition_filename = "coarse_test.hgr.part.2";
  }
  
  HypergraphType hypergraph;
  PartitionConfig config;
  HypergraphPartitioner partitioner;
};

TEST_F(APartitioner, UseshMetisPartitioningOnCoarsestHypergraph) {      
  partitioner.partition(hypergraph);
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(3), Eq(1));
}

TEST_F(APartitioner, UncoarsensTheInitiallyPartitionedHypergraph) {
  partitioner.partition(hypergraph);
  ASSERT_THAT(hypergraph.partitionIndex(0), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(2), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(3), Eq(1));
  ASSERT_THAT(hypergraph.partitionIndex(4), Eq(1));
  ASSERT_THAT(hypergraph.partitionIndex(5), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(6), Eq(1));
}

} // namespace partition
