#include <boost/program_options.hpp>

#include <limits>
#include <string>
#include "gmock/gmock.h"
#include "kahypar/application/command_line_options.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/evolutionary/mutate.h"
#include "kahypar/partition/evolutionary/population.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/evo_partitioner.h"
#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include <vector>

using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
namespace partition {

  class TheEvoPartitioner : public Test {
    public :
    TheEvoPartitioner() :
      context(),
    //hypergraph(6, 3, HyperedgeIndexVector { 0, 3, 6, /*sentinel*/ 8},
    //           HyperedgeVector {0,1,2,3,4,5,4,5}) {hypergraph.changeK(2); }
    hypergraph(6, 1, HyperedgeIndexVector {0, 6},
                HyperedgeVector {0,1,2,3,4,5})
                {
                hypergraph.changeK(2);
                parseIniToContext(context, "../../../../config/km1_direct_kway_alenex17.ini");
                context.partition.k = 2;
                context.partition.epsilon = 0.03;
                context.partition.objective = Objective::cut;
                context.partition.mode = Mode::direct_kway;
                context.local_search.algorithm = RefinementAlgorithm::kway_fm;
                context.evolutionary.replace_strategy = EvoReplaceStrategy::diverse;
                context.evolutionary.mutate_strategy = EvoMutateStrategy::vcycle;
                context.evolutionary.mutation_chance = 0.2;
                context.evolutionary.diversify_interval = -1; 
               
               }
    Context context;
    
    Hypergraph hypergraph;
    private :
    FRIEND_TEST(TheEvoPartitioner, PicksTheRightStrategy);
  };

  TEST_F(TheEvoPartitioner, IsCorrectlyDecidingTheActions) {
   

    EvoPartitioner evo_part(context);


    const int nr_runs = 10;
    float chances[nr_runs];
    Randomize::instance().setSeed(1);
    for(int i = 0; i < nr_runs; ++i) {
    
        chances[i] =  Randomize::instance().getRandomFloat(0,1);

      }
    Randomize::instance().setSeed(1);
    EvoDecision decision;
    for(int i = 0; i < nr_runs; ++i) {
      decision = evo_part.decideNextMove(context);

      ASSERT_EQ((chances[i] < 0.2),(decision == EvoDecision::mutation));
      ASSERT_EQ((chances[i] >= 0.2),(decision == EvoDecision::combine));
    }

    

 
  }
  TEST_F(TheEvoPartitioner, RespectsLimitsOfTheInitialPopulation) {
    context.partition.quiet_mode = true;
    EvoPartitioner evo_part(context);
    evo_part.generateInitialPopulation(hypergraph,context);
    ASSERT_EQ(evo_part._population.size(), 50);
  }

  TEST_F(TheEvoPartitioner, ProperlyGeneratesTheInitialPopulation) {
    context.partition.quiet_mode = true;
    context.evolutionary.time_limit_seconds = 60;
    context.partition.graph_filename = "../../../../tests/partition/evolutionary/ISPD98_ibm09.hgr";
      Hypergraph hypergraph(
    kahypar::io::createHypergraphFromFile(context.partition.graph_filename,
                                          context.partition.k));


    EvoPartitioner evo_part(context);
    evo_part.generateInitialPopulation(hypergraph,context);
   
      Partitioner partitioner;
      HighResClockTimepoint start = std::chrono::high_resolution_clock::now();
      partitioner.partition(hypergraph, context);
      HighResClockTimepoint end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed_seconds = end - start;
      std::cout << elapsed_seconds.count();
  }
   
}
}


