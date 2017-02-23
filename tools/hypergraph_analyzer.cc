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

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partition/metrics.h"

using namespace kahypar;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "Wrong number of arguments!" << std::endl;
    std::cout << "Usage: HypergraphAnalyzer <hypergraph.hgr>" << std::endl;
    return -1;
  }

  std::string graph_filename(argv[1]);

  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HyperedgeWeightVector hyperedge_weights;
  HypernodeWeightVector hypernode_weights;

  kahypar::io::readHypergraphFile(graph_filename, num_hypernodes, num_hyperedges,
                                  index_vector, edge_vector, &hyperedge_weights, &hypernode_weights);
  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);

  std::map<HyperedgeID, HyperedgeID> node_degrees;
  std::map<HypernodeID, HypernodeID> edge_sizes;

  for (const auto& hn : hypergraph.nodes()) {
    ++node_degrees[hypergraph.nodeDegree(hn)];
  }

  for (const auto& he : hypergraph.edges()) {
    ++edge_sizes[hypergraph.edgeSize(he)];
  }

  std::string graph_name = graph_filename.substr(graph_filename.find_last_of("/") + 1);
  std::string hn_output = graph_name + "_hn_degrees.csv";
  std::string he_output = graph_name + "_he_sizes.csv";
  std::ofstream out_stream_hns(hn_output.c_str(), std::ofstream::out);
  std::ofstream out_stream_hes(he_output.c_str(), std::ofstream::out);

  out_stream_hns << "\"degree\"" << "," << "\"count\"" << std::endl;
  for (const auto& deg : node_degrees) {
    out_stream_hns << deg.first << ", " << deg.second << std::endl;
  }

  out_stream_hes << "\"edgesize\"" << "," << "\"count\"" << std::endl;
  for (const auto& size : edge_sizes) {
    out_stream_hes << size.first << ", " << size.second << std::endl;
  }

  out_stream_hns.flush();
  out_stream_hes.flush();

  return 0;
}
