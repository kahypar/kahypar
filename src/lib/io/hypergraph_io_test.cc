#include "gmock/gmock.h"

#include "HypergraphIO.h"
#include "HypergraphIO_TestFixtures.h"

namespace io {

using ::testing::Eq;
using ::testing::ContainerEq;

TEST(AFunction, ParsesFirstLineOfaHGRFile) {
  std::string filename("test_instances/unweighted_hypergraph.hgr");
  std::ifstream file(filename);
  HyperedgeID num_hyperedges = 0;
  HypernodeID num_hypernodes = 0;
  HypergraphWeightType hypergraph_type = HypergraphWeightType::Unweighted;
  parseHGRHeader(file, num_hyperedges, num_hypernodes, hypergraph_type);
  ASSERT_THAT(num_hyperedges, Eq(4));
  ASSERT_THAT(num_hypernodes, Eq(7));
  ASSERT_THAT(hypergraph_type, Eq(HypergraphWeightType::Unweighted));
}

TEST_F(AnUnweightedHypergraphFile, CanBeParsedIntoAHypergraph) {
  hMetisHyperEdgeIndexVector index_vector;
  hMetisHyperEdgeVector edge_vector;
  parseHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                      nullptr, nullptr);
  
  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  HypergraphType hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                            nullptr, nullptr);  
}

TEST_F(AHypergraphFileWithHyperedgeWeights, CanBeParsedIntoAHypergraph) {
  hMetisHyperEdgeIndexVector index_vector;
  hMetisHyperEdgeVector edge_vector;
  hMetisHyperEdgeWeightVector hyperedge_weights;

  parseHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                      &hyperedge_weights, nullptr);
  
  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  ASSERT_THAT(hyperedge_weights, ContainerEq(_control_hyperedge_weights));
  HypergraphType hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                            &hyperedge_weights, nullptr);  
}

TEST_F(AHypergraphFileWithHypernodeWeights, CanBeParsedIntoAHypergraph) {
  hMetisHyperEdgeIndexVector index_vector;
  hMetisHyperEdgeVector edge_vector;
  hMetisHyperEdgeWeightVector hypernode_weights;

  parseHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                      nullptr, &hypernode_weights);
  
  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  ASSERT_THAT(hypernode_weights, ContainerEq(_control_hypernode_weights));
  HypergraphType hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                            nullptr, &hypernode_weights);  
}

TEST_F(AHypergraphFileWithHypernodeAndHyperedgeWeights, CanBeParsedIntoAHypergraph) {
  hMetisHyperEdgeIndexVector index_vector;
  hMetisHyperEdgeVector edge_vector;
  hMetisHyperEdgeWeightVector hypernode_weights;
  hMetisHyperEdgeWeightVector hyperedge_weights;

  parseHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                      &hyperedge_weights, &hypernode_weights);
  
  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  ASSERT_THAT(hyperedge_weights, ContainerEq(_control_hyperedge_weights));
  ASSERT_THAT(hypernode_weights, ContainerEq(_control_hypernode_weights));
  HypergraphType hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                            &hyperedge_weights, &hypernode_weights);
}

TEST_F(AnUnweightedHypergraph, CanBeWrittenToFile) {
  writeHypergraphFile(*_hypergraph, _filename);

  parseHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, _written_index_vector,
                      _written_edge_vector, nullptr, nullptr);  
  HypergraphType hypergraph2(_num_hypernodes, _num_hyperedges, _written_index_vector,
                             _written_edge_vector, nullptr, nullptr);
  
  ASSERT_THAT(verifyEquivalence(*_hypergraph, hypergraph2), Eq(true));
}

TEST_F(AHypergraphWithHyperedgeWeights, CanBeWrittenToFile) {
  writeHypergraphFile(*_hypergraph, _filename);

  parseHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, _written_index_vector,
                      _written_edge_vector, &_written_hyperedge_weights, nullptr);
  HypergraphType hypergraph2(_num_hypernodes, _num_hyperedges, _written_index_vector,
                             _written_edge_vector, &_written_hyperedge_weights, nullptr);
  ASSERT_THAT(verifyEquivalence(*_hypergraph, hypergraph2), Eq(true));
}

TEST_F(AHypergraphWithHypernodeWeights, CanBeWrittenToFile) {
  writeHypergraphFile(*_hypergraph, _filename);

  parseHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, _written_index_vector,
                      _written_edge_vector, nullptr, &_written_hypernode_weights);  
  HypergraphType hypergraph2(_num_hypernodes, _num_hyperedges, _written_index_vector,
                             _written_edge_vector, nullptr, &_written_hypernode_weights);

  ASSERT_THAT(verifyEquivalence(*_hypergraph, hypergraph2), Eq(true));
}

TEST_F(AHypergraphWithHypernodeAndHyperedgeWeights, CanBeWrittenToFile) {
  writeHypergraphFile(*_hypergraph, _filename);

  parseHypergraphFile(_filename, _num_hypernodes, _num_hyperedges, _written_index_vector,
                      _written_edge_vector, &_written_hyperedge_weights,
                      &_written_hypernode_weights);  
  HypergraphType hypergraph2(_num_hypernodes, _num_hyperedges, _written_index_vector,
                             _written_edge_vector, &_written_hyperedge_weights,
                             &_written_hypernode_weights);

  ASSERT_THAT(verifyEquivalence(*_hypergraph, hypergraph2), Eq(true));
}

}
