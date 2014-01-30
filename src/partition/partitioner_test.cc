#include "gmock/gmock.h"

#include "lib/macros.h"
#include "lib/datastructure/Hypergraph.h"
#include "partition/Configuration.h"
#include "partition/Partitioner.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/Coarsener.h"

namespace partition {
using ::testing::Test;
using ::testing::Eq;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;

typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef Coarsener<HypergraphType, FirstWinsRater> FirstWinsCoarsener;
typedef Configuration<HypergraphType> PartitionConfig;
typedef Partitioner<HypergraphType> HypergraphPartitioner;

class APartitioner : public Test {
 public:
  APartitioner() :
      hypergraph(7, 4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}),
      config(),
      partitioner(config),
      coarsener(new FirstWinsCoarsener(hypergraph, config)){
    config.coarsening.minimal_node_count = 2;
    config.coarsening.threshold_node_weight = 5;
    config.partitioning.graph_filename = "PartitionerTest.hgr";
    config.partitioning.graph_partition_filename = "PartitionerTest.hgr.part.2.KaHyPar";
    config.partitioning.coarse_graph_filename = "PartitionerTest_coarse.hgr";
    config.partitioning.coarse_graph_partition_filename = "PartitionerTest_coarse.hgr.part.2";
    config.partitioning.epsilon = 0.15;
  }
  
  HypergraphType hypergraph;
  PartitionConfig config;
  HypergraphPartitioner partitioner;
  std::unique_ptr<ICoarsener<HypergraphType> > coarsener;
};

TEST_F(APartitioner, UseshMetisPartitioningOnCoarsestHypergraph) {      
  partitioner.partition(hypergraph, coarsener);
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(1));
  ASSERT_THAT(hypergraph.partitionIndex(3), Eq(0));
}

TEST_F(APartitioner, UncoarsensTheInitiallyPartitionedHypergraph) {
  partitioner.partition(hypergraph, coarsener);
  ASSERT_THAT(hypergraph.partitionIndex(0), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(1), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(2), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(3), Eq(1));
  ASSERT_THAT(hypergraph.partitionIndex(4), Eq(1));
  ASSERT_THAT(hypergraph.partitionIndex(5), Eq(0));
  ASSERT_THAT(hypergraph.partitionIndex(6), Eq(1));
}

TEST_F(APartitioner, CalculatesPinCountsOfAHyperedgesAfterInitialPartitioning) {
  ASSERT_THAT(hypergraph.pinCountInPartition(0,0), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPartition(0,1), Eq(0));
  ASSERT_THAT(hypergraph.pinCountInPartition(2,0), Eq(3));
  ASSERT_THAT(hypergraph.pinCountInPartition(2,1), Eq(0));
  partitioner.partition(hypergraph, coarsener);
  ASSERT_THAT(hypergraph.pinCountInPartition(0,0), Eq(2));
  ASSERT_THAT(hypergraph.pinCountInPartition(0,1), Eq(0));
  ASSERT_THAT(hypergraph.pinCountInPartition(2,0), Eq(0));
  ASSERT_THAT(hypergraph.pinCountInPartition(2,1), Eq(3));
}

} // namespace partition
