/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "gtest/gtest.h"

#include "definitions.h"
#include "hgr_to_edgelist_conversion.h"
#include "macros.h"

using::testing::Test;

using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;

class HypergraphToEdgeListConversion : public Test {
 public:
  HypergraphToEdgeListConversion() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }
  Hypergraph hypergraph;
};


TEST_F(HypergraphToEdgeListConversion, OutputsCorrectNumberOfEdges) {
  EdgeVector edges = createEdgeVector(hypergraph);
  ASSERT_EQ(edges.size(), hypergraph.initialNumPins());
}

TEST_F(HypergraphToEdgeListConversion, KeepsHypernodeIDsForSourceNodes) {
  EdgeVector edges = createEdgeVector(hypergraph);
  std::vector<HypernodeID> hypernode_ids({ 0, 0, 1, 2, 2, 3, 3, 4, 4, 5, 6, 6 });

  for (size_t i = 0; i < edges.size(); ++i) {
    ASSERT_EQ(edges[i].src, hypernode_ids[i]);
  }
}

TEST_F(HypergraphToEdgeListConversion, OffsetsHyperedgeIDsByNumHypernodesForDestNodes) {
  EdgeVector edges = createEdgeVector(hypergraph);
  std::vector<HyperedgeID> hyperedge_ids({ 7, 8, 8, 7, 10, 8, 9, 8, 9, 10, 9, 10 });

  for (size_t i = 0; i < edges.size(); ++i) {
    ASSERT_EQ(edges[i].dest, hyperedge_ids[i]);
  }
}

TEST_F(HypergraphToEdgeListConversion, ProducesCorrectEdgeList) {
  EdgeVector edges = createEdgeVector(hypergraph);
  EdgeVector correct_result({ { 0, 7 }, { 0, 8 }, { 1, 8 }, { 2, 7 }, { 2, 10 }, { 3, 8 }, { 3, 9 },
                              { 4, 8 }, { 4, 9 }, { 5, 10 }, { 6, 9 }, { 6, 10 } });

  for (size_t i = 0; i < edges.size(); ++i) {
    ASSERT_EQ(edges[i], correct_result[i]);
  }
}
