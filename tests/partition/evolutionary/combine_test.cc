
#include "gmock/gmock.h"
#include "kahypar/definitions.h"
#include "kahypar/kahypar.h"
#include "kahypar/macros.h"
#include "kahypar/partition/coarsening/full_vertex_pair_coarsener.h"
#include "kahypar/partition/coarsening/i_coarsener.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/multilevel.h"
#include "kahypar/partition/evolutionary/individual.h"
#include "kahypar/partition/refinement/2way_fm_refiner.h"
#include "kahypar/partition/refinement/i_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"
using ::testing::Eq;
using ::testing::Test;
namespace kahypar {
using FirstWinsCoarsener = FullVertexPairCoarsener<HeavyEdgeScore,
                                                   MultiplicativePenalty,
                                                   UseCommunityStructure,
                                                   EvoPartitionPolicy,
                                                   BestRatingWithTieBreaking<FirstRatingWins>,
                                                   RatingType>;
using Refiner = TwoWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;
  class ACombineOperation : public Test {
     public :
    ACombineOperation() :
      hypergraph(new Hypergraph(8,7, HyperedgeIndexVector {0, 2, 4, 6, 8, 10, 14/*sentinel*/, 18},
				HyperedgeVector {0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 0, 1, 2, 3, 4, 5, 6, 7})),
      context(),
      coarsener(new FirstWinsCoarsener(*hypergraph, context,  /* heaviest_node_weight */ 1)),
    refiner(new Refiner(*hypergraph, context)) 
    {  context.coarsening.contraction_limit = 2;
    context.local_search.algorithm = RefinementAlgorithm::twoway_fm;
    context.coarsening.max_allowed_node_weight = 5;
    context.partition.graph_filename = "CombineTest.hgr";
    context.partition.graph_partition_filename = "CombineTest.hgr.part.2.KaHyParE";
    context.partition.epsilon = 0.15;
    context.partition.k = 2;
    context.partition.rb_lower_k = 0;
    context.partition.rb_upper_k = context.partition.k - 1;
    context.partition.total_graph_weight = 7;
    context.partition.perfect_balance_part_weights[0] = ceil(
      7 / static_cast<double>(context.partition.k));
    context.partition.perfect_balance_part_weights[1] = ceil(
      7 / static_cast<double>(context.partition.k));

    context.partition.max_part_weights[0] = (1 + context.partition.epsilon)
                                            * context.partition.perfect_balance_part_weights[0];
    context.partition.max_part_weights[1] = (1 + context.partition.epsilon)
                                            * context.partition.perfect_balance_part_weights[1];
    kahypar::Randomize::instance().setSeed(context.partition.seed);
    }

  std::unique_ptr<Hypergraph> hypergraph;
  Context context;
  std::unique_ptr<ICoarsener> coarsener;
  std::unique_ptr<IRefiner> refiner;
  };

  TEST_F(ACombineOperation, RespectsItsParents) {
    context.partition_evolutionary = true;
    context.evolutionary.action =  Action { meta::Int2Type<static_cast<int>(EvoDecision::combine)>() };
    std::vector<PartitionID> parent1 {0,1,0,1,0,1,0,1};
    std::vector<PartitionID> parent2 {0,1,1,0,0,1,1,0};
    context.evolutionary.parent1 = &parent1;
    context.evolutionary.parent2 = &parent2;

    context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
    context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;


    
    multilevel::partition(*hypergraph, *coarsener, *refiner, context);
    Individual indi(*hypergraph);
    //indi.printDebug();
    ASSERT_EQ(indi.fitness(), 6);
  }
   TEST_F(ACombineOperation, TakesTheBetterParent) {
    context.partition_evolutionary = true;
    context.evolutionary.action =  Action { meta::Int2Type<static_cast<int>(EvoDecision::combine)>() };
    std::vector<PartitionID> parent1 {0,1,0,1,0,1,0,1};
    std::vector<PartitionID> parent2 {0,0,1,1,0,0,1,1};
    context.evolutionary.parent1 = &parent1;
    context.evolutionary.parent2 = &parent2;

    context.coarsening.rating.rating_function = RatingFunction::heavy_edge;
    context.coarsening.rating.partition_policy = RatingPartitionPolicy::evolutionary;


    
    multilevel::partition(*hypergraph, *coarsener, *refiner, context);
    Individual indi(*hypergraph);
    //indi.printDebug();
    ASSERT_EQ(indi.fitness(), 2);
  }
}


