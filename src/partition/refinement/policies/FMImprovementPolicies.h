/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_POLICIES_FMIMPROVEMENTPOLICIES_H_
#define SRC_PARTITION_REFINEMENT_POLICIES_FMIMPROVEMENTPOLICIES_H_

#include "lib/core/PolicyRegistry.h"

using core::PolicyBase;

namespace partition {
struct ImprovementPolicy : PolicyBase {
};

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

  static inline bool improvementFound(const HyperedgeWeight best_cut,
                                      const HyperedgeWeight initial_cut,
                                      const HypernodeWeight best_difference,
                                      const HypernodeWeight initial_difference,
                                      const HypernodeWeight part_weight_0,
                                      const HypernodeWeight part_weight_1,
                                      const std::array<HypernodeWeight, 2>& max_allowed_part_weights) noexcept {
    return best_cut < initial_cut ||
           ((part_weight_0 > max_allowed_part_weights[0] ||
             part_weight_1 > max_allowed_part_weights[1]) && (best_difference < initial_difference));
  }
};

}  // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_POLICIES_FMIMPROVEMENTPOLICIES_H_
