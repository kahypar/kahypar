#include "gmock/gmock.h"

#include "Coarsener.h"
#include "../lib/datastructure/Hypergraph.h"

using ::testing::Test;
using ::testing::Eq;
using ::testing::DoubleEq;
using ::testing::AnyOf;

using defs::hMetisHyperEdgeIndexVector;
using defs::hMetisHyperEdgeVector;

namespace partition {

typedef datastructure::HypergraphType HypergraphType;
typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef Coarsener<HypergraphType, FirstWinsRater> CoarsenerType;
typedef HypergraphType::HypernodeID HypernodeID;
typedef HypergraphType::HypernodeWeight HypernodeWeight;

class ACoarsener : public Test {
 public:
  ACoarsener() :
      hypergraph(7,4, hMetisHyperEdgeIndexVector {0,2,6,9,/*sentinel*/12},
                 hMetisHyperEdgeVector {0,2,0,1,3,4,3,4,6,2,5,6}, nullptr, nullptr),
      threshold_node_weight(5),
      coarsener(hypergraph, threshold_node_weight) {}
  
  HypergraphType hypergraph;
  HypernodeWeight threshold_node_weight;
  CoarsenerType coarsener;
};

TEST_F(ACoarsener, RemovesHyperedgesOfSizeOneDuringCoarsening) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsValid(0), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsValid(2), Eq(false));
}

TEST_F(ACoarsener, ReAddsHyperedgesOfSizeOneOnUnContractions) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph.edgeIsValid(0), Eq(false));
  ASSERT_THAT(hypergraph.edgeIsValid(2), Eq(false));
  coarsener.uncoarsen();
  ASSERT_THAT(hypergraph.edgeIsValid(0), Eq(true));
  ASSERT_THAT(hypergraph.edgeIsValid(2), Eq(true));
}

TEST_F(ACoarsener, SelectsNodePairToContractBasedOnHighestRating) {
  coarsener.coarsen(6);
  ASSERT_THAT(hypergraph.nodeIsValid(2), Eq(false));
  ASSERT_THAT(coarsener._history.top().contraction_memento.u, Eq(0));
  ASSERT_THAT(coarsener._history.top().contraction_memento.v, Eq(2));
}

} // namespace partition
