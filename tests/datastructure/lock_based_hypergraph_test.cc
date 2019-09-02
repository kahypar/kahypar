/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2019 Tobias Heuer <tobias.heuer@kit.edu>
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

#include <iostream>

#include "gmock/gmock.h"

#include "tests/datastructure/lock_based_hypergraph_test_fixtures.h"

using ::testing::Eq;
using ::testing::ContainerEq;
using ::testing::Test;

namespace kahypar {
namespace ds {

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfOneHypernodes) {
  verifyParallelContractionStep( { {0, 2} } );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfTwoHypernodes1) {
  verifyParallelContractionStep( { {0, 2}, {5, 6} } );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfTwoHypernodes2) {
  verifyParallelContractionStep( { {5, 6}, {0, 2} } );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfTwoHypernodes3) {
  verifyParallelContractionStep( { {3, 4}, {0, 2} } );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfTwoHypernodes4) {
  verifyParallelContractionStep( { {0, 1}, {0, 2} } );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfTwoHypernodes5) {
  verifyParallelContractionStep( { {0, 1}, {2, 5} } );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfTwoHypernodes6) {
  verifyParallelContractionStep( { {3, 4}, {3, 6} } );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfThreeHypernodes1) {
  verifyParallelContractionStep( { {0, 2}, {3, 4}, {5, 6}} );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfThreeHypernodes2) {
  verifyParallelContractionStep( { {0, 2}, {3, 1}, {3, 6}} );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfThreeHypernodes3) {
  verifyParallelContractionStep( { {0, 2}, {6, 4}, {6, 5}} );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfThreeHypernodes4) {
  verifyParallelContractionStep( { {0, 1}, {3, 6}, {4, 5}} );
}

TEST_P(ALockBasedHypergraph, DoingParallelContractionOfFourHypernodes) {
  verifyParallelContractionStep( { {2, 0}, {5, 6}, {3, 4}, {2, 5}} );
}

} // namespace ds
} // namespace kahypar