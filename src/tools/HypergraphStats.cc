/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

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
  for (auto hn : hypergraph.nodes()) {
    max_hn_degree = std::max(max_hn_degree, hypergraph.nodeDegree(hn));
  }


  std::string graph_name = graph_filename.substr(graph_filename.find_last_of("/") + 1);
  std::ofstream out_stream(stats_filename.c_str(), std::ofstream::app);

  bool contains_single_node_hes = false;
  HyperedgeID single_node_he = -1;
  for (auto he : hypergraph.edges()) {
    if (hypergraph.edgeSize(he) == 1) {
      contains_single_node_hes = true;
      single_node_he = he;
      break;
    }
  }

  out_stream << "RESULT graph=" << graph_name
             << " containsSingleNodeHEs=" << contains_single_node_hes
             << " HE=" << single_node_he
             << " HNs=" << num_hypernodes
             << " HEs=" << num_hyperedges
             << " pins=" << edge_vector.size()
             << " avgHEsize=" << metrics::avgHyperedgeDegree(hypergraph)
             << " heSize90thPercentile=" << metrics::hyperedgeSizePercentile(hypergraph, 90)
             << " heSize95thPercentile=" << metrics::hyperedgeSizePercentile(hypergraph, 95)
             << " rank=" << metrics::rank(hypergraph)
             << " avgHNdegree=" << metrics::avgHypernodeDegree(hypergraph)
             << " hnDegree90thPercentile=" << metrics::hypernodeDegreePercentile(hypergraph, 90)
             << " hnDegree95thPercentile=" << metrics::hypernodeDegreePercentile(hypergraph, 95)
             << " maxHnDegree=" << max_hn_degree
             << " density=" << static_cast<double>(num_hyperedges) / num_hypernodes
             << std::endl;
  out_stream.flush();

  return 0;
}
