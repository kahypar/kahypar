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

#include <fstream>
#include <string>
#include <vector>

namespace mtxconversion {
enum class MatrixFormat { COORDINATE };
enum class MatrixSymmetry { GENERAL, SYMMETRIC };

struct MatrixInfo {
  MatrixFormat format = MatrixFormat::COORDINATE;
  MatrixSymmetry symmetry = MatrixSymmetry::GENERAL;
  int num_rows = 0;
  int num_columns = 0;
  int num_entries = 0;
};

using MatrixData = std::vector<std::vector<int> >;

struct Matrix {
  MatrixInfo info { };
  MatrixData data { };
  Matrix(const MatrixInfo& matrix_info, const MatrixData& matrix_data) :
    info(matrix_info),
    data(matrix_data) { }
  Matrix() { }
};

MatrixInfo parseHeader(std::ifstream& file);
void parseDimensionInformation(std::ifstream& file, MatrixInfo& info);
void parseMatrixEntries(std::ifstream& file, MatrixInfo& info, MatrixData& matrix_data);
void parseCoordinateMatrixEntries(std::ifstream& file, MatrixInfo& info, MatrixData& matrix_data);
void writeMatrixInHgrFormat(const MatrixInfo& info, const MatrixData& matrix_data,
                            const std::string& filename);
void convertMtxToHgr(const std::string& matrix_filename,
                     const std::string& hypergraph_filename);
void convertMtxToHgrForNonsymmetricParallelSPM(const std::string& matrix_filename,
                                               const std::string& hypergraph_filename);

Matrix readMatrix(const std::string& matrix_filename);
}  // namespace mtxconversion
