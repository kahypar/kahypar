/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_IREFINER_H_
#define SRC_PARTITION_REFINEMENT_IREFINER_H_
#include <string>
#include <vector>

#include "lib/definitions.h"
#include "lib/macros.h"
#include "lib/utils/Stats.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using utils::Stats;

namespace partition {
class IRefiner {
  public:
  bool refine(std::vector<HypernodeID>& refinement_nodes, const size_t num_refinement_nodes,
              const HypernodeWeight max_allowed_part_weight, HyperedgeWeight& best_cut,
              double& best_imbalance) {
    return refineImpl(refinement_nodes, num_refinement_nodes, max_allowed_part_weight,
                      best_cut, best_imbalance);
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

  const Stats & stats() const {
    return statsImpl();
  }

  virtual ~IRefiner() { }

  protected:
  IRefiner() { }

  private:
  virtual bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                          const size_t num_refinement_nodes,
                          const HypernodeWeight max_allowed_part_weight,
                          HyperedgeWeight& best_cut,
                          double& best_imbalance) = 0;
  virtual void initializeImpl() { }
  virtual void initializeImpl(HyperedgeWeight) { }
  virtual int numRepetitionsImpl() const = 0;
  virtual std::string policyStringImpl() const = 0;
  virtual const Stats & statsImpl() const = 0;
  DISALLOW_COPY_AND_ASSIGN(IRefiner);
};
} //namespace partition

#endif  // SRC_PARTITION_REFINEMENT_IREFINER_H_
