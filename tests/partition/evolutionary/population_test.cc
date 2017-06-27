#include <boost/program_options.hpp>

#include <limits>
#include <string>
#include "gmock/gmock.h"

#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/io/config_file_reader.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/metrics.h"
#include <vector>
#include "kahypar/definitions.h"
using ::testing::Eq;
using ::testing::Test;
namespace kahypar {

  class APopulation : public Test {
    public :
    APopulation() :
      population(),
      context(),
      hypergraph(4, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 5 },
               HyperedgeVector { 0,1,0,2,3}) {hypergraph.changeK(4); }
    Population population;
    Context context;
    Hypergraph hypergraph;
  };

  TEST_F(APopulation, IsCorrectlyGeneratingIndividuals) {
    //io::readInDirectKwayContext(context);
    // population.generateIndividual(hypergraph, context);
    //population.individualAt(0).printDebug();
  }
  TEST_F(APopulation, MakesTournamentSelection) {
    Context context;
    //population.insert(Individual(1), context);
    // population.insert(Individual(2), context);
    //population.insert(Individual(3), context);
    //ASSERT_EQ(population.singleTournamentSelection().fitness(), 1);
    
  }
}


