/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_POLICIES_FMQUEUESELECTIONPOLICIES_H_
#define SRC_PARTITION_REFINEMENT_POLICIES_FMQUEUESELECTIONPOLICIES_H_

#include <limits>

#include "lib/core/Mandatory.h"
#include "lib/core/PolicyRegistry.h"
#include "tools/RandomFunctions.h"

namespace partition {
template <typename Gain = Mandatory>
struct EligibleTopGain {
  template <typename PrioQueue>
  static bool selectQueue(bool pq0_eligible, bool pq1_eligible,
                          const PrioQueue& pq0, const PrioQueue& pq1) {
    Gain gain_pq0 = std::numeric_limits<Gain>::min();
    if (pq0_eligible) {
      gain_pq0 = pq0->maxKey();
    }
    bool chosen_pq_index = 0;
    if (pq1_eligible && ((pq1->maxKey() > gain_pq0) ||
                         (pq1->maxKey() == gain_pq0 && Randomize::flipCoin()))) {
      chosen_pq_index = 1;
    }
    return chosen_pq_index;
  }

  protected:
  ~EligibleTopGain() { }
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_POLICIES_FMQUEUESELECTIONPOLICIES_H_
