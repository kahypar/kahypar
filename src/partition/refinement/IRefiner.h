/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_IREFINER_H_
#define SRC_PARTITION_REFINEMENT_IREFINER_H_
#include <string>
#include <vector>

#include "lib/datastructure/Hypergraph.h"
#include "lib/macros.h"

using datastructure::HypergraphType;
using datastructure::HypernodeID;
using datastructure::HyperedgeWeight;

namespace partition {
class IRefiner {
  public:
  void refine(std::vector<HypernodeID>& refinement_nodes, size_t num_refinement_nodes,
              HyperedgeWeight& best_cut, double max_imbalance, double& best_imbalance) {
    refineImpl(refinement_nodes, num_refinement_nodes, best_cut, max_imbalance, best_imbalance);
  }

  void initialize() {
    initializeImpl();
  }

  void initialize(HyperedgeWeight max_gain) {
    initializeImpl(max_gain);
  }

  int numRepetitions() const {
    return numRepetitionsImpl();
  }

  std::string policyString() const {
    return policyStringImpl();
  }

  virtual ~IRefiner() { }

  protected:
  IRefiner() { }

  private:
  virtual void refineImpl(std::vector<HypernodeID>& refinement_nodes, size_t num_refinement_nodes,
                          HyperedgeWeight& best_cut, double max_imbalance, double& best_imbalance) = 0;
  virtual void initializeImpl() { }
  virtual void initializeImpl(HyperedgeWeight) { }
  virtual int numRepetitionsImpl() const = 0;
  virtual std::string policyStringImpl() const = 0;
  DISALLOW_COPY_AND_ASSIGN(IRefiner);
};
} //namespace partition

#endif  // SRC_PARTITION_REFINEMENT_IREFINER_H_
