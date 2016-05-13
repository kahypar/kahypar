/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "lib/macros.h"
#include "lib/definitions.h"
#include "lib/io/HypergraphIO.h"

using defs::Hypergraph;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "No .hgr file specified" << std::endl;
  }
  std::string hgr_filename(argv[1]);
  std::string graphml_filename(hgr_filename + ".graphml");

  Hypergraph hypergraph(io::createHypergraphFromFile(hgr_filename,2));

  std::ofstream out_stream(graphml_filename.c_str());

  out_stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
             << " <graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\" xmlns:java=\"http://www.yworks.com/xml/yfiles-common/1.0/java\""
             << " xmlns:sys=\"http://www.yworks.com/xml/yfiles-common/markup/primitives/2.0\""
             << " xmlns:x=\"http://www.yworks.com/xml/yfiles-common/markup/2.0\""
             << " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
             << " xmlns:y=\"http://www.yworks.com/xml/graphml\" xmlns:yed=\"http://www.yworks.com/xml/yed/3\""
             << " xsi:schemaLocation=\"http://graphml.graphdrawing.org/xmlns http://www.yworks.com/xml/schema/graphml/1.1/ygraphml.xsd\">"
      << std::endl;



  out_stream << "<key for=\"node\" id=\"d3\" yfiles.type=\"nodegraphics\"/>" << std::endl;

  out_stream << "<graph id=\"G\" edgedefault=\"undirected\">" << std::endl;
  for (const defs::HypernodeID hn : hypergraph.nodes()){
    out_stream << "<node id=\"n" << hn << "\">"  << std::endl;
    out_stream << "<data key=\"d3\">"<< std::endl;
    out_stream << "<y:ShapeNode><y:Fill color=\"#FA0000\" transparent=\"false\"/></y:ShapeNode>" << std::endl;
    out_stream << "</data>" << std::endl;
    out_stream << "</node>" << std::endl;
  }

  HyperedgeID edge_id = 0;
  for (const defs::HyperedgeID he : hypergraph.edges()) {
    const HyperedgeID he_id = hypergraph.numNodes() + he;
    out_stream << "<node id=\"n" << he_id << "\">"  << std::endl;
    out_stream << "<data key=\"d3\">"<< std::endl;
    out_stream << "<y:ShapeNode><y:Fill color=\"#000CFA\" transparent=\"false\"/></y:ShapeNode>" << std::endl;
    out_stream << "</data>" << std::endl;
    out_stream << "</node>"   << std::endl;
    for (const defs::HypernodeID pin : hypergraph.pins(he)) {
      out_stream << "<edge id=\"e" << edge_id++  << "\" source=\"n" << pin << "\" target=\"n" << he_id << "\"/>" << std::endl;
    }
  }

  out_stream << "</graph>" << std::endl;
  out_stream << "</graphml>" << std::endl;
  out_stream.close();
  std::cout << " ... done!" << std::endl;
  return 0;
}
