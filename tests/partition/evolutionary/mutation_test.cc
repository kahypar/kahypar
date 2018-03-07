#include <boost/program_options.hpp>

#include <limits>
#include <string>
#include "gmock/gmock.h"
#include "kahypar/application/command_line_options.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include <vector>

using ::testing::Eq;
using ::testing::Test;
namespace kahypar {

  class AMutation : public Test {
    public :
    AMutation() :
      context(),
    //hypergraph(6, 3, HyperedgeIndexVector { 0, 3, 6, /*sentinel*/ 8},
    //           HyperedgeVector {0,1,2,3,4,5,4,5}) {hypergraph.changeK(2); }
    hypergraph(6, 1, HyperedgeIndexVector {0, 6},
               HyperedgeVector {0,1,2,3,4,5}) {hypergraph.changeK(2); }
    Context context;
    
    Hypergraph hypergraph;
  };

  TEST_F(AMutation, IsPerformingVcyclesCorrectly) {
    parseIniToContext(context, "../../../../config/km1_direct_kway_alenex17.ini");
    context.partition.k = 2;
    context.partition.epsilon = 0.03;
    context.partition.objective = Objective::cut;
    context.partition.mode = Mode::direct_kway;
    context.local_search.algorithm = RefinementAlgorithm::kway_fm;
    context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
    context.evolutionary.mutate_strategy = EvoMutateStrategy::vcycle;
    hypergraph.reset();
    hypergraph.setNodePart(0,0);
    hypergraph.setNodePart(1,0);
    hypergraph.setNodePart(2,0);
    hypergraph.setNodePart(3,1);
    hypergraph.setNodePart(4,1);
    hypergraph.setNodePart(5,1);
    Individual ind1 = Individual(hypergraph, context);
    Individual ind2 = partition::mutate::vCycle(hypergraph, ind1, context);
    /* Due to the nature of the vcycle operator, no structural changes should be made since all 
    /  computated gains are 0 gains */
    ASSERT_EQ(ind2.partition().at(0), ind2.partition().at(2));
    ASSERT_EQ(ind2.partition().at(0), ind2.partition().at(1));
    ASSERT_EQ(ind2.partition().at(3), ind2.partition().at(4));
    ASSERT_EQ(ind2.partition().at(3), ind2.partition().at(5));    
  }
   TEST_F(AMutation, IsPerformingVcyclesNewIPCorrectly) {
    parseIniToContext(context, "../../../../config/km1_direct_kway_alenex17.ini");
    context.partition.k = 2;
    context.partition.epsilon = 0.03;
    context.partition.objective = Objective::cut;
    context.partition.mode = Mode::direct_kway;
    context.local_search.algorithm = RefinementAlgorithm::kway_fm;
    context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
    context.evolutionary.mutate_strategy = EvoMutateStrategy::new_initial_partitioning_vcycle;
    hypergraph.reset();
    hypergraph.setNodePart(0,0);
    hypergraph.setNodePart(1,0);
    hypergraph.setNodePart(2,0);
    hypergraph.setNodePart(3,1);
    hypergraph.setNodePart(4,1);
    hypergraph.setNodePart(5,1);
    Individual ind1 = Individual(hypergraph, context);
    Individual ind2 = partition::mutate::vCycleWithNewInitialPartitioning(hypergraph, ind1, context);
    /* Due to a new initial partitioning there should be highly probable changes in the structure
       Since the Randomizer is deterministic, these changes should always be the same */
    ASSERT_EQ(ind2.partition().at(0), ind2.partition().at(2));
    ASSERT_EQ(ind2.partition().at(0), ind2.partition().at(5));
        
  }
}


