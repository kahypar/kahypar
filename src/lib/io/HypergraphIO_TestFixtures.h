#ifndef _LIB_IO_HYPERGRAPHIO_TESTFIXTURES_H_
#define _LIB_IO_HYPERGRAPHIO_TESTFIXTURES_H_

#include <cstdio>
#include <string>

#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph.h"
#include "partition/Partitioner.h"
#include "partition/coarsening/HeuristicHeavyEdgeCoarsener.h"
#include "partition/coarsening/ICoarsener.h"
#include "partition/coarsening/Rater.h"

namespace io {
using::testing::Test;

using partition::Rater;
using partition::ICoarsener;
using partition::HeuristicHeavyEdgeCoarsener;
using partition::Partitioner;
using partition::FirstRatingWins;
using partition::Configuration;

class AnUnweightedHypergraphFile : public Test {
  public:
  AnUnweightedHypergraphFile() :
    _filename(""),
    _num_hyperedges(0),
    _num_hypernodes(0),
    _hypergraph_type(0),
    _control_index_vector({ 0, 2, 6, 9, 12 }),
    _control_edge_vector({ 0, 1, 0, 6, 4, 5, 4, 5, 3, 1, 2, 3 }) { }

  void SetUp() {
    _filename = "test_instances/unweighted_hypergraph.hgr";
  }

  std::string _filename;
  HyperedgeID _num_hyperedges;
  HypernodeID _num_hypernodes;
  int _hypergraph_type;
  HyperedgeIndexVector _control_index_vector;
  HyperedgeVector _control_edge_vector;
};

class AHypergraphFileWithHyperedgeWeights : public AnUnweightedHypergraphFile {
  public:
  AHypergraphFileWithHyperedgeWeights() :
    AnUnweightedHypergraphFile(),
    _control_hyperedge_weights({ 2, 3, 8, 7 }) { }

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_hypergraph.hgr";
  }

  HyperedgeWeightVector _control_hyperedge_weights;
};

class AHypergraphFileWithHypernodeWeights : public AnUnweightedHypergraphFile {
  public:
  AHypergraphFileWithHypernodeWeights() :
    AnUnweightedHypergraphFile(),
    _control_hypernode_weights({ 5, 1, 8, 7, 3, 9, 3 }) { }

  void SetUp() {
    _filename = "test_instances/weighted_hypernodes_hypergraph.hgr";
  }

  HyperedgeWeightVector _control_hypernode_weights;
};

class AHypergraphFileWithHypernodeAndHyperedgeWeights : public AnUnweightedHypergraphFile {
  public:
  AHypergraphFileWithHypernodeAndHyperedgeWeights() :
    AnUnweightedHypergraphFile(),
    _control_hyperedge_weights({ 2, 3, 8, 7 }),
    _control_hypernode_weights({ 5, 1, 8, 7, 3, 9, 3 }) { }

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_and_hypernodes_hypergraph.hgr";
  }

  HyperedgeWeightVector _control_hyperedge_weights;
  HypernodeWeightVector _control_hypernode_weights;
};

class AnUnweightedHypergraph : public Test {
  public:
  AnUnweightedHypergraph() :
    _filename(""),
    _num_hyperedges(4),
    _num_hypernodes(7),
    _hypergraph_type(0),
    _index_vector({ 0, 2, 6, 9, 12 }),
    _edge_vector({ 0, 1, 0, 6, 4, 5, 4, 5, 3, 1, 2, 3 }),
    _written_index_vector(),
    _written_edge_vector(),
    _written_file(),
    _hypergraph(nullptr) { }

  void SetUp() {
    _filename = "test_instances/unweighted_hypergraph.hgr.out";
    _hypergraph = new HypergraphType(_num_hypernodes, _num_hyperedges, _index_vector, _edge_vector);
  }

  void TearDown() {
    std::remove(_filename.c_str());
    delete _hypergraph;
  }

  std::string _filename;
  HyperedgeID _num_hyperedges;
  HypernodeID _num_hypernodes;
  int _hypergraph_type;
  HyperedgeIndexVector _index_vector;
  HyperedgeVector _edge_vector;
  HyperedgeIndexVector _written_index_vector;
  HyperedgeVector _written_edge_vector;
  std::ifstream _written_file;
  HypergraphType* _hypergraph;
  DISALLOW_COPY_AND_ASSIGN(AnUnweightedHypergraph);
};

class AHypergraphWithHyperedgeWeights : public AnUnweightedHypergraph {
  public:
  AHypergraphWithHyperedgeWeights() :
    AnUnweightedHypergraph(),
    _hyperedge_weights({ 2, 3, 8, 7 }),
    _written_hyperedge_weights() { }

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_hypergraph.hgr.out";
    _hypergraph = new HypergraphType(_num_hypernodes, _num_hyperedges, _index_vector, _edge_vector,
                                     &_hyperedge_weights);
  }

  HyperedgeWeightVector _hyperedge_weights;
  HyperedgeWeightVector _written_hyperedge_weights;
};

class AHypergraphWithHypernodeWeights : public AnUnweightedHypergraph {
  public:
  AHypergraphWithHypernodeWeights() :
    AnUnweightedHypergraph(),
    _hypernode_weights({ 5, 1, 8, 7, 3, 9, 3 }),
    _written_hypernode_weights() { }

  void SetUp() {
    _filename = "test_instances/weighted_hypernodes_hypergraph.hgr.out";
    _hypergraph = new HypergraphType(_num_hypernodes, _num_hyperedges, _index_vector, _edge_vector,
                                     nullptr, &_hypernode_weights);
  }

  HypernodeWeightVector _hypernode_weights;
  HypernodeWeightVector _written_hypernode_weights;
};

class AHypergraphWithHypernodeAndHyperedgeWeights : public AnUnweightedHypergraph {
  public:
  AHypergraphWithHypernodeAndHyperedgeWeights() :
    AnUnweightedHypergraph(),
    _hyperedge_weights({ 2, 3, 8, 7 }),
    _hypernode_weights({ 5, 1, 8, 7, 3, 9, 3 }),
    _written_hyperedge_weights(),
    _written_hypernode_weights() { }

  void SetUp() {
    _filename = "test_instances/weighted_hyperedges_and_hypernodes_hypergraph.hgr.out";
    _hypergraph = new HypergraphType(_num_hypernodes, _num_hyperedges, _index_vector, _edge_vector,
                                     &_hyperedge_weights, &_hypernode_weights);
  }

  HyperedgeWeightVector _hyperedge_weights;
  HypernodeWeightVector _hypernode_weights;
  HyperedgeWeightVector _written_hyperedge_weights;
  HypernodeWeightVector _written_hypernode_weights;
};

typedef Rater<HypergraphType, defs::RatingType, FirstRatingWins> FirstWinsRater;
typedef HeuristicHeavyEdgeCoarsener<HypergraphType, FirstWinsRater> FirstWinsCoarsener;
typedef Configuration<HypergraphType> PartitionConfig;
typedef Partitioner<HypergraphType> HypergraphPartitioner;

class APartitionOfAHypergraph : public Test {
  public:
  APartitionOfAHypergraph() :
    _hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
                HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }),
    _config(),
    _partitioner(_config),
    _coarsener(new FirstWinsCoarsener(_hypergraph, _config)) {
    _config.coarsening.minimal_node_count = 2;
    _config.coarsening.threshold_node_weight = 5;
    _config.partitioning.graph_filename = "APartitionOfAHypergrpahTest";
    _config.partitioning.graph_partition_filename = "APartitionOfAHypergrpahTest.hgr.part.2.KaHyPar";
    _config.partitioning.coarse_graph_filename = "APartitionOfAHypergrpahTest_coarse.hgr";
    _config.partitioning.coarse_graph_partition_filename = "APartitionOfAHypergrpahTest_coarse.hgr.part.2";
  }

  void TearDown() {
    std::remove(_config.partitioning.graph_partition_filename.c_str());
  }

  HypergraphType _hypergraph;
  PartitionConfig _config;
  HypergraphPartitioner _partitioner;
  std::unique_ptr<ICoarsener<HypergraphType> > _coarsener;
};
} // namespace io

#endif  // _LIB_IO_HYPERGRAPHIO_TESTFIXTURES_H_
