/*******************************************************************************
 * This file is part of KaHyPar.
 *
 * Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 *
 * KaHyPar is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KaHyPar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with KaHyPar.  If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#pragma once

#include "gmock/gmock.h"

#include "kahypar/definitions.h"

using ::testing::Test;

namespace kahypar {
namespace ds {
class AHypergraph : public Test {
 public:
  AHypergraph() :
    hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
               HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }
  Hypergraph hypergraph;
};

using AHypernodeIterator = AHypergraph;
using AHyperedgeIterator = AHypergraph;

class AHypergraphMacro : public AHypergraph {
 public:
  AHypergraphMacro() :
    AHypergraph() { }
};

class AContractionMemento : public AHypergraph {
 public:
  AContractionMemento() :
    AHypergraph() { }
};

class AnUncontractionOperation : public AHypergraph {
 public:
  AnUncontractionOperation() :
    AHypergraph() {
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
    modified_hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                        HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 }) { }

  Hypergraph modified_hypergraph;
};

class APartitionedHypergraph : public AHypergraph {
 public:
  APartitionedHypergraph() :
    AHypergraph(),
    original_hypergraph(7, 4, HyperedgeIndexVector { 0, 2, 6, 9,  /*sentinel*/ 12 },
                        HyperedgeVector { 0, 2, 0, 1, 3, 4, 3, 4, 6, 2, 5, 6 })
  {
    hypergraph.setNodePart(0, 0);
    hypergraph.setNodePart(1, 0);
    hypergraph.setNodePart(2, 1);
    hypergraph.setNodePart(3, 0);
    hypergraph.setNodePart(4, 0);
    hypergraph.setNodePart(5, 1);
    hypergraph.setNodePart(6, 1);
  }

  Hypergraph original_hypergraph;
};

class AnUnPartitionedHypergraph : public AHypergraph {
 public:
  AnUnPartitionedHypergraph() :
    AHypergraph() { }
};
}  // namespace ds
}  // namespace kahypar
