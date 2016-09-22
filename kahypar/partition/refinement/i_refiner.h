/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "kahypar/definitions.h"
#include "kahypar/macros.h"
#include "kahypar/partition/metrics.h"
#include "kahypar/partition/refinement/uncontraction_gain_changes.h"
#include "kahypar/utils/stats.h"

namespace partition {
class IRefiner {
 public:
  IRefiner(const IRefiner&) = delete;
  IRefiner(IRefiner&&) = delete;
  IRefiner& operator= (const IRefiner&) = delete;
  IRefiner& operator= (IRefiner&&) = delete;

  bool refine(std::vector<HypernodeID>& refinement_nodes,
              const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
              const UncontractionGainChanges& uncontraction_changes,
              Metrics& best_metrics) {
    ASSERT(_is_initialized, "initialize() has to be called before refine");
    return refineImpl(refinement_nodes, max_allowed_part_weights,
                      uncontraction_changes,
                      best_metrics);
  }

  void initialize() {
    initializeImpl();
  }

  void initialize(const HyperedgeWeight max_gain) {
    initializeImpl(max_gain);
  }

  std::string policyString() const {
    return policyStringImpl();
  }

  virtual ~IRefiner() { }

 protected:
  IRefiner() { }
  bool _is_initialized = false;

 private:
  virtual bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                          const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                          const UncontractionGainChanges& uncontraction_changes,
                          Metrics& best_metrics) = 0;
  virtual void initializeImpl() { }
  virtual void initializeImpl(const HyperedgeWeight) { }
  virtual std::string policyStringImpl() const = 0;
};
}  // namespace partition
