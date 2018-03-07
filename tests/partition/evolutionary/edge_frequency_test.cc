#include "gmock/gmock.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/context_enum_classes.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/edge_frequency.h"
#include "kahypar/utils/randomize.h"
using ::testing::Eq;
using ::testing::Test;

namespace kahypar {

  class TheEdgeFrequencyCalculation : public Test {
    public :
    TheEdgeFrequencyCalculation() :
      context(),
      hypergraph(8, 5, HyperedgeIndexVector { 0, 2, 4, 7, 10, /*sentinel*/ 15 },
               HyperedgeVector {0,1,4,5,1,5,6,3,6,7,0,1,2,4,5}) {hypergraph.changeK(2); }
    
    Context context;
    Hypergraph hypergraph;

  };

  TEST_F(TheEdgeFrequencyCalculation, IsSummarizingCorrectly) {
    context.partition.objective = Objective::km1;
    hypergraph.reset();
    hypergraph.setNodePart(0,0);
    hypergraph.setNodePart(1,0);
    hypergraph.setNodePart(2,1);
    hypergraph.setNodePart(3,1);
    hypergraph.setNodePart(4,0);
    hypergraph.setNodePart(5,0);
    hypergraph.setNodePart(6,1);
    hypergraph.setNodePart(7,1);
    Individual ind1 = Individual(hypergraph, context);
    hypergraph.reset();
    hypergraph.setNodePart(0,0);
    hypergraph.setNodePart(1,0);
    hypergraph.setNodePart(2,1);
    hypergraph.setNodePart(3,0);
    hypergraph.setNodePart(4,0);
    hypergraph.setNodePart(5,0);
    hypergraph.setNodePart(6,0);
    hypergraph.setNodePart(7,0);
    Individual ind2 = Individual(hypergraph, context);
    Individuals individuals;
    individuals.push_back(ind1);
    individuals.push_back(ind2);
    std::vector<size_t> results = computeEdgeFrequency(individuals, 5);
    ASSERT_EQ(results.at(0),0);
    ASSERT_EQ(results.at(1),0);
    ASSERT_EQ(results.at(2),1);
    ASSERT_EQ(results.at(3),0);
    ASSERT_EQ(results.at(4),2);
  } 
}


