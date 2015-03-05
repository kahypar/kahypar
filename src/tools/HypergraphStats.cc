/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "partition/Metrics.h"

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::Hypergraph;
using defs::HypernodeWeightVector;
using defs::HyperedgeWeightVector;

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

  io::readHypergraphFile(graph_filename, num_hypernodes, num_hyperedges,
                         index_vector, edge_vector, &hyperedge_weights, &hypernode_weights);
  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);

  HyperedgeID max_hn_degree = 0;
  HyperedgeID min_hn_degree = std::numeric_limits<HyperedgeID>::max();
  double avg_hn_degree = metrics::avgHypernodeDegree(hypergraph);
  double sd_hn_degree = 0.0;
  for (auto hn : hypergraph.nodes()) {
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
  double avg_he_size = metrics::avgHyperedgeDegree(hypergraph);
  double sd_he_size = 0.0;
  for (auto he : hypergraph.edges()) {
    if (hypergraph.edgeSize(he) == 1) {
      ++num_single_node_hes;
    }
    max_he_size = std::max(max_he_size, hypergraph.edgeSize(he));
    min_he_size = std::min(min_he_size, hypergraph.edgeSize(he));
    sd_he_size += std::pow(hypergraph.edgeSize(he), 2);
  }

  sd_he_size = std::sqrt((sd_he_size / num_hyperedges) - std::pow(avg_he_size, 2));

  out_stream << "RESULT graph=" << graph_name
  << " HNs=" << num_hypernodes
  << " HEs=" << num_hyperedges
  << " pins=" << edge_vector.size()
  << " numSingleNodeHEs=" << num_single_node_hes
  << " avgHEsize=" << avg_he_size
  << " sdHEsize=" << sd_he_size
  << " minHEsize=" << min_he_size
  << " heSize90thPercentile=" << metrics::hyperedgeSizePercentile(hypergraph, 90)
  << " maxHEsize=" << max_he_size
  << " avgHNdegree=" << avg_hn_degree
  << " sdHNdegree=" << sd_hn_degree
  << " minHnDegree=" << min_hn_degree
  << " hnDegree90thPercentile=" << metrics::hypernodeDegreePercentile(hypergraph, 90)
  << " maxHnDegree=" << max_hn_degree
  << " density=" << static_cast<double>(num_hyperedges) / num_hypernodes
  << std::endl;
  out_stream.flush();

  return 0;
}
