/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
#define SRC_LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
#include "gmock/gmock.h"

#include "lib/definitions.h"

using::testing::Test;
using defs::Hypergraph;
using defs::HyperedgeIndexVector;
using defs::HyperedgeVector;
using defs::HyperedgeID;
using defs::HypernodeIterator;
using defs::HyperedgeIterator;

namespace datastructure {
class AHypergraph : public Test {
  public:
  AHypergraph() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }
  Hypergraph hypergraph;
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
  AnUncontractionOperation() : AHypergraph() {
    hypergraph.setNodePart(0, 0);
    hypergraph.setNodePart(1, 0);
    hypergraph.setNodePart(2, 0);
    hypergraph.setNodePart(3, 0);
    hypergraph.setNodePart(4, 0);
    hypergraph.setNodePart(5, 0);
    hypergraph.setNodePart(6, 0);
  }
};

class AnUncontractedHypergraph : public AHypergraph {
  public:
  AnUncontractedHypergraph() :
    AHypergraph(),
    modified_hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9, /*sentinel*/ 12 },
                        HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }

  Hypergraph modified_hypergraph;
};

class APartitionedHypergraph : public AHypergraph {
  public:
  APartitionedHypergraph() :
    AHypergraph() {
    hypergraph.setNodePart(0, 0);
    hypergraph.setNodePart(1, 0);
    hypergraph.setNodePart(2, 1);
    hypergraph.setNodePart(3, 0);
    hypergraph.setNodePart(4, 0);
    hypergraph.setNodePart(5, 1);
    hypergraph.setNodePart(6, 1);
  }
};

class AnUnPartitionedHypergraph : public AHypergraph {
  public:
  AnUnPartitionedHypergraph() :
    AHypergraph() { }
};
} // namespace datastructure


#endif  // SRC_LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
