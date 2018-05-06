/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2015 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "gmock/gmock.h"

#include "kahypar/definitions.h"
#include "kahypar/partition/refinement/kway_fm_cut_refiner.h"
#include "kahypar/partition/refinement/policies/fm_stop_policy.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
using KWayFMRefinerSimpleStopping = KWayFMRefiner<NumberOfFruitlessMovesStopsSearch>;

class AKwayFMRefiner : public Test {
 public:
  AKwayFMRefiner() :
    context(),
    hypergraph(new Hypergraph(2, 2, HyperedgeIndexVector { 0, 2,  /*sentinel*/ 3 },
                              HyperedgeVector { 0, 1, 0 }, 2)),
    refiner() {
    context.local_search.fm.max_number_of_fruitless_moves = 50;
    context.partition.k = 2;
    context.partition.rb_lower_k = 0;
    context.partition.rb_upper_k = context.partition.k - 1;
    context.partition.epsilon = 1.0;
    context.partition.perfect_balance_part_weights.push_back(ceil(hypergraph->totalWeight() /
                                                                  static_cast<double>(context.partition.k)));
    context.partition.perfect_balance_part_weights.push_back(ceil(hypergraph->totalWeight() /
                                                                  static_cast<double>(context.partition.k)));

    context.partition.max_part_weights.push_back((1 + context.partition.epsilon)
                                                 * context.partition.perfect_balance_part_weights[0]);
    context.partition.max_part_weights.push_back(context.partition.max_part_weights[0]);

    hypergraph->setNodePart(0, 0);
    hypergraph->setNodePart(1, 1);
    hypergraph->initializeNumCutHyperedges();

    refiner = std::make_unique<KWayFMRefinerSimpleStopping>(*hypergraph, context);
  }

  Context context;
  std::unique_ptr<Hypergraph> hypergraph;
  std::unique_ptr<KWayFMRefinerSimpleStopping> refiner;
};

using AKwayFMRefinerDeathTest = AKwayFMRefiner;

TEST_F(AKwayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInitialGainComputation) {
  ASSERT_DEBUG_DEATH(refiner->initialize(100), ".*");
}

TEST_F(AKwayFMRefinerDeathTest, ConsidersSingleNodeHEsDuringInducedGainComputation) {
  ASSERT_DEBUG_DEATH(refiner->initialize(100), ".*");
}

TEST_F(AKwayFMRefiner, KnowsIfAHyperedgeIsFullyActive) {
  hypergraph.reset(new Hypergraph(3, 1, HyperedgeIndexVector { 0,  /*sentinel*/ 3 },
                                  HyperedgeVector { 0, 1, 2 }, 2));
  hypergraph->setNodePart(0, 0);
  hypergraph->setNodePart(1, 0);
  hypergraph->setNodePart(2, 0);
  hypergraph->initializeNumCutHyperedges();

  refiner = std::make_unique<KWayFMRefinerSimpleStopping>(*hypergraph, context);
  refiner->initialize(100);
  refiner->_hg.activate(0);
  hypergraph->changeNodePart(0, 0, 1);
  refiner->_hg.mark(0);

  refiner->fullUpdate(0, 0, 1, 0);
  ASSERT_THAT(refiner->_he_fully_active[0], Eq(true));
}
}  // namespace kahypar
