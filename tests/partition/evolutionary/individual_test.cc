#include "gmock/gmock.h"


#include "kahypar/partition/metrics.h"
#include <vector>
#include "kahypar/definitions.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/datastructure/hypergraph.h"
using ::testing::Eq;
using ::testing::Test;
namespace kahypar {

  class AnIndividual : public Test {
    public : 
    AnIndividual() :
    individual(),
    hypergraph(4, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 5 },
               HyperedgeVector { 0,1,0,2,3}) {hypergraph.changeK(4); }
  Individual individual;

  Hypergraph hypergraph;
  };


  TEST_F(AnIndividual, IsCorrectlyExtractedFromTheHypergraphForKM1) {
    Context context;
    context.partition.objective = Objective::km1;
    hypergraph.setNodePart(0,0);
    hypergraph.setNodePart(1,1);
    hypergraph.setNodePart(2,2);
    hypergraph.setNodePart(3,3);
    individual = Individual(hypergraph, context);
    ASSERT_EQ(individual.partition()[0], 0);
    ASSERT_EQ(individual.partition()[1], 1);
    ASSERT_EQ(individual.partition()[2], 2);
    ASSERT_EQ(individual.partition()[3], 3);
    ASSERT_EQ(individual.cutEdges()[0], 0);
    ASSERT_EQ(individual.cutEdges()[1], 1);
    ASSERT_EQ(individual.cutEdges().size(), 2);
    ASSERT_EQ(individual.strongCutEdges().size(),3);
    ASSERT_EQ(individual.strongCutEdges()[0], 0);
    ASSERT_EQ(individual.strongCutEdges()[1], 1);
    ASSERT_EQ(individual.strongCutEdges()[2], 1);
    ASSERT_EQ(individual.fitness(), 3);
  }
  TEST_F(AnIndividual, IsCorrectlyExtractedFromTheHypergraphForCut) {
    Context context;
    context.partition.objective = Objective::cut;
    hypergraph.setNodePart(0,0);
    hypergraph.setNodePart(1,1);
    hypergraph.setNodePart(2,2);
    hypergraph.setNodePart(3,3);
    individual = Individual(hypergraph, context);
    ASSERT_EQ(individual.partition()[0], 0);
    ASSERT_EQ(individual.partition()[1], 1);
    ASSERT_EQ(individual.partition()[2], 2);
    ASSERT_EQ(individual.partition()[3], 3);
    ASSERT_EQ(individual.cutEdges()[0], 0);
    ASSERT_EQ(individual.cutEdges()[1], 1);
    ASSERT_EQ(individual.cutEdges().size(), 2);
    ASSERT_EQ(individual.strongCutEdges().size(),3);
    ASSERT_EQ(individual.strongCutEdges()[0], 0);
    ASSERT_EQ(individual.strongCutEdges()[1], 1);
    ASSERT_EQ(individual.strongCutEdges()[2], 1);
    ASSERT_EQ(individual.fitness(), 2);
  }
}



