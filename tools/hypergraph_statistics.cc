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

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

#include "kahypar/definitions.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/io/partitioning_output.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/utils/math.h"

using namespace kahypar;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    std::cout << "Wrong number of arguments!" << std::endl;
    std::cout << "Usage: hypergraph_stats <hypergraph.hgr> <statsfile.txt>" << std::endl;
  }

  std::string graph_filename(argv[1]);
  std::string stats_filename(argv[2]);

  HypernodeID num_hypernodes;
  HyperedgeID num_hyperedges;
  HyperedgeIndexVector index_vector;
  HyperedgeVector edge_vector;
  HyperedgeWeightVector hyperedge_weights;
  HypernodeWeightVector hypernode_weights;

  kahypar::io::readHypergraphFile(graph_filename, num_hypernodes, num_hyperedges,
                                  index_vector, edge_vector, &hyperedge_weights, &hypernode_weights);
  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);

  HyperedgeID max_hn_degree = 0;
  HyperedgeID min_hn_degree = std::numeric_limits<HyperedgeID>::max();
  double avg_hn_degree = kahypar::metrics::avgHypernodeDegree(hypergraph);
  double sd_hn_degree = 0.0;
  std::vector<HyperedgeID> hn_degrees;
  hn_degrees.reserve(hypergraph.currentNumNodes());
  for (const auto& hn : hypergraph.nodes()) {
    hn_degrees.push_back(hypergraph.nodeDegree(hn));
    max_hn_degree = std::max(max_hn_degree, hypergraph.nodeDegree(hn));
    min_hn_degree = std::min(min_hn_degree, hypergraph.nodeDegree(hn));
    sd_hn_degree += std::pow(hypergraph.nodeDegree(hn), 2);
  }

  sd_hn_degree = std::sqrt((sd_hn_degree / num_hypernodes) - std::pow(avg_hn_degree, 2));

  std::string graph_name = graph_filename.substr(graph_filename.find_last_of("/") + 1);
  std::ofstream out_stream(stats_filename.c_str(), std::ofstream::app);

  HyperedgeID num_single_node_hes = 0;
  HypernodeID max_he_size = 0;
  HypernodeID min_he_size = std::numeric_limits<HypernodeID>::max();
  double avg_he_size = kahypar::metrics::avgHyperedgeDegree(hypergraph);
  double sd_he_size = 0.0;
  std::vector<HypernodeID> he_sizes;
  he_sizes.reserve(hypergraph.currentNumEdges());
  for (const auto& he : hypergraph.edges()) {
    if (hypergraph.edgeSize(he) == 1) {
      ++num_single_node_hes;
    }
    he_sizes.push_back(hypergraph.edgeSize(he));
    max_he_size = std::max(max_he_size, hypergraph.edgeSize(he));
    min_he_size = std::min(min_he_size, hypergraph.edgeSize(he));
    sd_he_size += std::pow(hypergraph.edgeSize(he), 2);
  }

  sd_he_size = std::sqrt((sd_he_size / num_hyperedges) - std::pow(avg_he_size, 2));

  std::sort(he_sizes.begin(), he_sizes.end());
  std::sort(hn_degrees.begin(), hn_degrees.end());

  auto he_size_quartiles = kahypar::math::firstAndThirdQuartile(he_sizes);
  auto hn_deg_quartiles = kahypar::math::firstAndThirdQuartile(hn_degrees);

  out_stream << "RESULT graph=" << graph_name
             << " HNs=" << num_hypernodes
             << " HEs=" << num_hyperedges
             << " pins=" << edge_vector.size()
             << " numSingleNodeHEs=" << num_single_node_hes
             << " avgHEsize=" << avg_he_size
             << " sdHEsize=" << sd_he_size
             << " minHEsize=" << min_he_size
             << " heSize90thPercentile=" << kahypar::metrics::hyperedgeSizePercentile(hypergraph, 90)
             << " Q1HEsize=" << he_size_quartiles.first
             << " medHEsize=" << kahypar::math::median(he_sizes)
             << " Q3HEsize=" << he_size_quartiles.second
             << " maxHEsize=" << max_he_size
             << " avgHNdegree=" << avg_hn_degree
             << " sdHNdegree=" << sd_hn_degree
             << " minHnDegree=" << min_hn_degree
             << " hnDegree90thPercentile=" << kahypar::metrics::hypernodeDegreePercentile(hypergraph, 90)
             << " maxHnDegree=" << max_hn_degree
             << " Q1HNdegree=" << hn_deg_quartiles.first
             << " medHNdegree=" << kahypar::math::median(hn_degrees)
             << " Q3HNdegree=" << hn_deg_quartiles.second
             << " density=" << static_cast<double>(num_hyperedges) / num_hypernodes
             << std::endl;
  out_stream.flush();

  return 0;
}
