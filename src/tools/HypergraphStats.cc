/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <iostream>
#include <string>
#include <fstream>

#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"
#include "partition/Metrics.h"

using defs::HypernodeID;
using defs::HyperedgeID;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::Hypergraph;

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

  io::readHypergraphFile(graph_filename, num_hypernodes, num_hyperedges,
                         index_vector, edge_vector);
  Hypergraph hypergraph(num_hypernodes, num_hyperedges, index_vector, edge_vector);

  std::string graph_name = graph_filename.substr(graph_filename.find_last_of("/") + 1);
  std::ofstream out_stream(stats_filename.c_str(), std::ofstream::app);

  out_stream << "graph=" << graph_name
             << " HNs=" << num_hypernodes
             << " HEs=" << num_hyperedges
             << " pins=" << edge_vector.size()
             << " avgHEsize=" << metrics::avgHyperedgeDegree(hypergraph)
             << " heSize90thPercentile=" << metrics::hyperedgeSizePercentile(hypergraph,90)
             << " heSize95thPercentile=" << metrics::hyperedgeSizePercentile(hypergraph,95)
             << " rank=" << metrics::rank(hypergraph)
             << " avgHNdegree=" << metrics::avgHypernodeDegree(hypergraph)
             << " hnDegree90thPercentile=" << metrics::hypernodeDegreePercentile(hypergraph,90)
             << " hnDegree95thPercentile=" << metrics::hypernodeDegreePercentile(hypergraph,95)
             << " density=" << static_cast<double>(num_hyperedges) / num_hypernodes
             << std::endl;
  out_stream.flush();

  return 0;
}
