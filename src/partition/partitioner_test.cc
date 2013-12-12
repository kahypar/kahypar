#include "gmock/gmock.h"

#include "../lib/macros.h"
#include "../lib/datastructure/Hypergraph.h"
#include "Coarsener.h"
#include "Configuration.h"
#include "Partitioner.h"

using ::testing::Test;
using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;

namespace partition {

class APartitioner : public Test {
 public:
  APartitioner() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}) {}
  
  HypergraphType hypergraph;
};

TEST_F(APartitioner, TakesAHypergraphAsInput) {
  typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
  typedef Coarsener<HypergraphType, FirstWinsRater> FirstWinsCoarsener;
  typedef Configuration<HypergraphType> PartitionConfig;
  typedef Partitioner<HypergraphType, FirstWinsCoarsener> HypergraphPartitioner;
      
  PartitionConfig config(5,2);  
  HypergraphPartitioner partitioner(hypergraph,config);
  partitioner.partition();
}

} // namespace partition
