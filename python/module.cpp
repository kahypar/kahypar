
/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
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

#include <pybind11/pybind11.h>

#include <pybind11/stl.h>

#include <string>
#include <vector>

#include "kahypar/definitions.h"

#include "kahypar/partition/context.h"
#include "kahypar/io/hypergraph_io.h"
#include "kahypar/partitioner_facade.h"
#include "kahypar/application/command_line_options.h"
#include "kahypar/datastructure/connectivity_sets.h"
#include "kahypar/partition/metrics.h"

void hello(const std::string& input) {
  std::cout << input << std::endl;
}

void partition(kahypar::Hypergraph& hypergraph,
               kahypar::Context& context) {
  kahypar::PartitionerFacade().partition(hypergraph, context);
}


namespace py = pybind11;

PYBIND11_MODULE(kahypar, m) {
  using kahypar::Hypergraph;
  using kahypar::HypernodeID;
  using kahypar::HyperedgeID;
  using kahypar::HyperedgeIndexVector;
  using kahypar::HyperedgeVector;
  using kahypar::HyperedgeWeightVector;
  using kahypar::HypernodeWeightVector;
  using kahypar::PartitionID;
  using ConnectivitySet = typename kahypar::ds::ConnectivitySets<PartitionID, HyperedgeID>::ConnectivitySet;

  py::class_<Hypergraph>(
      m, "Hypergraph")
      .def(py::init<const HypernodeID,
           const HyperedgeID,
           const HyperedgeIndexVector,
           const HyperedgeVector,
           const PartitionID>(),R"pbdoc(
Construct an unweighted hypergraph.

:param HypernodeID num_nodes: Number of nodes
:param HyperedgeID num_edges: Number of hyperedges
:param HyperedgeIndexVector index_vector: Starting indices for each hyperedge
:param HyperedgeVector edge_vector: Vector containing all hyperedges
:param PartitionID k: Number of blocks in which the hypergraph should be partitioned

          )pbdoc",
           py::arg("num_nodes"),
           py::arg("num_edges"),
           py::arg("index_vector"),
           py::arg("edge_vector"),
           py::arg("k"))
       .def(py::init<const HypernodeID,
           const HyperedgeID,
           const HyperedgeIndexVector,
           const HyperedgeVector,
           const PartitionID,
           const HyperedgeWeightVector,
           const HypernodeWeightVector>(),R"pbdoc(
Construct a hypergraph with node and edge weights.

If only one type of weights is required, the other argument has to be an empty list.

:param HypernodeID num_nodes: Number of nodes
:param HyperedgeID num_edges: Number of hyperedges
:param HyperedgeIndexVector index_vector: Starting indices for each hyperedge
:param HyperedgeVector edge_vector: Vector containing all hyperedges
:param PartitionID k: Number of blocks in which the hypergraph should be partitioned
:param HyperedgeWeightVector edge_weights: Weights of all hyperedges
:param HypernodeWeightVector node_weights: Weights of all hypernodes

          )pbdoc",
           py::arg("num_nodes"),
           py::arg("num_edges"),
           py::arg("index_vector"),
           py::arg("edge_vector"),
           py::arg("k"),
           py::arg("edge_weights"),
           py::arg("node_weights"))
      .def("printGraphState", &Hypergraph::printGraphState,
           "Print the hypergraph state (for debugging purposes)")
      .def("nodeDegree", &Hypergraph::nodeDegree,
           "Get the degree of the node",
           py::arg("node"))
      .def("edgeSize", &Hypergraph::edgeSize,
           "Get the size of the hyperedge",
           py::arg("hyperedge"))
      .def("nodeWeight", &Hypergraph::nodeWeight,
           "Get the weight of the node",
           py::arg("node"))
      .def("edgeWeight", &Hypergraph::edgeWeight,
           "Get the weight of the hyperedge",
           py::arg("hyperedge"))
      .def("blockID", &Hypergraph::partID,
           "Get the block of the node in the current hypergraph partition (before partitioning: -1)",
           py::arg("node"))
      .def("numNodes", &Hypergraph::initialNumNodes,
           "Get the number of nodes")
      .def("numEdges", &Hypergraph::initialNumEdges,
           "Get the number of hyperedges")
      .def("numPins", &Hypergraph::initialNumPins,
           "Get the number of pins")
      .def("numBlocks", &Hypergraph::k,
           "Get the number of blocks")
      .def("numPinsInBlock", &Hypergraph::pinCountInPart,
           "Get the number of pins of the hyperedge that are assigned to corresponding block",
           py::arg("hyperedge"),
           py::arg("block"))
      .def("connectivity", &Hypergraph::connectivity,
           "Get the connecivity of the hyperedge (i.e., the number of blocks which contain at least one pin)",
           py::arg("hyperedge"))
      .def("connectivitySet", &Hypergraph::connectivitySet, py::return_value_policy::reference_internal,
           "Get the connectivity set of the hyperedge",
           py::arg("hyperedge"))
      .def("communities", &Hypergraph::communities,
           "Get the community structure")
      .def("blockWeight", &Hypergraph::partWeight,
      "Get the weight of the block",
           py::arg("block"))
      .def("blockSize", &Hypergraph::partSize,
           "Get the number of vertices in the block",
           py::arg("block"))
      .def("reset", &Hypergraph::reset,
           "Reset the hypergraph to its initial state")
      .def("fixNodeToBlock", &Hypergraph::setFixedVertex,
        "Fix node to the cooresponding block",
        py::arg("node"), py::arg("block"))
      .def("numFixedNodes", &Hypergraph::numFixedVertices,
        "Get the number of fixed nodes in the hypergraph")
      .def("containsFixedNodex", &Hypergraph::containsFixedVertices,
        "Return true if the hypergraph contains nodes fixed to a specific block")
      .def("isFixedNode", &Hypergraph::isFixedVertex,
        "Return true if the node is fixed to a block",
        py::arg("node"))
      .def("nodes", [](Hypergraph &h) {
          return py::make_iterator(h.nodes().first,h.nodes().second);}, py::keep_alive<0, 1>(),
        "Iterate over all nodes")
      .def("edges", [](Hypergraph &h) {
          return py::make_iterator(h.edges().first,h.edges().second);}, py::keep_alive<0, 1>(),
        "Iterate over all hyperedges")
      .def("pins", [](Hypergraph &h, HyperedgeID he) {
          return py::make_iterator(h.pins(he).first,h.pins(he).second);}, py::keep_alive<0, 1>(),
        "Iterate over all pins of the hyperedge",
        py::arg("hyperedge"))
      .def("incidentEdges", [](Hypergraph &h, HypernodeID hn) {
          return py::make_iterator(h.incidentEdges(hn).first,h.incidentEdges(hn).second);}, py::keep_alive<0, 1>(),
        "Iterate over all incident hyperedges of the node",
        py::arg("node"));


  py::class_<ConnectivitySet>(m,
                              "Connectivity Set")
      .def("contains",&ConnectivitySet::contains,
           "Check if the block is contained in the connectivity set of the hyperedge",
           py::arg("block"))
      .def("__iter__",
           [](const ConnectivitySet& con) {
             return py::make_iterator(con.begin(),con.end());
           },py::keep_alive<0, 1>(),
           "Iterate over all blocks contained in the connectivity set of the hyperedge");

  m.def(
      "createHypergraphFromFile", &kahypar::io::createHypergraphFromFile,
      "Construct a hypergraph from a file in hMETIS format",
      py::arg("filename"), py::arg("k"));


  m.def(
      "partition", &partition,
      "Compute a k-way partition of the hypergraph",
      py::arg("hypergraph"), py::arg("context"));

  m.def(
      "cut", &kahypar::metrics::hyperedgeCut,
      "Compute the cut-net metric for the partitioned hypergraph",
      py::arg("hypergraph"));

  m.def(
      "soed", &kahypar::metrics::soed,
      "Compute the sum-of-extrnal-degrees metric for the partitioned hypergraph",
      py::arg("hypergraph"));

  m.def(
      "connectivityMinusOne", &kahypar::metrics::km1,
      "Compute the connecivity metric for the partitioned hypergraph",
      py::arg("hypergraph"));

    m.def(
        "imbalance", &kahypar::metrics::imbalance,
        "Compute the imbalance of the hypergraph partition",
      py::arg("hypergraph"),py::arg("context"));


  using kahypar::Context;
  py::class_<Context>(
      m, "Context")
      .def(py::init<>())
      .def("setK",[](Context& c, const PartitionID k) {
          c.partition.k = k;
        },
        "Number of blocks the hypergraph should be partitioned into",
        py::arg("k"))
      .def("setEpsilon",[](Context& c, const double eps) {
          c.partition.epsilon = eps;
        },
        "Allowed imbalance epsilon",
        py::arg("imbalance parameter epsilon"))
      .def("setCustomTargetBlockWeights",
        [](Context& c, const std::vector<kahypar::HypernodeWeight>& custom_target_weights) {
          c.partition.use_individual_part_weights = true;
          c.partition.max_part_weights.clear();
          for ( size_t block = 0; block < custom_target_weights.size(); ++block ) {
            c.partition.max_part_weights.push_back(custom_target_weights[block]);
          }
        },
        "Assigns each block of the partition an individual maximum allowed block weight",
        py::arg("custom target block weights"))
      .def("setSeed",[](Context& c, const int seed) {
          c.partition.seed = seed;
        },
        "Seed for the random number generator",
        py::arg("seed"))
      .def("suppressOutput",[](Context& c, const bool decision) {
          c.partition.quiet_mode = decision;
        },
        "Suppress partitioning output",
        py::arg("bool"))
      .def("loadINIconfiguration",
           [](Context& c, const std::string& path) {
             parseIniToContext(c, path);
           },
           "Read KaHyPar configuration from file",
           py::arg("path-to-file")
           );


#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
