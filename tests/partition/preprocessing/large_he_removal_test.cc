/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
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
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/context.h"
#include "kahypar/partition/preprocessing/large_he_remover.h"

using ::testing::Eq;

namespace kahypar {
TEST(ALargeHERemover, RemovesHyperedgesLargerThanAThreshold) {
  Hypergraph hg(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                      HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 });
  Context context;
  context.partition.max_he_size_threshold = 2;

  LargeHyperedgeRemover large_he_remover;
  // Removes hyperedges 1, 2 and 3
  HypernodeID num_removed_hes = large_he_remover.removeLargeHyperedges(hg, context);
  ASSERT_EQ(3, num_removed_hes);
  ASSERT_TRUE(hg.edgeIsEnabled(0));
  ASSERT_FALSE(hg.edgeIsEnabled(1));
  ASSERT_FALSE(hg.edgeIsEnabled(2));
  ASSERT_FALSE(hg.edgeIsEnabled(3));
}

TEST(ALargeHERemover, RestoresHyperedgesLargerThanAThresholdCorrectly) {
  Hypergraph hg(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                      HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 });
  Context context;
  context.partition.max_he_size_threshold = 2;

  LargeHyperedgeRemover large_he_remover;
  large_he_remover.removeLargeHyperedges(hg, context);

  // Restores hyperedges 1, 2 and 3
  large_he_remover.restoreLargeHyperedges(hg, context);
  ASSERT_TRUE(hg.edgeIsEnabled(0));
  ASSERT_TRUE(hg.edgeIsEnabled(1));
  ASSERT_TRUE(hg.edgeIsEnabled(2));
  ASSERT_TRUE(hg.edgeIsEnabled(3));
}
}  // namespace kahypar
