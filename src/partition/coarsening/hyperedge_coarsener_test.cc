/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph.h"
#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/coarsening/HeavyEdgeCoarsener_TestFixtures.h"
#include "partition/coarsening/HyperedgeCoarsener.h"
#include "partition/coarsening/Rater.h"

namespace partition {
class AHyperedgeCoarsener : public Test {
  public:
  explicit AHyperedgeCoarsener(HypergraphType* graph =
                                 new HypergraphType(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
                                                    HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })) :
    hypergraph(graph),
    config(),
    coarsener(*hypergraph, config),
    refiner(new DummyRefiner()) {
    config.coarsening.threshold_node_weight = 5;
  }

  std::unique_ptr<HypergraphType> hypergraph;
  Configuration config;
  HyperedgeCoarsener<HyperedgeRater<defs::RatingType> > coarsener;
  std::unique_ptr<IRefiner> refiner;

  private:
  DISALLOW_COPY_AND_ASSIGN(AHyperedgeCoarsener);
};

TEST_F(AHyperedgeCoarsener, RemembersMementosOfNodeContractionsDuringOneCoarseningStep) {
  coarsener.coarsen(5);
  ASSERT_THAT(coarsener._contraction_mementos.size(), Eq(2));
  ASSERT_THAT(coarsener._history.top().mementos_begin, Eq(0));
  ASSERT_THAT(coarsener._history.top().mementos_size, Eq(2));
}

TEST(HyperedgeCoarsener, RemoveNestedHyperedgesAsPartOfTheContractionRoutine) {
  HypergraphType hypergraph(5, 4, HyperedgeIndexVector { 0, 3, 7, 9, /*sentinel*/ 11 },
                            HyperedgeVector { 0, 1, 2, 0, 1, 2, 3, 2, 3, 3, 4 });
  hypergraph.setEdgeWeight(1, 5);
  Configuration config;
  config.coarsening.threshold_node_weight = 5;
  HyperedgeCoarsener<HyperedgeRater<defs::RatingType> > coarsener(hypergraph, config);

  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsEnabled(0), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(1), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsEnabled(2), Eq(false));
}
} // namespace partition
