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
        context.loadINIconfiguration("/home/schlag/repo/kahypar/config/km1_kKaHyPar_dissertation.ini")

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
