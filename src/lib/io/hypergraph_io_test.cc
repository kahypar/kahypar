#include <string>

#include "gmock/gmock.h"

#include "../definitions.h"
#include "../datastructure/Hypergraph.h"
#include "HypergraphIO.h"

namespace io {

using ::testing::Eq;
using ::testing::ContainerEq;
using ::testing::Test;

typedef typename hgr::Hypergraph<HyperNodeID,HyperEdgeID,HyperNodeWeight,HyperEdgeWeight> HypergraphType;

class AnUnweightedHypergraph : public Test {
 public:
  AnUnweightedHypergraph() :
      _filename(""),
      _file(),
      _num_hyperedges(0),
      _num_hypernodes(0),
      _hypergraph_type(0),
      _control_index_vector({0,2,6,9,12}),
      _control_edge_vector({0,1,0,6,4,5,4,5,3,1,2,3}) {}

  void SetUp() {
    _filename = "test_instances/unweighted_hypergraph.hgr";
    _file.open(_filename, std::ifstream::in);
  }

  void TearDown() {
    _file.close();
  }
  
  std::string _filename;
  std::ifstream _file;
  HyperEdgeID _num_hyperedges;
  HyperNodeID _num_hypernodes;
  int _hypergraph_type;
  hMetisHyperEdgeIndexVector _control_index_vector;
  hMetisHyperEdgeVector _control_edge_vector;
};

class AHypergraphWithHyperedgeWeights : public AnUnweightedHypergraph {
 public:
  AHypergraphWithHyperedgeWeights() :
      AnUnweightedHypergraph(),
      _control_hyperedge_weights({2,3,8,7}) {}

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_hypergraph.hgr";
    _file.open(_filename, std::ifstream::in);
  }
  
  hMetisHyperEdgeWeightVector _control_hyperedge_weights;
};

class AHypergraphWithHypernodeWeights : public AnUnweightedHypergraph {
 public:
  AHypergraphWithHypernodeWeights() :
      _control_hypernode_weights({5,1,8,7,3,9,3}) {}

  void SetUp() {
    _filename = "test_instances/weighted_hypernodes_hypergraph.hgr";
    _file.open(_filename, std::ifstream::in);
  }

  hMetisHyperEdgeWeightVector _control_hypernode_weights;
};

class AHypergraphWithHypernodeAndHyperedgeWeights : public AnUnweightedHypergraph {
 public:
  AHypergraphWithHypernodeAndHyperedgeWeights() :   
      _control_hyperedge_weights({2,3,8,7}),
      _control_hypernode_weights({5,1,8,7,3,9,3}) {}

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_and_hypernodes_hypergraph.hgr";
    _file.open(_filename, std::ifstream::in);
  }
  
  hMetisHyperEdgeWeightVector _control_hyperedge_weights;
  hMetisHyperEdgeWeightVector _control_hypernode_weights;
};

TEST(AFunction, ParsesFirstLineOfaHGRFile) {
  std::string filename("test_instances/unweighted_hypergraph.hgr");
  std::ifstream file(filename);
  HyperEdgeID num_hyperedges = 0;
  HyperNodeID num_hypernodes = 0;
  int hypergraph_type = 0;
  parseHGRHeader(file, num_hyperedges, num_hypernodes, hypergraph_type);
  ASSERT_THAT(num_hyperedges, Eq(4));
  ASSERT_THAT(num_hypernodes, Eq(7));
  ASSERT_THAT(hypergraph_type, Eq(0));
}

TEST_F(AnUnweightedHypergraph, CanBeConstructedFromFile) {
  hMetisHyperEdgeIndexVector index_vector;
  hMetisHyperEdgeVector edge_vector;
  parseHypergraphFile(_file, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                      nullptr, nullptr);
  
  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  HypergraphType hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                            nullptr, nullptr);  
}

TEST_F(AHypergraphWithHyperedgeWeights, CanBeConstructedFromFile) {
  hMetisHyperEdgeIndexVector index_vector;
  hMetisHyperEdgeVector edge_vector;
  hMetisHyperEdgeWeightVector hyperedge_weights;

  parseHypergraphFile(_file, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                      &hyperedge_weights, nullptr);
  
  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  ASSERT_THAT(hyperedge_weights, ContainerEq(_control_hyperedge_weights));
  HypergraphType hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                            &hyperedge_weights, nullptr);  
}

TEST_F(AHypergraphWithHypernodeWeights, CanBeConstructedFromFile) {
  hMetisHyperEdgeIndexVector index_vector;
  hMetisHyperEdgeVector edge_vector;
  hMetisHyperEdgeWeightVector hypernode_weights;

  parseHypergraphFile(_file, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                      nullptr, &hypernode_weights);
  
  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  ASSERT_THAT(hypernode_weights, ContainerEq(_control_hypernode_weights));
  HypergraphType hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                            nullptr, &hypernode_weights);  
}

TEST_F(AHypergraphWithHypernodeAndHyperedgeWeights, CanBeConstructedFromFile) {
  hMetisHyperEdgeIndexVector index_vector;
  hMetisHyperEdgeVector edge_vector;
  hMetisHyperEdgeWeightVector hypernode_weights;
  hMetisHyperEdgeWeightVector hyperedge_weights;

  parseHypergraphFile(_file, _num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                      &hyperedge_weights, &hypernode_weights);
  
  ASSERT_THAT(index_vector, ContainerEq(_control_index_vector));
  ASSERT_THAT(edge_vector, ContainerEq(_control_edge_vector));
  ASSERT_THAT(hyperedge_weights, ContainerEq(_control_hyperedge_weights));
  ASSERT_THAT(hypernode_weights, ContainerEq(_control_hypernode_weights));
  HypergraphType hypergraph(_num_hypernodes, _num_hyperedges, index_vector, edge_vector,
                            &hyperedge_weights, &hypernode_weights);
}


}
