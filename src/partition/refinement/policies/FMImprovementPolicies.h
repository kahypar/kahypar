/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "meta/PolicyRegistry.h"

using meta::PolicyBase;

namespace partition {
struct ImprovementPolicy : PolicyBase { };

class CutDecreasedOrInfeasibleImbalanceDecreased : public ImprovementPolicy {
 public:
  static inline bool improvementFound(const HyperedgeWeight best_cut,
                                      const HyperedgeWeight initial_cut,
                                      const double best_imbalance,
                                      const double initial_imbalance,
                                      const double max_imbalance) noexcept {
    return best_cut < initial_cut ||
           ((initial_imbalance > max_imbalance) && (best_imbalance < initial_imbalance));
  }
};
}  // namespace partition
