#ifndef LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
#define LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
#include "gmock/gmock.h"

#include "Hypergraph.h"

namespace datastructure {
using ::testing::Test;

using datastructure::HypergraphType;
using datastructure::HyperedgeIndexVector;
using datastructure::HyperedgeVector;
using datastructure::HypernodeIterator;

class AHypergraph : public Test {
 public:
  AHypergraph() :
      hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                 HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}) {}
  HypergraphType hypergraph;
};

class AHypernodeIterator : public AHypergraph {
 public:
  AHypernodeIterator() :
      AHypergraph(),
      begin(),
      end() {}

  HypernodeIterator begin;
  HypernodeIterator end;
};

class AHyperedgeIterator : public AHypergraph {
 public:
  AHyperedgeIterator() :
      AHypergraph(),
      begin(),
      end() {}

  HyperedgeIterator begin;
  HyperedgeIterator end;
};

class AHypergraphMacro : public AHypergraph {
 public:
  AHypergraphMacro() : AHypergraph() {}
};

class AContractionMemento : public AHypergraph {
 public:
  AContractionMemento() : AHypergraph() {}
};

class AnUncontractionOperation : public AHypergraph {
 public:
  AnUncontractionOperation() : AHypergraph() {}
};

class AnUncontractedHypergraph : public AHypergraph {
 public:
  AnUncontractedHypergraph() :
      AHypergraph(),
      modified_hypergraph(7,4, HyperedgeIndexVector {0,2,6,9,/*sentinel*/12},
                          HyperedgeVector {0,2,0,1,3,4,3,4,6,2,5,6}) {}

  HypergraphType modified_hypergraph;
};

} // namespace datastructure


#endif  // LIB_DATASTRUCTURE_HYPERGRAPH_TESTFIXTURES_H_
