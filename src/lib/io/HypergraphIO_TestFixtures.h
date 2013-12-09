#ifndef _LIB_IO_HYPERGRAPHIO_TESTFIXTURES_H_
#define _LIB_IO_HYPERGRAPHIO_TESTFIXTURES_H_

#include <cstdio>
#include <string>

#include "gmock/gmock.h"

#include "../datastructure/Hypergraph.h"

namespace io {

using ::testing::Test;

typedef datastructure::HypergraphType HypergraphType;
typedef HypergraphType::HypernodeID HypernodeID;
typedef HypergraphType::HyperedgeID HyperedgeID;

class AnUnweightedHypergraphFile : public Test {
 public:
  AnUnweightedHypergraphFile() :
      _filename(""),
      _num_hyperedges(0),
      _num_hypernodes(0),
      _hypergraph_type(0),
      _control_index_vector({0,2,6,9,12}),
      _control_edge_vector({0,1,0,6,4,5,4,5,3,1,2,3}) {}

  void SetUp() {
    _filename = "test_instances/unweighted_hypergraph.hgr";
  }

  std::string _filename;
  HyperedgeID _num_hyperedges;
  HypernodeID _num_hypernodes;
  int _hypergraph_type;
  hMetisHyperEdgeIndexVector _control_index_vector;
  hMetisHyperEdgeVector _control_edge_vector;
};

class AHypergraphFileWithHyperedgeWeights : public AnUnweightedHypergraphFile {
 public:
  AHypergraphFileWithHyperedgeWeights() :
      AnUnweightedHypergraphFile(),
      _control_hyperedge_weights({2,3,8,7}) {}

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_hypergraph.hgr";
  }
  
  hMetisHyperEdgeWeightVector _control_hyperedge_weights;
};

class AHypergraphFileWithHypernodeWeights : public AnUnweightedHypergraphFile {
 public:
  AHypergraphFileWithHypernodeWeights() :
      AnUnweightedHypergraphFile(),
      _control_hypernode_weights({5,1,8,7,3,9,3}) {}

  void SetUp() {
    _filename = "test_instances/weighted_hypernodes_hypergraph.hgr";
  }

  hMetisHyperEdgeWeightVector _control_hypernode_weights;
};

class AHypergraphFileWithHypernodeAndHyperedgeWeights : public AnUnweightedHypergraphFile {
 public:
  AHypergraphFileWithHypernodeAndHyperedgeWeights() :
      AnUnweightedHypergraphFile(),
      _control_hyperedge_weights({2,3,8,7}),
      _control_hypernode_weights({5,1,8,7,3,9,3}) {}

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_and_hypernodes_hypergraph.hgr";
  }
  
  hMetisHyperEdgeWeightVector _control_hyperedge_weights;
  hMetisHyperEdgeWeightVector _control_hypernode_weights;
};

class AnUnweightedHypergraph : public Test {
 public:
  AnUnweightedHypergraph() :
      _filename(""),
      _num_hyperedges(4),
      _num_hypernodes(7),
      _hypergraph_type(0),
      _index_vector({0,2,6,9,12}),
      _edge_vector({0,1,0,6,4,5,4,5,3,1,2,3}),
      _written_index_vector(),
      _written_edge_vector(),
      _written_file(),
      _hypergraph(nullptr){}

  void SetUp() {
    _filename = "test_instances/unweighted_hypergraph.hgr.out";
    _hypergraph = new HypergraphType(_num_hypernodes, _num_hyperedges, _index_vector, _edge_vector,
                                     nullptr, nullptr);
  }

  void TearDown() {
    std::remove(_filename.c_str());
    delete _hypergraph;
  }
  
  std::string _filename;
  HyperedgeID _num_hyperedges;
  HypernodeID _num_hypernodes;
  int _hypergraph_type;
  hMetisHyperEdgeIndexVector _index_vector;
  hMetisHyperEdgeVector _edge_vector;
  hMetisHyperEdgeIndexVector _written_index_vector;
  hMetisHyperEdgeVector _written_edge_vector;
  std::ifstream _written_file;
  HypergraphType* _hypergraph;
  DISALLOW_COPY_AND_ASSIGN(AnUnweightedHypergraph);
};

class AHypergraphWithHyperedgeWeights : public AnUnweightedHypergraph {
 public:
  AHypergraphWithHyperedgeWeights() :
      AnUnweightedHypergraph(),
      _hyperedge_weights({2,3,8,7}),
      _written_hyperedge_weights() {}

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_hypergraph.hgr.out";
    _hypergraph = new HypergraphType(_num_hypernodes, _num_hyperedges, _index_vector, _edge_vector,
                                     &_hyperedge_weights, nullptr);
  }
  
  hMetisHyperEdgeWeightVector _hyperedge_weights;
  hMetisHyperEdgeWeightVector _written_hyperedge_weights;
};

class AHypergraphWithHypernodeWeights : public AnUnweightedHypergraph {
 public:
  AHypergraphWithHypernodeWeights() :
      AnUnweightedHypergraph(),
      _hypernode_weights({5,1,8,7,3,9,3}),
      _written_hypernode_weights() {}

  void SetUp() {
    _filename = "test_instances/weighted_hypernodes_hypergraph.hgr.out";
    _hypergraph = new HypergraphType(_num_hypernodes, _num_hyperedges, _index_vector, _edge_vector,
                                     nullptr, &_hypernode_weights);
  }

  hMetisHyperEdgeWeightVector _hypernode_weights;
  hMetisHyperEdgeWeightVector _written_hypernode_weights;
};

class AHypergraphWithHypernodeAndHyperedgeWeights : public AnUnweightedHypergraph {
 public:
  AHypergraphWithHypernodeAndHyperedgeWeights() :
      AnUnweightedHypergraph(),
      _hyperedge_weights({2,3,8,7}),
      _hypernode_weights({5,1,8,7,3,9,3}),
      _written_hyperedge_weights(),
      _written_hypernode_weights() {}

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_and_hypernodes_hypergraph.hgr.out";
    _hypergraph = new HypergraphType(_num_hypernodes, _num_hyperedges, _index_vector, _edge_vector,
                            &_hyperedge_weights, &_hypernode_weights);
  }
  
  hMetisHyperEdgeWeightVector _hyperedge_weights;
  hMetisHyperEdgeWeightVector _hypernode_weights;
  hMetisHyperEdgeWeightVector _written_hyperedge_weights;
  hMetisHyperEdgeWeightVector _written_hypernode_weights;
};

} // namespace io


#endif  // _LIB_IO_HYPERGRAPHIO_TESTFIXTURES_H_
