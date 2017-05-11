/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include "kahypar/datastructure/incidence_set.h"
#include "kahypar/definitions.h"
#include "kahypar/macros.h"

using ::testing::Test;
using ::testing::Eq;

namespace kahypar {
namespace ds {
class AnIncidenceSet : public Test {
 public:
  AnIncidenceSet() :
    sparse_map(20) { }

  IncidenceSet<HypernodeID> sparse_map;
};


TEST_F(AnIncidenceSet, ReturnsTrueIfElementIsInTheSet) {
  sparse_map.add(5);
  ASSERT_TRUE(sparse_map.contains(5));
}

TEST_F(AnIncidenceSet, ReturnsFalseIfElementIsNotInTheSet) {
  sparse_map.add(6);
  ASSERT_FALSE(sparse_map.contains(5));
}

TEST_F(AnIncidenceSet, ReturnsFalseIfSetIsEmpty) {
  ASSERT_FALSE(sparse_map.contains(5));
}

TEST_F(AnIncidenceSet, ReturnsFalseAfterElementIsRemoved) {
  sparse_map.add(6);
  sparse_map.add(1);
  sparse_map.add(3);

  sparse_map.remove(6);

  ASSERT_FALSE(sparse_map.contains(6));
  ASSERT_TRUE(sparse_map.contains(1));
  ASSERT_TRUE(sparse_map.contains(3));
}


TEST_F(AnIncidenceSet, HasCorrectSizeAfterElementIsRemoved) {
  sparse_map.add(6);
  sparse_map.add(1);
  sparse_map.add(3);

  sparse_map.remove(1);

  ASSERT_THAT(sparse_map.size(), Eq(2));
}

TEST_F(AnIncidenceSet, AllowsIterationOverSetElements) {
  std::vector<HypernodeID> v { 6, 1, 3 };

  sparse_map.add(6);
  sparse_map.add(1);
  sparse_map.add(3);

  size_t i = 0;
  for (const auto& element : sparse_map) {
    ASSERT_THAT(element, Eq(v[i++]));
  }
}


TEST_F(AnIncidenceSet, AllowsUndoOperationsRelevantForUncontraction) {
  std::vector<HypernodeID> v { 6, 1, 3, 25, 9, 4, 23 };

  sparse_map.add(6);
  sparse_map.add(1);
  sparse_map.add(3);
  sparse_map.add(25);
  sparse_map.add(9);
  sparse_map.add(4);
  sparse_map.add(23);

  size_t i = 0;
  for (const auto& element : sparse_map) {
    ASSERT_THAT(element, Eq(v[i++]));
  }

  std::vector<HypernodeID> w { 9, 1, 3, 23 };

  sparse_map.remove(25);
  sparse_map.remove(4);
  sparse_map.remove(6);

  i = 0;
  for (const auto& element : sparse_map) {
    ASSERT_THAT(element, Eq(w[i++]));
  }

  sparse_map.undoRemoval(6);
  sparse_map.undoRemoval(4);
  sparse_map.undoRemoval(25);

  std::vector<HypernodeID> x { 9, 1, 3, 23, 6, 4, 25 };
  i = 0;
  for (const auto& element : sparse_map) {
    ASSERT_THAT(element, Eq(x[i++]));
  }
}

TEST_F(AnIncidenceSet, AllowsUndoOperationsForReuseDuringUncontraction) {
  sparse_map.add(6);
  sparse_map.add(1);
  sparse_map.add(3);
  sparse_map.add(23);

  // simulate reuse: 3 will be reused to store 77
  sparse_map.remove(3);
  sparse_map.add(77);

  std::vector<HypernodeID> v { 6, 1, 23, 77 };
  size_t i = 0;
  for (const auto& element : sparse_map) {
    ASSERT_THAT(element, Eq(v[i++]));
  }

  sparse_map.undoReuse(77, 3);

  std::vector<HypernodeID> w { 6, 1, 23, 3 };
  i = 0;
  for (const auto& element : sparse_map) {
    ASSERT_THAT(element, Eq(w[i++]));
  }
}


TEST(AnSmallIncidenceSet, GrowsAutomatically) {
  IncidenceSet<HypernodeID> sparse_map(2);
  ASSERT_THAT(sparse_map.capacity(), Eq(2));

  sparse_map.add(6);
  sparse_map.add(1);
  sparse_map.add(3);
  sparse_map.add(23);
  sparse_map.add(5);
  sparse_map.add(12);

  ASSERT_THAT(sparse_map.capacity(), Eq(8));

  std::vector<HypernodeID> w { 6, 1, 3, 23, 5, 12 };
  size_t i = 0;
  for (const auto& element : sparse_map) {
    ASSERT_THAT(element, Eq(w[i++]));
    ASSERT_TRUE(sparse_map.contains(element));
  }
}

TEST(IncidenceSets, SupportOperationsForCoarsening) {
  IncidenceSet<HyperedgeID> dummy(10);
  IncidenceSet<HyperedgeID> incident_edges_0(10);
  IncidenceSet<HyperedgeID> incident_edges_4(10);
  IncidenceSet<HypernodeID> pins_1(10);
  IncidenceSet<HypernodeID> pins_2(10);
  IncidenceSet<HypernodeID> pins_3(10);

  std::vector<IncidenceSet<HyperedgeID>*> hypernodes;
  std::vector<IncidenceSet<HypernodeID>*> hyperedges;

  hypernodes.push_back(&incident_edges_0);
  hypernodes.push_back(&dummy);
  hypernodes.push_back(&dummy);
  hypernodes.push_back(&dummy);
  hypernodes.push_back(&incident_edges_4);

  hyperedges.push_back(&dummy);
  hyperedges.push_back(&pins_1);
  hyperedges.push_back(&pins_2);
  hyperedges.push_back(&pins_3);

  incident_edges_0.add(1);
  incident_edges_0.add(2);

  incident_edges_4.add(2);
  incident_edges_4.add(3);

  pins_1.add(0);
  pins_1.add(1);

  pins_2.add(0);
  pins_2.add(1);
  pins_2.add(3);
  pins_2.add(4);

  pins_3.add(3);
  pins_3.add(4);
  pins_3.add(6);

///////////////////////////////////////////////
// simulate contraction of (0,4)
///////////////////////////////////////////////
  for (const HyperedgeID& he : incident_edges_4) {
    LOG << "looking at HE " << he;
    if (!hyperedges[he]->contains(0)) {
      LOG << "Case 2 removal for HE " << he;
      hyperedges[he]->reuse(0, 4);
      hypernodes[0]->add(he);
    } else {
      LOG << "Case 1 removal for HE " << he;
      hyperedges[he]->remove(4);
    }
  }

  std::vector<HyperedgeID> expected_0 = { 1, 2, 3 };
  size_t i = 0;
  LOG << "incidence structure for HN" << 0;
  for (const auto& he : *hypernodes[0]) {
    LOG << he;
    ASSERT_THAT(he, Eq(expected_0[i++]));
  }

  std::vector<HyperedgeID> expected_4 = { 2, 3 };
  i = 0;
  LOG << "incidence structure for HN" << 4;
  for (const auto& he : *hypernodes[4]) {
    LOG << he;
    ASSERT_THAT(he, Eq(expected_4[i++]));
  }

  std::vector<HypernodeID> expected_pins_1 = { 0, 1 };
  i = 0;
  LOG << "incidence structure for HE" << 1;
  for (const auto& pin : *hyperedges[1]) {
    LOG << pin;
    ASSERT_THAT(pin, Eq(expected_pins_1[i++]));
  }

  std::vector<HypernodeID> expected_pins_2 = { 0, 1, 3 };
  i = 0;
  LOG << "incidence structure for HE" << 2;
  for (const auto& pin : *hyperedges[2]) {
    LOG << pin;
    ASSERT_THAT(pin, Eq(expected_pins_2[i++]));
  }

  std::vector<HypernodeID> expected_pins_3 = { 3, 6, 0 };
  i = 0;
  LOG << "incidence structure for HE" << 3;
  for (const auto& pin : *hyperedges[3]) {
    LOG << pin;
    ASSERT_THAT(pin, Eq(expected_pins_3[i++]));
  }

///////////////////////////////////////////////
// simulate uncontraction of (0,4)
///////////////////////////////////////////////
  LOG << "simulating uncontraction";
  std::vector<HyperedgeID> to_remove;
  for (const HyperedgeID& he : *hypernodes[0]) {
    if (hypernodes[4]->contains(he)) {
      // ... then we have to do some kind of restore operation.
      if (hyperedges[he]->peek() == 4) {
        // Undo case 1 operation (i.e. Pin v was just cut off by decreasing size of HE e)
        hyperedges[he]->add(4);
      } else {
        to_remove.push_back(he);
        // Undo case 2 opeations (i.e. Entry of pin v in HE e was reused to store connection to u)
        hyperedges[he]->undoReuse(0, 4);
      }
    }
  }

  for (const HyperedgeID& he : to_remove) {
    hypernodes[0]->remove(he);
  }

  LOG << "incidence structure for HN" << 0;
  std::vector<HyperedgeID> expected_0_uncontracted = { 1, 2 };
  i = 0;
  for (const auto& he : *hypernodes[0]) {
    LOG << he;
    ASSERT_THAT(he, Eq(expected_0_uncontracted[i++]));
  }

  i = 0;
  LOG << "incidence structure for HN" << 4;
  for (const auto& he : *hypernodes[4]) {
    LOG << he;
    ASSERT_THAT(he, Eq(expected_4[i++]));
  }

  i = 0;
  LOG << "incidence structure for HE" << 1;
  for (const auto& pin : *hyperedges[1]) {
    LOG << pin;
    ASSERT_THAT(pin, Eq(expected_pins_1[i++]));
  }

  std::vector<HypernodeID> expected_pins_2_uncontracted = { 0, 1, 3, 4 };
  i = 0;
  LOG << "incidence structure for HE" << 2;
  for (const auto& pin : *hyperedges[2]) {
    LOG << pin;
    ASSERT_THAT(pin, Eq(expected_pins_2_uncontracted[i++]));
  }

  std::vector<HypernodeID> expected_pins_3_uncontracted = { 3, 6, 4 };
  i = 0;
  LOG << "incidence structure for HE" << 3;
  for (const auto& pin : *hyperedges[3]) {
    LOG << pin;
    ASSERT_THAT(pin, Eq(expected_pins_3_uncontracted[i++]));
  }
}
}  // namespace ds
}  // namespace kahypar
