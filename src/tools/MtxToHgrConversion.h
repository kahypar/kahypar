/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_TOOLS_MTXTOHGRCONVERSION_H_
#define SRC_TOOLS_MTXTOHGRCONVERSION_H_
#include <fstream>
#include <string>
#include <vector>

namespace mtxconversion {
enum class MatrixFormat { COORDINATE };
enum class MatrixSymmetry { GENERAL, SYMMETRIC };

struct MatrixInfo {
  MatrixFormat format;
  MatrixSymmetry symmetry;
  int num_rows;
  int num_columns;
  int num_entries;
};

typedef std::vector<std::vector<int> > MatrixData;

MatrixInfo parseHeader(std::ifstream& file);
void parseDimensionInformation(std::ifstream& file, MatrixInfo& info);
void parseMatrixEntries(std::ifstream& file, const MatrixInfo& info, MatrixData& matrix_data);
void parseCoordinateMatrixEntries(std::ifstream& file, const MatrixInfo& info, MatrixData& matrix_data);
void writeMatrixInHgrFormat(const MatrixInfo& info, const MatrixData& matrix_data, const std::string& filename);
void convertMtxToHgr(const std::string& matrix_filename, const std::string& hypergraph_filename);
} // namespace mtxconversion

#endif  // SRC_TOOLS_MTXTOHGRCONVERSION_H_
