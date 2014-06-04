/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_COARSENING_HEAVYEDGECOARSENER_TESTFIXTURES_H_
#define SRC_PARTITION_COARSENING_HEAVYEDGECOARSENER_TESTFIXTURES_H_
#include <string>
#include <vector>

#include "gmock/gmock.h"

#include "lib/definitions.h"
#include "partition/Configuration.h"
#include "partition/refinement/IRefiner.h"

using::testing::AnyOf;
using::testing::DoubleEq;
using::testing::Eq;
using::testing::Le;
using::testing::Test;

using defs::HyperedgeIndexVector;
using defs::HyperedgeWeightVector;
using defs::HypernodeWeightVector;
using defs::HyperedgeVector;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;

namespace partition {
class DummyRefiner : public IRefiner {
  void refineImpl(std::vector<HypernodeID>&, size_t,
                  HyperedgeWeight&, double, double&) final { }
  int numRepetitionsImpl() const final { return 1; }
  std::string policyStringImpl() const final { return std::string(""); }
};

template <class CoarsenerType>
class ACoarsenerBase : public Test {
  public:
  explicit ACoarsenerBase(Hypergraph* graph =
                            new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
                                           HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })) :
    hypergraph(graph),
    config(),
    coarsener(*hypergraph, config),
    refiner(new DummyRefiner()) {
    config.coarsening.threshold_node_weight = 5;
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Configuration config;
  CoarsenerType coarsener;
  std::unique_ptr<IRefiner> refiner;

  private:
  DISALLOW_COPY_AND_ASSIGN(ACoarsenerBase);
};

template <class Coarsener, class Hypergraph>
void removesHyperedgesOfSizeOneDuringCoarsening(Coarsener& coarsener, Hypergraph& hypergraph) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph->edgeIsEnabled(0), Eq(false));
  ASSERT_THAT(hypergraph->edgeIsEnabled(2), Eq(false));
}

template <class Coarsener, class Hypergraph>
void decreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne(Coarsener& coarsener, Hypergraph& hypergraph) {
  coarsener.coarsen(6);
  ASSERT_THAT(hypergraph->edgeIsEnabled(0), Eq(false));

  ASSERT_THAT(hypergraph->numPins(), Eq(10));
}

template <class Coarsener, class HypergraphT, class Refiner>
void reAddsHyperedgesOfSizeOneDuringUncoarsening(Coarsener& coarsener, HypergraphT& hypergraph, Refiner& refiner) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph->edgeIsEnabled(0), Eq(false));
  ASSERT_THAT(hypergraph->edgeIsEnabled(2), Eq(false));
  hypergraph->changeNodePartition(1, Hypergraph::kInvalidPartition, 0);
  hypergraph->changeNodePartition(3, Hypergraph::kInvalidPartition, 1);
  coarsener.uncoarsen(*refiner);
  ASSERT_THAT(hypergraph->edgeIsEnabled(0), Eq(true));
  ASSERT_THAT(hypergraph->edgeIsEnabled(2), Eq(true));
  ASSERT_THAT(hypergraph->edgeSize(1), Eq(4));
  ASSERT_THAT(hypergraph->edgeSize(3), Eq(3));
}

template <class Coarsener, class Hypergraph>
void removesParallelHyperedgesDuringCoarsening(Coarsener& coarsener, Hypergraph& hypergraph) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph->edgeIsEnabled(3), Eq(false));
  ASSERT_THAT(hypergraph->edgeIsEnabled(1), Eq(true));
}

template <class Coarsener, class Hypergraph>
void updatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval(Coarsener& coarsener, Hypergraph& hypergraph) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph->edgeWeight(1), Eq(2));
}

template <class Coarsener, class Hypergraph>
void decreasesNumberOfHyperedgesOnParallelHyperedgeRemoval(Coarsener& coarsener, Hypergraph& hypergraph) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph->numEdges(), Eq(1));
}

template <class Coarsener, class Hypergraph>
void decreasesNumberOfPinsOnParallelHyperedgeRemoval(Coarsener& coarsener, Hypergraph& hypergraph) {
  coarsener.coarsen(2);
  ASSERT_THAT(hypergraph->numPins(), Eq(2));
}


template <class Coarsener, class HypergraphT, class Refiner>
void restoresParallelHyperedgesDuringUncoarsening(Coarsener& coarsener, HypergraphT& hypergraph, Refiner& refiner) {
  coarsener.coarsen(2);
  hypergraph->changeNodePartition(1, Hypergraph::kInvalidPartition, 0);
  hypergraph->changeNodePartition(4, Hypergraph::kInvalidPartition, 1);

  coarsener.uncoarsen(*refiner);
  ASSERT_THAT(hypergraph->edgeSize(1), Eq(4));
  ASSERT_THAT(hypergraph->edgeSize(3), Eq(3));
  ASSERT_THAT(hypergraph->edgeWeight(1), Eq(1));
  ASSERT_THAT(hypergraph->edgeWeight(3), Eq(1));
}

template <class CoarsenerType>
void restoresParallelHyperedgesInReverseOrder() {
  // Artificially constructed hypergraph that enforces the successive removal of
  // two successive parallel hyperedges.
  HyperedgeWeightVector edge_weights { 1, 1, 1, 1 };
  HypernodeWeightVector node_weights { 50, 1, 1 };
  Hypergraph hypergraph(3, 4, HyperedgeIndexVector { 0, 2, 4, 6, /*sentinel*/ 8 },
                        HyperedgeVector { 0, 1, 0, 1, 0, 2, 1, 2 }, &edge_weights,
                        &node_weights);

  Configuration config;
  config.coarsening.threshold_node_weight = 4;
  CoarsenerType coarsener(hypergraph, config);
  std::unique_ptr<IRefiner> refiner(new DummyRefiner());

  coarsener.coarsen(2);
  hypergraph.changeNodePartition(0, Hypergraph::kInvalidPartition, 0);
  hypergraph.changeNodePartition(1, Hypergraph::kInvalidPartition, 1);

  // The following assertion is thrown if parallel hyperedges are restored in the order in which
  // they were removed: Assertion `_incidence_array[hypernode(pin).firstInvalidEntry() - 1] == e`
  // failed: Incorrect restore of HE 1. In order to correctly restore the hypergraph during un-
  // coarsening, we have to restore the parallel hyperedges in reverse order!
  coarsener.uncoarsen(*refiner);
}

template <class CoarsenerType>
void restoresSingleNodeHyperedgesInReverseOrder() {
  // Artificially constructed hypergraph that enforces the successive removal of
  // three single-node hyperedges.
  HyperedgeWeightVector edge_weights { 5, 5, 5, 1 };
  HypernodeWeightVector node_weights { 1, 1, 5 };
  Hypergraph hypergraph(3, 4, HyperedgeIndexVector { 0, 2, 4, 6, /*sentinel*/ 8 },
                        HyperedgeVector { 0, 1, 0, 1, 0, 1, 0, 2 }, &edge_weights,
                        &node_weights);

  Configuration config;
  config.coarsening.threshold_node_weight = 4;
  CoarsenerType coarsener(hypergraph, config);
  std::unique_ptr<IRefiner> refiner(new DummyRefiner());

  coarsener.coarsen(2);
  hypergraph.changeNodePartition(0, Hypergraph::kInvalidPartition, 0);
  hypergraph.changeNodePartition(2, Hypergraph::kInvalidPartition, 0);
  // The following assertion is thrown if parallel hyperedges are restored in the order in which
  // they were removed: Assertion `_incidence_array[hypernode(pin).firstInvalidEntry() - 1] == e`
  // failed: Incorrect restore of HE 0. In order to correctly restore the hypergraph during un-
  // coarsening, we have to restore the single-node hyperedges in reverse order!
  coarsener.uncoarsen(*refiner);
}

template <class Coarsener, class HypergraphT, class Config>
void doesNotCoarsenUntilCoarseningLimit(Coarsener& coarsener, HypergraphT& hypergraph, Config& config) {
  config.coarsening.threshold_node_weight = 3;
  coarsener.coarsen(2);
  Hypergraph& hgr = *hypergraph;
  forall_hypernodes(hn, hgr) {
    ASSERT_THAT(hgr.nodeWeight(*hn), Le(3));
  } endfor
    ASSERT_THAT(hypergraph->numNodes(), Eq(3));
}
} // namespace partition

#endif  // SRC_PARTITION_COARSENING_HEAVYEDGECOARSENER_TESTFIXTURES_H_
