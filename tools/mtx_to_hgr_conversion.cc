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

#include <sstream>
#include <string>

#include "kahypar/macros.h"
#include "tools/mtx_to_hgr_conversion.h"

namespace mtxconversion {
static constexpr bool debug = false;

MatrixInfo parseHeader(std::ifstream& file) {
  std::string line;
  std::getline(file, line);
  std::istringstream sstream(line);
  std::string matrix_market, object, matrix_format, data_format, symmetry;
  sstream >> matrix_market >> object >> matrix_format >> data_format >> symmetry;
  LOG << "matrix_market=" << matrix_market;
  LOG << "object=" << object;
  LOG << "matrix_format=" << matrix_format;
  LOG << "data_format=" << data_format;
  LOG << "symmetry=" << symmetry;
  ALWAYS_ASSERT(matrix_market == "%%MatrixMarket", "Invalid Header " << V(matrix_market));

  MatrixInfo info;
  if (matrix_format == "coordinate") {
    info.format = MatrixFormat::COORDINATE;
  } else {
    std::cout << "Matrix format " << matrix_format << " is not supported" << std::endl;
    exit(-1);
  }
  if (object == "matrix") {
    info.object = MatrixObjectType::MATRIX;
  } else if (object == "weightedmatrix") {
    info.object = MatrixObjectType::WEIGHTED_MATRIX;
  } else {
    std::cout << "Object type " << object << " is not supported" << std::endl;
    exit(-1);
  }
  if (symmetry == "general") {
    info.symmetry = MatrixSymmetry::GENERAL;
  } else if (symmetry == "symmetric") {
    info.symmetry = MatrixSymmetry::SYMMETRIC;
  } else {
    std::cout << "Symmetry format " << symmetry << " is not supported" << std::endl;
    exit(-1);
  }
  return info;
}

void parseDimensionInformation(std::ifstream& file, MatrixInfo& info) {
  std::string line;
  std::getline(file, line);
  // skip any comments
  while (line[0] == '%') {
    std::getline(file, line);
  }
  std::istringstream sstream(line);
  sstream >> info.num_rows >> info.num_columns >> info.num_entries;
  LOG << "num_rows=" << info.num_rows;
  LOG << "num_columns=" << info.num_columns;
  LOG << "num_entries=" << info.num_entries;
}

void parseMatrixEntries(std::ifstream& file, MatrixInfo& info, MatrixData& matrix_data) {
  // row-net representation
  matrix_data.entries.resize(info.num_rows);

  switch (info.format) {
    case MatrixFormat::COORDINATE:
      parseCoordinateMatrixEntries(file, info, matrix_data);
      break;
  }
  switch (info.object) {
    case MatrixObjectType::MATRIX:
      break;
    case MatrixObjectType::WEIGHTED_MATRIX:
      parseWeights(file, info, matrix_data);
      break;
  }
}

void parseCoordinateMatrixEntries(std::ifstream& file, MatrixInfo& info,
                                  MatrixData& matrix_data) {
  std::string line;
  int row = -1;
  int column = -1;
  for (int i = 0; i < info.num_entries; ++i) {
    std::getline(file, line);
    DBG << line;
    std::istringstream line_stream(line);
    line_stream >> row >> column;

    // indices start at 1
    --column;
    --row;
    matrix_data.entries[row].push_back(column);
    DBG << "_matrix_data[" << row << "].push_back(" << column << ")";

    if (info.symmetry == MatrixSymmetry::SYMMETRIC && row != column) {
      matrix_data.entries[column].push_back(row);
      DBG << "_matrix_data[" << column << "].push_back(" << row << ")";
    }
  }

  int num_empty_hyperedges = 0;
  for (const auto& hyperedge : matrix_data.entries) {
    if (hyperedge.size() == 0) {
      ++num_empty_hyperedges;
    }
  }
  if (num_empty_hyperedges > 0) {
    std::cout << "WARNING: matrix contains " << num_empty_hyperedges << " empty hyperedges"
              << std::endl;
    std::cout << "Number of hyperedges in hypergraph will be adjusted!"
              << std::endl;
    info.num_rows -= num_empty_hyperedges;
  }
}

void parseWeights(std::ifstream& file, MatrixInfo& info, MatrixData& matrix_data) {
  ALWAYS_ASSERT(info.object == MatrixObjectType::WEIGHTED_MATRIX, "Matrix is not weighted");
  std::string line;
  int num_weights = 0;
  for (int i = 0; i < info.num_columns; ++i) {
    std::getline(file, line);
    DBG << line;
    std::istringstream line_stream(line);
    int weight;
    line_stream >> weight;

    ++num_weights;
    matrix_data.weights.push_back(weight);
    DBG << "weights.push_back(" << weight << ")";
  }
}

void addRowNetWeights(MatrixInfo& info, MatrixData& matrix_data) {
  if (info.object == MatrixObjectType::WEIGHTED_MATRIX) {
    std::cout << "Warning: Matrix is already weighted. Overwriting with node degrees." << std::endl;
  }

  matrix_data.weights.resize(info.num_columns);
  for (const auto& hyperedge : matrix_data.entries) {
    if (hyperedge.size() != 0) {
      for (auto pin_iter = hyperedge.begin(); pin_iter != hyperedge.end(); ++pin_iter) {
        matrix_data.weights.at(*pin_iter)++;
      }
    }
  }
  info.object = MatrixObjectType::WEIGHTED_MATRIX;
}

void writeMatrixInHgrFormat(const MatrixInfo& info, const MatrixData& matrix_data,
                            const std::string& filename) {
  std::ofstream out_stream(filename.c_str());
  bool weighted = (info.object == MatrixObjectType::WEIGHTED_MATRIX);
  out_stream << info.num_rows << " " << info.num_columns;
  if (weighted) {
    out_stream << " 10";
  }
  out_stream << std::endl;
  for (const auto& hyperedge : matrix_data.entries) {
    if (hyperedge.size() != 0) {
      for (auto pin_iter = hyperedge.begin(); pin_iter != hyperedge.end(); ++pin_iter) {
        // ids start at 1
        out_stream << *pin_iter + 1;
        if (pin_iter + 1 != hyperedge.end()) {
          out_stream << " ";
        }
      }
      out_stream << std::endl;
    }
  }
  if (weighted) {
    for (const auto& weight : matrix_data.weights) {
      out_stream << weight << std::endl;
    }
  }
  out_stream.close();
}

void convertMtxToHgr(const std::string& matrix_filename, const std::string& hypergraph_filename) {
  Matrix matrix = readMatrix(matrix_filename);
  writeMatrixInHgrFormat(matrix.info, matrix.data, hypergraph_filename);
}

void convertMtxToWeightedHgr(const std::string& matrix_filename, const std::string& hypergraph_filename) {
  Matrix matrix = readMatrix(matrix_filename);
  addRowNetWeights(matrix.info, matrix.data);
  writeMatrixInHgrFormat(matrix.info, matrix.data, hypergraph_filename);
}

void convertMtxToHgrForNonsymmetricParallelSPM(const std::string& matrix_filename,
                                               const std::string& hypergraph_filename) {
  std::ifstream mtx_file(matrix_filename);

  Matrix matrix;

  matrix.info = parseHeader(mtx_file);
  parseDimensionInformation(mtx_file, matrix.info);

  ALWAYS_ASSERT(matrix.info.format == MatrixFormat::COORDINATE, "Unknown Format");
  ALWAYS_ASSERT(matrix.info.object == MatrixObjectType::MATRIX, "Unsupported object type");

  // Here, we actually use the column-net model
  std::vector<std::vector<int> >& entries = matrix.data.entries;
  entries.resize(matrix.info.num_columns);

  std::vector<int> vertex_weights(matrix.info.num_rows, 0);

  std::string line;
  int row = -1;
  int column = -1;
  for (int i = 0; i < matrix.info.num_entries; ++i) {
    std::getline(mtx_file, line);
    DBG << line;
    std::istringstream line_stream(line);
    line_stream >> row >> column;

    // indices start at 1
    --column;
    --row;
    entries[column].push_back(row);
    DBG << "_matrix_data[" << column << "].push_back(" << row << ")";

    // Vertex weights in column-net model -> c(v) = |{a_ij != 0}|
    vertex_weights[row] += 1;

    if (matrix.info.symmetry == MatrixSymmetry::SYMMETRIC && row != column) {
      entries[row].push_back(column);
      vertex_weights[column] += 1;
      DBG << "_matrix_data[" << row << "].push_back(" << column << ")";
    }
  }
  mtx_file.close();

  int num_empty_hyperedges = 0;
  for (const auto& hyperedge : matrix.data.entries) {
    if (hyperedge.size() == 0) {
      ++num_empty_hyperedges;
    }
  }
  if (num_empty_hyperedges > 0) {
    std::cout << "WARNING: matrix contains " << num_empty_hyperedges << " empty hyperedges"
              << std::endl;
  }

  // Nonsymmetric partitioning using column-net model:
  // We have to add one vertex to each net/column that represents the dependency on x_j.
  // |V| = num_rows + num_columns
  // |E| = num_columns - num_empty_hyperedges
  // fmt = 10, since we got vertex weights

  std::ofstream out_stream(hypergraph_filename.c_str());
  out_stream << (matrix.info.num_columns - num_empty_hyperedges) << " "
             << (matrix.info.num_rows + matrix.info.num_columns) << " " << 10 << std::endl;

  for (int i = 0; i < matrix.info.num_columns; ++i) {
    const auto& hyperedge = entries[i];
    if (hyperedge.size() != 0) {
      // add xj vertex
      out_stream << matrix.info.num_rows + i + 1 << " ";
      for (auto pin_iter = hyperedge.begin(); pin_iter != hyperedge.end(); ++pin_iter) {
        // ids start at 1
        out_stream << *pin_iter + 1;
        if (pin_iter + 1 != hyperedge.end()) {
          out_stream << " ";
        }
      }
      out_stream << std::endl;
    }
  }

  // now we print the vertex weights for row-vertices
  for (const auto& weight : vertex_weights) {
    out_stream << weight << std::endl;
  }

  // unit vertex weights for x_j vertices
  for (int i = 0; i < matrix.info.num_columns; ++i) {
    out_stream << 1 << std::endl;
  }

  out_stream.close();
}

Matrix readMatrix(const std::string& matrix_filename) {
  std::ifstream mtx_file(matrix_filename);
  MatrixInfo info = parseHeader(mtx_file);

  parseDimensionInformation(mtx_file, info);
  MatrixData matrix_data;
  parseMatrixEntries(mtx_file, info, matrix_data);
  mtx_file.close();
  const Matrix matrix = { info, matrix_data };
  return matrix;
}
}  // namespace mtxconversion
