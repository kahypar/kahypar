/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_IREFINER_H_
#define SRC_PARTITION_REFINEMENT_IREFINER_H_

#include <array>
#include <string>
#include <utility>
#include <vector>

#include "lib/definitions.h"
#include "lib/macros.h"
#include "lib/utils/Stats.h"
#include "partition/Metrics.h"
#include "partition/refinement/UncontractionGainChanges.h"

using defs::Hypergraph;
using defs::HypernodeID;
using defs::PartitionID;
using defs::HypernodeWeight;
using defs::HyperedgeWeight;
using utils::Stats;
using Metrics = metrics::Metrics;

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
              Metrics& best_metrics) noexcept {
    ASSERT(_is_initialized, "initialize() has to be called before refine");
    return refineImpl(refinement_nodes, max_allowed_part_weights,
                      uncontraction_changes,
                      best_metrics);
  }

  void initialize() noexcept {
    initializeImpl();
  }

  void initialize(const HyperedgeWeight max_gain) noexcept {
    initializeImpl(max_gain);
  }

  std::string policyString() const noexcept {
    return policyStringImpl();
  }

  virtual ~IRefiner() { }

 protected:
  IRefiner() noexcept { }
  bool _is_initialized = false;

 private:
  virtual bool refineImpl(std::vector<HypernodeID>& refinement_nodes,
                          const std::array<HypernodeWeight, 2>& max_allowed_part_weights,
                          const UncontractionGainChanges& uncontraction_changes,
                          Metrics& best_metrics) noexcept = 0;
  virtual void initializeImpl() noexcept { }
  virtual void initializeImpl(const HyperedgeWeight) noexcept { }
  virtual std::string policyStringImpl() const noexcept = 0;
};
}  // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_IREFINER_H_
