#/*******************************************************************************
# * This file is part of KaHyPar.
# *
# * Copyright (C) 2019 Sebastian Schlag <sebastian.schlag@kit.edu>
# *
# * KaHyPar is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, either version 3 of the License, or
# * (at your option) any later version.
# *
# * KaHyPar is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
# *
# ******************************************************************************/

import unittest
import os

import kahypar as kahypar

mydir = os.path.dirname(os.path.realpath(__file__))

class MainTest(unittest.TestCase):
    # build a custom hypergraph
    def test_construct_hypergraph(self):
        num_nodes = 7
        num_nets = 4

        hyperedge_indices = [0,2,6,9,12]
        hyperedges = [0,2,0,1,3,4,3,4,6,2,5,6]

        k=2

        hypergraph = kahypar.Hypergraph(num_nodes, num_nets, hyperedge_indices, hyperedges, k)

        self.assertEqual(hypergraph.numNodes(),7)
        self.assertEqual(hypergraph.numEdges(),4)
        self.assertEqual(hypergraph.numPins(),12)

     # build a custom weighted hypergraph
    def test_construct_weighted_hypergraph(self):
        num_nodes = 7
        num_nets = 4

        hyperedge_indices = [0,2,6,9,12]
        hyperedges = [0,2,0,1,3,4,3,4,6,2,5,6]

        node_weights = [1,2,3,4,5,6,7]
        edge_weights = [11,22,33,44]

        k=2

        hypergraph = kahypar.Hypergraph(num_nodes, num_nets, hyperedge_indices, hyperedges, k, edge_weights, node_weights)

        self.assertEqual(hypergraph.nodeWeight(0),1)
        self.assertEqual(hypergraph.nodeWeight(1),2)
        self.assertEqual(hypergraph.nodeWeight(2),3)
        self.assertEqual(hypergraph.nodeWeight(3),4)
        self.assertEqual(hypergraph.nodeWeight(4),5)
        self.assertEqual(hypergraph.nodeWeight(5),6)
        self.assertEqual(hypergraph.nodeWeight(6),7)

        self.assertEqual(hypergraph.edgeWeight(0),11)
        self.assertEqual(hypergraph.edgeWeight(1),22)
        self.assertEqual(hypergraph.edgeWeight(2),33)
        self.assertEqual(hypergraph.edgeWeight(3),44)


    # construct hypergraph from file
    def test_construct_hypergraph_from_file(self):
        ibm01 = kahypar.createHypergraphFromFile(mydir+"/ISPD98_ibm01.hgr",2)
        print("her")
        self.assertEqual(ibm01.numNodes(),12752)
        self.assertEqual(ibm01.numEdges(),14111)
        self.assertEqual(ibm01.numPins(),50566)


    # partition hypergraph
    def test_partition_hypergraph(self):
        context = kahypar.Context()
        context.loadINIconfiguration(mydir+"/../..//config/km1_kKaHyPar_dissertation.ini")

        ibm01 = kahypar.createHypergraphFromFile(mydir+"/ISPD98_ibm01.hgr",2)

        context.setK(2)
        context.setEpsilon(0.03)
        kahypar.partition(ibm01, context)

        self.assertEqual(kahypar.cut(ibm01), 202)
        self.assertEqual(kahypar.soed(ibm01), 404)
        self.assertEqual(kahypar.connectivityMinusOne(ibm01), 202)
        self.assertEqual(kahypar.imbalance(ibm01,context), 0.027603513174403904)

if __name__ == '__main__':
    unittest.main()
