/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2017 Sebastian Schlag <sebastian.schlag@kit.edu>
 * Copyright (C) 2017 Tobias Heuer <tobias.heuer@gmx.net>
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

#include "kahypar/datastructure/union_find.h"

using ::testing::Eq;
using ::testing::Test;


namespace kahypar {
namespace ds {

class AUnionFind : public Test {
 public:
  AUnionFind() :
    uf(10) { }

  UnionFind uf;
};

TEST_F(AUnionFind, HasEachNodeAsRepresentativeForItsOwnSet) {
  for ( HypernodeID u = 0; u < 10; ++u ) {
    ASSERT_EQ(u, uf.find(u));
  }
}

TEST_F(AUnionFind, LinksTwoElements) {
  uf.link(0, 1);
  ASSERT_EQ(0, uf.find(0));
  ASSERT_EQ(0, uf.find(1));
}

TEST_F(AUnionFind, LinksTwoSets) {
  uf.link(0, 1);
  uf.link(2, 3);
  uf.link(0, 3);
  ASSERT_EQ(0, uf.find(0));
  ASSERT_EQ(0, uf.find(1));
  ASSERT_EQ(0, uf.find(2));
  ASSERT_EQ(0, uf.find(3));
}

TEST_F(AUnionFind, LinksThreeSets) {
  uf.link(0, 1);
  uf.link(2, 3);
  uf.link(4, 5);
  uf.link(3, 5);
  uf.link(0, 4);
  ASSERT_EQ(2, uf.find(0));
  ASSERT_EQ(2, uf.find(1));
  ASSERT_EQ(2, uf.find(2));
  ASSERT_EQ(2, uf.find(3));
  ASSERT_EQ(2, uf.find(4));
  ASSERT_EQ(2, uf.find(5));
}

TEST_F(AUnionFind, ChainingAllElements) {
  for ( HypernodeID i = 1; i < 10; ++i ) {
    uf.link(i - 1, i);
  }
  for ( HypernodeID i = 0; i < 10; ++i ) {
    ASSERT_EQ(0, uf.find(i));
  }
}

TEST_F(AUnionFind, HasEachNodeAsRepresentativeForItsOwnSetAfterReset) {
  for ( HypernodeID i = 1; i < 10; ++i ) {
    uf.link(i - 1, i);
  }
  uf.reset();
  for ( HypernodeID u = 0; u < 10; ++u ) {
    ASSERT_EQ(u, uf.find(u));
  }
}

}  // namespace ds
}  // namespace kahypar
