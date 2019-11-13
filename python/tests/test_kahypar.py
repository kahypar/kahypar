import unittest
import os

import kahypar as kahypar

mydir = os.path.dirname(os.path.realpath(__file__))

class MainTest(unittest.TestCase):
    # read a directory containing FastA files
    def test_doc_list(self):
        num_nodes = 7
        num_nets = 4

        hyperedge_indices = [0,2,6,9,12]
        hyperedges = [0,2,0,1,3,4,3,4,6,2,5,6]

        k=2

        test = kahypar.Hypergraph(num_nodes, num_nets, hyperedge_indices, hyperedges, k)

        test.printGraphState()

        for node in test.nodes():
            print(node)

        for net in test.edges():
            print(net)

        print("===")

        for pin in test.pins(1):
            print(pin)

        for edge in test.incidentEdges(2):
            print(edge)


        print("degree of node 2:", test.nodeDegree(2))

        ccc = kahypar.Context()
        ccc.loadINIconfiguration("/home/schlag/repo/kahypar/config/km1_kKaHyPar_dissertation.ini")

        self.assertEqual(7, 7)

if __name__ == '__main__':
    unittest.main()
