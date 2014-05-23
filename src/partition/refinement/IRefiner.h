/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_IREFINER_H_
#define SRC_PARTITION_REFINEMENT_IREFINER_H_
#include <string>
#include <vector>

#include "lib/datastructure/Hypergraph.h"

using datastructure::HypergraphType;
using datastructure::HypernodeID;
using datastructure::HyperedgeWeight;

namespace partition {
class IRefiner {
  public:
  virtual void refine(std::vector<HypernodeID>& refinement_nodes, size_t num_refinement_nodes,
                      HyperedgeWeight& best_cut, double max_imbalance, double& best_imbalance) = 0;
  virtual void initialize() { }
  virtual void initialize(HyperedgeWeight max_gain) { }
  virtual int numRepetitions() = 0;
  virtual std::string policyString() const = 0;
  virtual ~IRefiner() { }
};
} //namespace partition

#endif  // SRC_PARTITION_REFINEMENT_IREFINER_H_
