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
  IRefiner(const IRefiner&) = delete;
  IRefiner(IRefiner&&) = delete;
  IRefiner& operator = (const IRefiner&) = delete;
  IRefiner& operator = (IRefiner&&) = delete;

  bool refine(std::vector<HypernodeID>& refinement_nodes, const size_t num_refinement_nodes,
              const HypernodeWeight max_allowed_part_weight, HyperedgeWeight& best_cut,
              double& best_imbalance) noexcept {
    ASSERT(_is_initialized, "initialize() has to be called before refine");
    return refineImpl(refinement_nodes, num_refinement_nodes, max_allowed_part_weight,
                      best_cut, best_imbalance);
  }

  void initialize() noexcept {
    initializeImpl();
  }

  void initialize(const HyperedgeWeight max_gain) noexcept {
    initializeImpl(max_gain);
  }

  int numRepetitions() const noexcept {
    return numRepetitionsImpl();
  }

  std::string policyString() const noexcept {
    return policyStringImpl();
  }

  const Stats & stats() const noexcept {
    return statsImpl();
  }

  virtual ~IRefiner() { }

 protected:
  IRefiner() noexcept { }
  bool _is_initialized = false;

 private:
  virtual bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                          const size_t num_refinement_nodes,
                          const HypernodeWeight max_allowed_part_weight,
                          HyperedgeWeight& best_cut,
                          double& best_imbalance) noexcept = 0;
  virtual void initializeImpl() noexcept { }
  virtual void initializeImpl(const HyperedgeWeight) noexcept { }
  virtual int numRepetitionsImpl() const noexcept = 0;
  virtual std::string policyStringImpl() const noexcept = 0;
  virtual const Stats & statsImpl() const noexcept = 0;
};
} //namespace partition

#endif  // SRC_PARTITION_REFINEMENT_IREFINER_H_
