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
  using kahypar::PartitionID;
  using ConnectivitySet = typename ConnectivitySets<PartitionID, HyperedgeID>::ConnectivitySet;

  py::class_<Hypergraph>(
      m, "Hypergraph")
      .def(py::init<const HypernodeID,
           const HyperedgeID,
           const HyperedgeIndexVector,
           const HyperedgeVector,
           const PartitionID>())
      .def("printGraphState", &Hypergraph::printGraphState)
      .def("nodeDegree", &Hypergraph::nodeDegree)
      .def("edgeSize", &Hypergraph::edgeSize)
      .def("nodeWeight", &Hypergraph::nodeWeight)
      .def("edgeWeight", &Hypergraph::edgeWeight)
      .def("blockID", &Hypergraph::partID)
      .def("numNodes", &Hypergraph::initialNumNodes)
      .def("numEdges", &Hypergraph::initialNumEdges)
      .def("numPins", &Hypergraph::initialNumPins)
      .def("numBlocks", &Hypergraph::k)
      .def("numPinsInBlock", &Hypergraph::pinCountInPart)
      .def("connectivity", &Hypergraph::connectivity)
      .def("connectivitySet", &Hypergraph::connectivitySet, py::return_value_policy::reference_internal)
      .def("communities", &Hypergraph::communities)
      .def("blockWeight", &Hypergraph::partWeight)
      .def("blockSize", &Hypergraph::partSize)
      .def("reset()", &Hypergraph::reset)
      .def("nodes", [](Hypergraph &h) {
          return py::make_iterator(h.nodes().first,h.nodes().second);}, py::keep_alive<0, 1>())
      .def("edges", [](Hypergraph &h) {
          return py::make_iterator(h.edges().first,h.edges().second);}, py::keep_alive<0, 1>())
      .def("pins", [](Hypergraph &h, HyperedgeID he) {
          return py::make_iterator(h.pins(he).first,h.pins(he).second);}, py::keep_alive<0, 1>())
      .def("incidentEdges", [](Hypergraph &h, HypernodeID hn) {
          return py::make_iterator(h.incidentEdges(hn).first,h.incidentEdges(hn).second);}, py::keep_alive<0, 1>());


  py::class_<ConnectivitySet>(m,
                              "Connectivity Set")
      .def("contains",&ConnectivitySet::contains)
      .def("__iter__",
           [](const ConnectivitySet& con) {
             return py::make_iterator(con.begin(),con.end());
           },py::keep_alive<0, 1>());

  m.def(
      "createHypergraphFromFile", &kahypar::io::createHypergraphFromFile,
      py::arg("filename"), py::arg("k"));


  m.def(
      "partition", &partition,
      py::arg("hypergraph"), py::arg("context"));

  m.def(
      "cut", &kahypar::metrics::hyperedgeCut,
      py::arg("hypergraph"));

  m.def(
      "soed", &kahypar::metrics::soed,
      py::arg("hypergraph"));

  m.def(
      "connectivityMinusOne", &kahypar::metrics::km1,
      py::arg("hypergraph"));

    m.def(
        "imbalance", &kahypar::metrics::imbalance,
      py::arg("hypergraph"),py::arg("context"));


  using kahypar::Context;
  py::class_<Context>(
      m, "Context")
      .def(py::init<>())
      .def("setK",[](Context& c, const PartitionID k) {
          c.partition.k = k;
        },py::arg("k"))
      .def("setEpsilon",[](Context& c, const double eps) {
          c.partition.epsilon = eps;
        },py::arg("imbalance parameter epsilon"))
      .def("setSeed",[](Context& c, const int seed) {
          c.partition.seed = seed;
        },py::arg("k"))
      .def("suppressOutput",[](Context& c, const bool decision) {
          c.partition.quiet_mode = decision;
        },py::arg("imbalance parameter epsilon"))
      .def("loadINIconfiguration",
           [](Context& c, const std::string& path) {
             parseIniToContext(c, path);
           },py::arg("path")
           );


#ifdef VERSION_INFO
    m.attr("__version__") = VERSION_INFO;
#else
    m.attr("__version__") = "dev";
#endif
}
