/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
#define SRC_LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
#include "gmock/gmock.h"

#include "lib/datastructure/Hypergraph.h"

using::testing::Test;
using defs::INVALID_PARTITION;

namespace datastructure {
class AHypergraph : public Test {
  public:
  AHypergraph() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }
  HypergraphType hypergraph;
};

class AHypernodeIterator : public AHypergraph {
  public:
  AHypernodeIterator() :
    AHypergraph(),
    begin(),
    end() { }

  HypernodeIterator begin;
  HypernodeIterator end;
};

class AHyperedgeIterator : public AHypergraph {
  public:
  AHyperedgeIterator() :
    AHypergraph(),
    begin(),
    end() { }

  HyperedgeIterator begin;
  HyperedgeIterator end;
};

class AHypergraphMacro : public AHypergraph {
  public:
  AHypergraphMacro() : AHypergraph() { }
};

class AContractionMemento : public AHypergraph {
  public:
  AContractionMemento() : AHypergraph() { }
};

class AnUncontractionOperation : public AHypergraph {
  public:
  AnUncontractionOperation() : AHypergraph() { }
};

class AnUncontractedHypergraph : public AHypergraph {
  public:
  AnUncontractedHypergraph() :
    AHypergraph(),
    modified_hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
                        HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }

  HypergraphType modified_hypergraph;
};

class APartitionedHypergraph : public AHypergraph {
  public:
  APartitionedHypergraph() :
    AHypergraph() {
    hypergraph.changeNodePartition(0, INVALID_PARTITION, 0);
    hypergraph.changeNodePartition(1, INVALID_PARTITION, 0);
    hypergraph.changeNodePartition(2, INVALID_PARTITION, 1);
    hypergraph.changeNodePartition(3, INVALID_PARTITION, 0);
    hypergraph.changeNodePartition(4, INVALID_PARTITION, 0);
    hypergraph.changeNodePartition(5, INVALID_PARTITION, 1);
    hypergraph.changeNodePartition(6, INVALID_PARTITION, 1);
  }
};

class AnUnPartitionedHypergraph : public AHypergraph {
  public:
  AnUnPartitionedHypergraph() :
    AHypergraph() { }
};
} // namespace datastructure


#endif  // SRC_LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
