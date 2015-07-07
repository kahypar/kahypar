/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include "tools/MtxToHgrConversion.h"

#include <sstream>
#include <string>

#include "lib/macros.h"

namespace mtxconversion {
MatrixInfo parseHeader(std::ifstream& file) {
  std::string line;
  std::getline(file, line);
  std::istringstream sstream(line);
  std::string matrix_market, object, matrix_format, data_format, symmetry;
  sstream >> matrix_market >> object >> matrix_format >> data_format >> symmetry;
  LOG("matrix_market=" << matrix_market);
  LOG("object=" << object);
  LOG("matrix_format=" << matrix_format);
  LOG("data_format=" << data_format);
  LOG("symmetry=" << symmetry);
  ALWAYS_ASSERT(matrix_market == "%%MatrixMarket", "Invalid Header " << V(matrix_market));
  ALWAYS_ASSERT(object == "matrix", "Invalid object type " << V(object));

  MatrixInfo info;
  if (matrix_format == "coordinate") {
    info.format = MatrixFormat::COORDINATE;
  } else {
    std::cout << "Matrix format " << matrix_format << " is not supported" << std::endl;
    exit(-1);
  }
  if (symmetry == "general") {
    info.symmetry = MatrixSymmetry::GENERAL;
  } else if (symmetry == "symmetric") {
    info.symmetry = MatrixSymmetry::SYMMETRIC;
  } else {
    std::cout << "Symmetry format " << symmetry << " is not supported" << std::endl;
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
  LOG("num_rows=" << info.num_rows);
  LOG("num_columns=" << info.num_columns);
  LOG("num_entries=" << info.num_entries);
}

void parseMatrixEntries(std::ifstream& file, MatrixInfo& info, MatrixData& matrix_data) {
  // row-net representation
  matrix_data.resize(info.num_rows);

  switch (info.format) {
    case MatrixFormat::COORDINATE:
      parseCoordinateMatrixEntries(file, info, matrix_data);
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
    DBG(false, line);
    std::istringstream line_stream(line);
    line_stream >> row >> column;

    // indices start at 1
    --column;
    --row;
    matrix_data[row].push_back(column);
    DBG(false, "_matrix_data[" << row << "].push_back(" << column << ")");

    if (info.symmetry == MatrixSymmetry::SYMMETRIC && row != column) {
      matrix_data[column].push_back(row);
      DBG(false, "_matrix_data[" << column << "].push_back(" << row << ")");
    }
  }

  int num_empty_hyperedges = 0;
  for (auto hyperedge : matrix_data) {
    if (hyperedge.size() == 0) {
      ++num_empty_hyperedges;
    }
  }
  std::cout << "WARNING: matrix contains " << num_empty_hyperedges << " empty hyperedges"
            << std::endl;
  std::cout << "Number of hyperedges in hypergraph will be adjusted!"
            << std::endl;
  info.num_rows -= num_empty_hyperedges;
}

void writeMatrixInHgrFormat(const MatrixInfo& info, const MatrixData& matrix_data,
                            const std::string& filename) {


  std::ofstream out_stream(filename.c_str());
  out_stream << info.num_rows << " " << info.num_columns << std::endl;
  for (auto hyperedge : matrix_data) {
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
  out_stream.close();
}

void convertMtxToHgr(const std::string& matrix_filename, const std::string& hypergraph_filename) {
  std::ifstream mtx_file(matrix_filename);
  MatrixInfo info = parseHeader(mtx_file);

  parseDimensionInformation(mtx_file, info);
  MatrixData matrix_data;
  parseMatrixEntries(mtx_file, info, matrix_data);
  mtx_file.close();
  writeMatrixInHgrFormat(info, matrix_data, hypergraph_filename);
}
}  // namespace mtxconversion
