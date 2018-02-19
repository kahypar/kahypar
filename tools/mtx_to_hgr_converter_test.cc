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

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "tools/mtx_to_hgr_conversion.h"

using ::testing::Test;
using ::testing::ContainerEq;

using namespace kahypar;

namespace mtxconversion {
TEST(AnMtxToHgrConversionRoutine, ParsesMTXHeaderAndReturnsMatrixType) {
  std::string filename("test_instances/CoordinateSymmetric.mtx");
  std::ifstream file(filename);
  MatrixInfo matrix = parseHeader(file);
  ASSERT_EQ(matrix.format, MatrixFormat::COORDINATE);
  ASSERT_EQ(matrix.symmetry, MatrixSymmetry::SYMMETRIC);
}

TEST(AnMtxToHgrConversionRoutine, ParsesRowAndColumnInformation) {
  std::string filename("test_instances/CoordinateSymmetric.mtx");
  std::ifstream file(filename);
  MatrixInfo info = parseHeader(file);
  parseDimensionInformation(file, info);
  ASSERT_EQ(info.num_rows, 3);
  ASSERT_EQ(info.num_columns, 3);
  ASSERT_EQ(info.num_entries, 4);
}

TEST(AnMtxToHgrConversionRoutine, ParsesMatrixEntries) {
  std::string filename("test_instances/CoordinateSymmetric.mtx");
  std::ifstream file(filename);
  MatrixInfo info = parseHeader(file);
  parseDimensionInformation(file, info);
  MatrixData matrix_data;
  parseMatrixEntries(file, info, matrix_data);
  ASSERT_EQ(matrix_data[0].size(), 1);
  ASSERT_EQ(matrix_data[0][0], 0);
  ASSERT_EQ(matrix_data[1].size(), 2);
  ASSERT_EQ(matrix_data[1][0], 1);
  ASSERT_EQ(matrix_data[1][1], 2);
  ASSERT_EQ(matrix_data[2].size(), 2);
  ASSERT_EQ(matrix_data[2][0], 2);
  ASSERT_EQ(matrix_data[2][1], 1);
}

TEST(AnMtxToHgrConversionRoutine, ConvertsSymmetricCoordinateMTXMatrixToHGRFormat) {
  std::string mtx_filename("test_instances/CoordinateSymmetric.mtx");
  std::string hgr_filename("test_instances/CoordinateSymmetric.hgr");
  convertMtxToHgr(mtx_filename, hgr_filename);
}

TEST(AnMtxToHgrConversionRoutine, ConvertsGeneralCoordinateMTXMatrixToHGRFormat) {
  std::string mtx_filename("test_instances/CoordinateGeneral.mtx");
  std::string hgr_filename("test_instances/CoordinateGeneral.hgr");
  convertMtxToHgr(mtx_filename, hgr_filename);
}

TEST(AnMtxToHgrConversionRoutine, AdjustsNumberOfHyperedgesIfEmptyRowsArePresent) {
  std::string mtx_filename("test_instances/EmptyRows.mtx");
  std::string hgr_filename("test_instances/EmptyRows.hgr");
  std::string correct_hgr_filename("test_instances/EmptyRowsCorrect.hgr");
  convertMtxToHgr(mtx_filename, hgr_filename);

  Hypergraph correct_hypergraph = kahypar::io::createHypergraphFromFile(correct_hgr_filename, 2);
  Hypergraph hypergraph = kahypar::io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(kahypar::ds::verifyEquivalenceWithoutPartitionInfo(hypergraph,
                                                               correct_hypergraph), true);
}

TEST(MTXfiles, CanBeConvertedToHgrsForNonsymmetricLAMAparitioning) {
  std::string mtx_filename("test_instances/CoordinateGeneral.mtx");
  std::string hgr_filename("test_instances/CoordinateGeneral_lama.hgr");

  mtxconversion::convertMtxToHgrForNonsymmetricParallelSPM(mtx_filename, hgr_filename);
  Hypergraph hypergraph = kahypar::io::createHypergraphFromFile(hgr_filename, 2);

  ASSERT_EQ(hypergraph.initialNumNodes(), 10);
  ASSERT_EQ(hypergraph.initialNumEdges(), 5);

  ASSERT_TRUE(hypergraph.type() == Hypergraph::Type::NodeWeights);

  ASSERT_EQ(hypergraph.edgeWeight(0), 1);
  ASSERT_EQ(hypergraph.edgeWeight(1), 1);
  ASSERT_EQ(hypergraph.edgeWeight(2), 1);
  ASSERT_EQ(hypergraph.edgeWeight(3), 1);
  ASSERT_EQ(hypergraph.edgeWeight(4), 1);

  ASSERT_EQ(hypergraph.nodeWeight(0), 2);
  ASSERT_EQ(hypergraph.nodeWeight(1), 1);
  ASSERT_EQ(hypergraph.nodeWeight(2), 1);
  ASSERT_EQ(hypergraph.nodeWeight(3), 3);
  ASSERT_EQ(hypergraph.nodeWeight(4), 1);
  ASSERT_EQ(hypergraph.nodeWeight(5), 1);
  ASSERT_EQ(hypergraph.nodeWeight(6), 1);
  ASSERT_EQ(hypergraph.nodeWeight(7), 1);
  ASSERT_EQ(hypergraph.nodeWeight(8), 1);
  ASSERT_EQ(hypergraph.nodeWeight(9), 1);

  ASSERT_THAT(std::vector<HypernodeID>(hypergraph.pins(0).first, hypergraph.pins(0).second),
              ContainerEq(std::vector<HypernodeID>{ 5, 0 }));
  ASSERT_THAT(std::vector<HypernodeID>(hypergraph.pins(1).first, hypergraph.pins(1).second),
              ContainerEq(std::vector<HypernodeID>{ 6, 1, 3 }));
  ASSERT_THAT(std::vector<HypernodeID>(hypergraph.pins(2).first, hypergraph.pins(2).second),
              ContainerEq(std::vector<HypernodeID>{ 7, 2 }));
  ASSERT_THAT(std::vector<HypernodeID>(hypergraph.pins(3).first, hypergraph.pins(3).second),
              ContainerEq(std::vector<HypernodeID>{ 8, 0, 3 }));
  ASSERT_THAT(std::vector<HypernodeID>(hypergraph.pins(4).first, hypergraph.pins(4).second),
              ContainerEq(std::vector<HypernodeID>{ 9, 3, 4 }));
}
}  // namespace mtxconversion
