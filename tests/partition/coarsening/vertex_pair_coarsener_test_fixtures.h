/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/refinement/do_nothing_refiner.h"
#include "kahypar/partition/refinement/i_refiner.h"

using ::testing::AnyOf;
using ::testing::DoubleEq;
using ::testing::Eq;
using ::testing::Le;
using ::testing::Test;

namespace kahypar {
template <class CoarsenerType>
class ACoarsenerBase : public Test {
 public:
  explicit ACoarsenerBase(Hypergraph* graph =
                            new Hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                                           HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })) :
    hypergraph(graph),
    context(),
    coarsener(*hypergraph, context,  /* heaviest_node_weight */ 1),
    refiner(new DoNothingRefiner()) {
    refiner->initialize(999999);
    context.partition.k = 2;
    context.partition.objective = Objective::cut;
    context.partition.epsilon = 0.3;
    context.partition.perfect_balance_part_weights.push_back(ceil(7.0 / 2));
    context.partition.perfect_balance_part_weights.push_back(ceil(7.0 / 2));
    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[0]);
    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[1]);
    kahypar::Randomize::instance().setSeed(context.partition.seed);
    context.coarsening.max_allowed_node_weight = 5;
  }

  std::unique_ptr<Hypergraph> hypergraph;
  Context context;
  CoarsenerType coarsener;
  std::unique_ptr<IRefiner> refiner;
};

template <class Coarsener, class Hypergraph>
void removesHyperedgesOfSizeOneDuringCoarsening(Coarsener& coarsener,
                                                Hypergraph& hypergraph,
                                                const size_t contraction_limit,
                                                const std::vector<HyperedgeID>& single_node_hes) {
  coarsener.coarsen(contraction_limit);
  for ( const HyperedgeID& he : single_node_hes ) {
    ASSERT_THAT(hypergraph->edgeIsEnabled(he), Eq(false));
  }
}

template <class Coarsener, class Hypergraph>
void decreasesNumberOfPinsWhenRemovingHyperedgesOfSizeOne(Coarsener& coarsener,
                                                          Hypergraph& hypergraph,
                                                          const size_t contraction_limit,
                                                          const size_t number_of_pins) {
  coarsener.coarsen(contraction_limit);
  ASSERT_THAT(hypergraph->currentNumPins(), Eq(number_of_pins));
}

template <class Coarsener, class HypergraphT, class Refiner>
void reAddsHyperedgesOfSizeOneDuringUncoarsening(Coarsener& coarsener,
                                                 HypergraphT& hypergraph,
                                                 Refiner& refiner,
                                                 const size_t contraction_limit,
                                                 const std::vector<HyperedgeID>& single_node_hes,
                                                 const std::vector<HypernodeID>& block_0,
                                                 const std::vector<HypernodeID>& block_1) {
  coarsener.coarsen(contraction_limit);
  for ( const HyperedgeID& he : single_node_hes ) {
    ASSERT_THAT(hypergraph->edgeIsEnabled(he), Eq(false));
  }
  hypergraph->printGraphState();

  for ( const HypernodeID& hn : block_0 ) {
    ASSERT_THAT(hypergraph->nodeIsEnabled(hn), Eq(true));
    hypergraph->setNodePart(hn, 0);
  }

  for ( const HypernodeID& hn : block_1 ) {
    ASSERT_THAT(hypergraph->nodeIsEnabled(hn), Eq(true));
    hypergraph->setNodePart(hn, 1);
  }

  hypergraph->initializeNumCutHyperedges();
  coarsener.uncoarsen(*refiner);
  for ( const HyperedgeID& he : single_node_hes ) {
    ASSERT_THAT(hypergraph->edgeIsEnabled(he), Eq(true));
  }
}

template <class Coarsener, class Hypergraph>
void removesParallelHyperedgesDuringCoarsening(Coarsener& coarsener,
                                               Hypergraph& hypergraph,
                                               const size_t contraction_limit,
                                               const std::vector<HyperedgeID>& parallel_hes) {
  coarsener.coarsen(contraction_limit);
  for ( const HyperedgeID& he : parallel_hes ) {
    ASSERT_THAT(hypergraph->edgeIsEnabled(he), Eq(false));
  }
}

template <class Coarsener, class Hypergraph>
void updatesEdgeWeightOfRepresentativeHyperedgeOnParallelHyperedgeRemoval(Coarsener& coarsener,
                                                                          Hypergraph& hypergraph,
                                                                          const size_t contraction_limit,
                                                                          const std::vector<std::pair<HyperedgeID, HyperedgeWeight>>& he_weights) {
  coarsener.coarsen(contraction_limit);
  for ( const auto& he_weight : he_weights ) {
    HyperedgeID he = he_weight.first;
    HyperedgeWeight weight = he_weight.second;
    ASSERT_THAT(hypergraph->edgeIsEnabled(he), Eq(true));
    ASSERT_THAT(hypergraph->edgeWeight(he), Eq(weight));
  }
}

template <class Coarsener, class Hypergraph>
void decreasesNumberOfHyperedgesOnParallelHyperedgeRemoval(Coarsener& coarsener,
                                                           Hypergraph& hypergraph,
                                                           const size_t contraction_limit,
                                                           const HyperedgeID num_hyperedges) {
  coarsener.coarsen(contraction_limit);
  ASSERT_THAT(hypergraph->currentNumEdges(), Eq(num_hyperedges));
}

template <class Coarsener, class Hypergraph>
void decreasesNumberOfPinsOnParallelHyperedgeRemoval(Coarsener& coarsener,
                                                     Hypergraph& hypergraph,
                                                     const size_t contraction_limit,
                                                     const size_t number_of_pins) {
  coarsener.coarsen(contraction_limit);
  ASSERT_THAT(hypergraph->currentNumPins(), Eq(number_of_pins));
}


template <class Coarsener, class HypergraphT, class Refiner>
void restoresParallelHyperedgesDuringUncoarsening(Coarsener& coarsener, HypergraphT& hypergraph,
                                                  Refiner& refiner,
                                                  const size_t contraction_limit,
                                                  const std::vector<HyperedgeID>& parallel_hes,
                                                  const std::vector<HypernodeID>& block_0,
                                                  const std::vector<HypernodeID>& block_1) {
  coarsener.coarsen(contraction_limit);
  hypergraph->printGraphState();
  for ( const HypernodeID& hn : block_0 ) {
    ASSERT_THAT(hypergraph->nodeIsEnabled(hn), Eq(true));
    hypergraph->setNodePart(hn, 0);
  }

  for ( const HypernodeID& hn : block_1 ) {
    ASSERT_THAT(hypergraph->nodeIsEnabled(hn), Eq(true));
    hypergraph->setNodePart(hn, 1);
  }
  hypergraph->initializeNumCutHyperedges();

  coarsener.uncoarsen(*refiner);
  for ( const HyperedgeID& he : parallel_hes ) {
    ASSERT_THAT(hypergraph->edgeIsEnabled(he), Eq(true));
  }
}

template <class CoarsenerType>
void restoresParallelHyperedgesInReverseOrder() {
  // Artificially constructed hypergraph that enforces the successive removal of
  // two successive parallel hyperedges.
  HyperedgeWeightVector edge_weights { 1, 1, 1, 1 };
  HypernodeWeightVector node_weights { 50, 1, 1 };
  Hypergraph hypergraph(3, 4, HyperedgeIndexVector { 0, 2, 4, 6,  /*sentinel*/ 8 },
                        HyperedgeVector { 0, 1, 0, 1, 0, 2, 1, 2 }, 2, &edge_weights,
                        &node_weights);

  Context context;
  context.partition.epsilon = 1.0;
  context.partition.k = 2;
  context.partition.objective = Objective::cut;
  context.partition.perfect_balance_part_weights.push_back(ceil(52.0 / 2));
  context.partition.perfect_balance_part_weights.push_back(ceil(52.0 / 2));
  context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                               * context.partition.perfect_balance_part_weights[0]);
  context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                               * context.partition.perfect_balance_part_weights[1]);

  context.coarsening.max_allowed_node_weight = 4;
  CoarsenerType coarsener(hypergraph, context,  /* heaviest_node_weight */ 1);
  std::unique_ptr<IRefiner> refiner(new DoNothingRefiner());
  refiner->initialize(999999);

  coarsener.coarsen(2);
  hypergraph.setNodePart(0, 0);

  // depends on permutation and pq
  if (hypergraph.nodeIsEnabled(1)) {
    hypergraph.setNodePart(1, 1);
  } else {
    hypergraph.setNodePart(2, 1);
  }
  hypergraph.initializeNumCutHyperedges();

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
  Hypergraph hypergraph(3, 4, HyperedgeIndexVector { 0, 2, 4, 6,  /*sentinel*/ 8 },
                        HyperedgeVector { 0, 1, 0, 1, 0, 1, 0, 2 }, 2, &edge_weights,
                        &node_weights);

  Context context;
  context.partition.epsilon = 1.0;
  context.partition.k = 2;
  context.partition.objective = Objective::cut;
  context.partition.perfect_balance_part_weights.push_back(ceil(7.0 / 2));
  context.partition.perfect_balance_part_weights.push_back(ceil(7.0 / 2));
  context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                               * context.partition.perfect_balance_part_weights[0]);
  context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                               * context.partition.perfect_balance_part_weights[1]);
  context.coarsening.max_allowed_node_weight = 4;
  CoarsenerType coarsener(hypergraph, context,  /* heaviest_node_weight */ 1);
  std::unique_ptr<IRefiner> refiner(new DoNothingRefiner());
  refiner->initialize(999999);

  coarsener.coarsen(2);

  // Lazy-Update Coarsener coarsens slightly differently, thus we have to distinguish this case.
  if (hypergraph.nodeIsEnabled(0)) {
    hypergraph.setNodePart(0, 0);
  } else {
    ASSERT_THAT(hypergraph.nodeIsEnabled(1), Eq(true));
    hypergraph.setNodePart(1, 0);
  }
  hypergraph.setNodePart(2, 0);
  // The following assertion is thrown if parallel hyperedges are restored in the order in which
  // they were removed: Assertion `_incidence_array[hypernode(pin).firstInvalidEntry() - 1] == e`
  // failed: Incorrect restore of HE 0. In order to correctly restore the hypergraph during un-
  // coarsening, we have to restore the single-node hyperedges in reverse order!
  coarsener.uncoarsen(*refiner);
}

template <class Coarsener, class HypergraphT, class Context>
void doesNotCoarsenUntilCoarseningLimit(Coarsener& coarsener, HypergraphT& hypergraph,
                                        Context& context,
                                        const size_t contraction_limit,
                                        const size_t expected_num_nodes) {
  context.coarsening.max_allowed_node_weight = 3;
  coarsener.coarsen(contraction_limit);
  for (const HypernodeID& hn : hypergraph->nodes()) {
    ASSERT_THAT(hypergraph->nodeWeight(hn), Le(context.coarsening.max_allowed_node_weight));
  }
  ASSERT_THAT(hypergraph->currentNumNodes(), Eq(expected_num_nodes));
}
}  // namespace kahypar
