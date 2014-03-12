/***************************************************************************
 *  Copyright (C) 2014 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_HYPEREDGEFMMOVESELECTIONPOLICIES_H_
#define SRC_PARTITION_REFINEMENT_HYPEREDGEFMMOVESELECTIONPOLICIES_H_

#include <limits>

#include "tools/RandomFunctions.h"

namespace partition {
struct EligibleTopGain {
  template <typename Gain, typename HyperedgeID, typename PartitionID, typename PrioQueues>
  static void chooseNextMove(Gain& max_gain, HyperedgeID& max_gain_he, PartitionID& from_partition,
                             PartitionID& to_partition, bool pq0_eligible, bool pq1_eligible,
                             PrioQueues& pqs) {
    Gain gain_pq0 = std::numeric_limits<Gain>::min();
    if (pq0_eligible) {
      gain_pq0 = pqs[0]->maxKey();
    }

    bool chosen_pq_index = 0;
    if (pq1_eligible && ((pqs[1]->maxKey() > gain_pq0) ||
                         (pqs[1]->maxKey() == gain_pq0 && Randomize::flipCoin()))) {
      chosen_pq_index = 1;
    }

    max_gain = pqs[chosen_pq_index]->maxKey();
    max_gain_he = pqs[chosen_pq_index]->max();
    pqs[chosen_pq_index]->deleteMax();
    from_partition = chosen_pq_index;
    to_partition = chosen_pq_index ^ 1;
  }

  protected:
  ~EligibleTopGain() { }
};
} // namespace partition
#endif  // SRC_PARTITION_REFINEMENT_HYPEREDGEFMMOVESELECTIONPOLICIES_H_
