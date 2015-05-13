/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <gmock/gmock.h>

#include "tools/MtxToHgrConversion.h"

using::testing::Test;
using::testing::Eq;

namespace mtxconversion {
TEST(AnMtxToHgrConversionRoutine, ParsesMTXHeaderAndReturnsMatrixType) {
  std::string filename("test_instances/CoordinateSymmetric.mtx");
  std::ifstream file(filename);
  MatrixInfo matrix = parseHeader(file);
  ASSERT_THAT(matrix.format, Eq(MatrixFormat::COORDINATE));
  ASSERT_THAT(matrix.symmetry, Eq(MatrixSymmetry::SYMMETRIC));
}

TEST(AnMtxToHgrConversionRoutine, ParsesRowAndColumnInformation) {
  std::string filename("test_instances/CoordinateSymmetric.mtx");
  std::ifstream file(filename);
  MatrixInfo info = parseHeader(file);
  parseDimensionInformation(file, info);
  ASSERT_THAT(info.num_rows, Eq(3));
  ASSERT_THAT(info.num_columns, Eq(3));
  ASSERT_THAT(info.num_entries, Eq(4));
}

TEST(AnMtxToHgrConversionRoutine, ParsesMatrixEntries) {
  std::string filename("test_instances/CoordinateSymmetric.mtx");
  std::ifstream file(filename);
  MatrixInfo info = parseHeader(file);
  parseDimensionInformation(file, info);
  MatrixData matrix_data;
  parseMatrixEntries(file, info, matrix_data);
  ASSERT_THAT(matrix_data[0].size(), Eq(1));
  ASSERT_THAT(matrix_data[0][0], Eq(0));
  ASSERT_THAT(matrix_data[1].size(), Eq(2));
  ASSERT_THAT(matrix_data[1][0], Eq(1));
  ASSERT_THAT(matrix_data[1][1], Eq(2));
  ASSERT_THAT(matrix_data[2].size(), Eq(2));
  ASSERT_THAT(matrix_data[2][0], Eq(2));
  ASSERT_THAT(matrix_data[2][1], Eq(1));
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
}  // namespace mtxconversion
