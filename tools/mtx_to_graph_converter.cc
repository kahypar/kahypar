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

#include <iostream>
#include <string>
#include <algorithm>

#include "kahypar/macros.h"
#include "tools/mtx_to_hgr_conversion.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .mtx file specified" << std::endl;
  }
  std::string mtx_filename(argv[1]);
  std::string hgr_filename(mtx_filename + ".hgr");
  std::cout << "Converting MTX matrix " << mtx_filename << " to HGR hypergraph format: "
            << hgr_filename << "..." << std::endl;

	mtxconversion::Matrix matrix = mtxconversion::readMatrix(mtx_filename);

	// Convert matrix into an undirected graph
	mtxconversion::MatrixData& data = matrix.data;
	for ( int u = 0; u < static_cast<int>(data.entries.size()); ++u ) {
		for ( size_t i = 0; i < data.entries[u].size(); ++i ) {
			int v = data.entries[u][i];
			if ( u != v ) {
				data.entries[v].push_back(u);
			} else {
				std::swap(data.entries[u][i--], data.entries[u].back());
				data.entries[u].pop_back();
			}
		}
	}

	// Remove duplicated edges
	size_t num_edges = 0;
	for ( size_t i = 0; i < data.entries.size(); ++i ) {
		std::sort(data.entries[i].begin(), data.entries[i].end());
		auto ptr = std::unique(data.entries[i].begin(), data.entries[i].end());
		if ( ptr != data.entries[i].end() ) {
    	data.entries[i].erase(ptr, data.entries[i].end());
		}
		num_edges += data.entries[i].size();
	}
	std::cout << "Num Nodes = " << data.entries.size() << ", Num Edges = " << (num_edges/2) << std::endl;

	std::ofstream out_stream(hgr_filename.c_str());
	out_stream << (num_edges / 2) << " " << data.entries.size() << std::endl;
	for ( size_t i = 0; i < data.entries.size(); ++i ) {
		int u = i;
		for ( const int v : data.entries[u] ) {
			if ( u < v ) {
				out_stream << ( u + 1 ) << " " << ( v + 1 ) << std::endl;
			}
		}
	}
  out_stream.close();

  std::cout << " ... done!" << std::endl;
  return 0;
}
