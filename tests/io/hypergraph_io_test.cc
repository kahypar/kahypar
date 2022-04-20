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

#include "gmock/gmock.h"

#include "kahypar/io/hypergraph_io.h"
#include "tests/io/hypergraph_io_test_fixtures.h"

using ::testing::Eq;
using ::testing::ContainerEq;

namespace kahypar {
namespace io {
TEST(AFunction, ParsesFirstLineOfaHGRFile) {
  std::string filename("test_instances/unweighted_hypergraph.hgr");
  std::ifstream file(filename);
  HyperedgeID num_hyperedges = 0;
  HypernodeID num_hypernodes = 0;
  HypergraphType hypergraph_type = HypergraphType::Unweighted;

  readHGRHeader(file, num_hyperedges, num_hypernodes, hypergraph_type);
  ASSERT_THAT(num_hyperedges, Eq(4));
  ASSERT_THAT(num_hypernodes, Eq(7));
  ASSERT_THAT(hypergraph_type, Eq(HypergraphType::Unweighted));
}

TEST_F(AnUnweightedHypergraphFile, CanBeParsedIntoAHypergraph) {
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;

  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector);

  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  Hypergraph hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector);
}

TEST_F(AHypergraphFileWithHyperedgeWeights, CanBeParsedIntoAHypergraph) {
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HyperedgeWeightVector hyperedge_weights;

  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                     &hyperedge_weights);

  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  ASSERT_THAT(hyperedge_weights, ContainerEq(_control_hyperedge_weights));
  Hypergraph hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                        2, &hyperedge_weights);
}

TEST_F(AHypergraphFileWithHypernodeWeights, CanBeParsedIntoAHypergraph) {
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HypernodeWeightVector hypernode_weights;

  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                     nullptr, &hypernode_weights);

  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  // ASSERT_THAT(hypernode_weights, ContainerEq(_control_hypernode_weights));
  Hypergraph hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                        2, nullptr, &hypernode_weights);
}

TEST_F(AHypergraphFileWithHypernodeAndHyperedgeWeights, CanBeParsedIntoAHypergraph) {
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HypernodeWeightVector hypernode_weights;
  HyperedgeWeightVector hyperedge_weights;

  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                     &hyperedge_weights, &hypernode_weights);

  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  ASSERT_THAT(hyperedge_weights, ContainerEq(_control_hyperedge_weights));
  ASSERT_THAT(hypernode_weights, ContainerEq(_control_hypernode_weights));
  Hypergraph hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                        2, &hyperedge_weights, &hypernode_weights);
}

TEST_F(AHypergraphFileWithoutHyperedges, CanBeParsedIntoAHypergraphIfFileContainesHypernodeWeights) {
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HypernodeWeightVector hypernode_weights;
  HyperedgeWeightVector hyperedge_weights;
  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                     &hyperedge_weights, &hypernode_weights);
  ASSERT_THAT(index_vector.size(), Eq(1));
  ASSERT_THAT(edge_vector.empty(), Eq(true));
  ASSERT_THAT(_num_hypernodes, Eq(3));
  ASSERT_THAT(_num_hyperedges, Eq(0));
  ASSERT_THAT(hypernode_weights, ::testing::ContainerEq(_control_hypernode_weights));
  ASSERT_THAT(hyperedge_weights.empty(), Eq(true));

  Hypergraph hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                        2, &hyperedge_weights, &hypernode_weights);
  ASSERT_THAT(hypergraph.initialNumNodes(), Eq(3));
  ASSERT_THAT(hypergraph.currentNumEdges(), Eq(0));
  for (const HypernodeID& hn : hypergraph.nodes()) {
    ASSERT_THAT(hypergraph.nodeWeight(hn), Eq(_control_hypernode_weights[hn]));
  }
}

TEST(AHypergraphWithoutHyperedges, CanBeWrittenToFile) {
  HypernodeID num_hypernodes = 0;
  HyperedgeID num_hyperedges = 0;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HypernodeWeightVector hypernode_weights;
  HyperedgeWeightVector hyperedge_weights;
  std::string input_filename("test_instances/hypergraph_without_hyperedges.hgr");
  std::string output_filename("test_instances/hypergraph_without_hyperedges.hgr.out");
  readHypergraphFile(input_filename, num_hypernodes, num_hyperedges, index_vector, edge_vector,
                     &hyperedge_weights, &hypernode_weights);
  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector,
                        2, &hyperedge_weights, &hypernode_weights);
  num_hypernodes = 0;
  num_hyperedges = 0;
  index_vector.clear();
  edge_vector.clear();
  hypernode_weights.clear();
  hyperedge_weights.clear();


  writeHypergraphFile(hypergraph, output_filename);
  readHypergraphFile(output_filename, num_hypernodes, num_hyperedges, index_vector, edge_vector,
                     &hyperedge_weights, &hypernode_weights);
  Hypergraph hypergraph_from_file(num_hypernodes, num_hyperedges, index_vector, edge_vector,
                                  2, &hyperedge_weights, &hypernode_weights);

  ASSERT_THAT(verifyEquivalenceWithPartitionInfo(hypergraph, hypergraph_from_file), Eq(true));
}

TEST_F(AnUnweightedHypergraph, CanBeWrittenToFile) {
  writeHypergraphFile(*_hypergraph, _filename);

  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, _written_index_vector,
                     _written_edge_vector);
  Hypergraph hypergraph2(_num_hypernodes, _num_hyperedges, _written_index_vector,
                         _written_edge_vector);

  ASSERT_THAT(verifyEquivalenceWithPartitionInfo(*_hypergraph, hypergraph2), Eq(true));
}

TEST_F(AHypergraphWithHyperedgeWeights, CanBeWrittenToFile) {
  writeHypergraphFile(*_hypergraph, _filename);

  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, _written_index_vector,
                     _written_edge_vector, &_written_hyperedge_weights, nullptr);
  Hypergraph hypergraph2(_num_hypernodes, _num_hyperedges, _written_index_vector,
                         _written_edge_vector, 2, &_written_hyperedge_weights);
  ASSERT_THAT(verifyEquivalenceWithPartitionInfo(*_hypergraph, hypergraph2), Eq(true));
}

TEST_F(AHypergraphWithHypernodeWeights, CanBeWrittenToFile) {
  writeHypergraphFile(*_hypergraph, _filename);

  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, _written_index_vector,
                     _written_edge_vector, nullptr, &_written_hypernode_weights);
  Hypergraph hypergraph2(_num_hypernodes, _num_hyperedges, _written_index_vector,
                         _written_edge_vector, 2, nullptr, &_written_hypernode_weights);

  ASSERT_THAT(verifyEquivalenceWithPartitionInfo(*_hypergraph, hypergraph2), Eq(true));
}

TEST_F(AHypergraphWithHypernodeAndHyperedgeWeights, CanBeWrittenToFile) {
  writeHypergraphFile(*_hypergraph, _filename);

  readHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, _written_index_vector,
                     _written_edge_vector, &_written_hyperedge_weights,
                     &_written_hypernode_weights);
  Hypergraph hypergraph2(_num_hypernodes, _num_hyperedges, _written_index_vector,
                         _written_edge_vector, 2, &_written_hyperedge_weights,
                         &_written_hypernode_weights);

  ASSERT_THAT(verifyEquivalenceWithPartitionInfo(*_hypergraph, hypergraph2), Eq(true));
}

TEST_F(APartitionOfAHypergraph, IsCorrectlyWrittenToFile) {
  multilevel::partition(_hypergraph, *_coarsener, *_refiner, _context);
  writePartitionFile(_hypergraph, _context.partition.graph_partition_filename);

  std::vector<PartitionID> read_partition;
  readPartitionFile(_context.partition.graph_partition_filename, read_partition);
  for (const HypernodeID& hn : _hypergraph.nodes()) {
    ASSERT_THAT(read_partition[hn], Eq(_hypergraph.partID(hn)));
  }
}

TEST(AHypergraph, CanBeSerializedToPaToHFormat) {
  HyperedgeWeightVector he_weights = { 10, 15, 13, 18, 25, 20, 14, 27, 29 };
  HypernodeWeightVector hn_weights = HypernodeWeightVector { 80, 85, 30, 55, 42, 39, 90, 102 };
  Hypergraph hypergraph(8, 9, HyperedgeIndexVector { 0, 5, 9, 13, 15, 17, 20, 23, 26,  /*sentinel*/ 28 },
                        HyperedgeVector { 7, 5, 2, 4, 1, 3, 4, 0, 6, 3, 1, 4, 6, 3, 6, 2, 4, 7, 1, 3, 5, 4, 1, 4, 6, 1, 7, 3 },
                        2, &he_weights, &hn_weights);

  std::unordered_map<HypernodeID, HypernodeID> mapping;
  for (int i = 0; i < 8; ++i) {
    mapping[i] = i;
  }
  writeHypergraphForPaToHPartitioning(hypergraph, "serialized_hypergraph.patoh", mapping);

  std::ifstream file;
  std::vector<std::string> original_lines;
  std::vector<std::string> serialized_lines;
  std::string tmp;

  file.open("serialized_hypergraph.patoh", std::ifstream::in);
  while (getline(file, tmp)) {
    serialized_lines.push_back(std::move(tmp));
  }
  file.close();

  LOG << serialized_lines.size();

  file.open("test_instances/example_hypergraph.patoh", std::ifstream::in);
  while (getline(file, tmp)) {
    original_lines.push_back(std::move(tmp));
  }
  file.close();

  ASSERT_THAT(serialized_lines, ::testing::ContainerEq(original_lines));
}

TEST(AHypergraphDeathTest, WithEmptyHyperedgesLeadsToProgramExit) {
  EXPECT_EXIT(createHypergraphFromFile("test_instances/corrupted_hypergraph_with_empty_hyperedges.hgr", 2),
              ::testing::ExitedWithCode(1),
              "Error: Hyperedge 1 is empty");
}
}  // namespace io
}  // namespace kahypar
