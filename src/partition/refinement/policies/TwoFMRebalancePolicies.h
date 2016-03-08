/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_PARTITION_REFINEMENT_POLICIES_TWOFMREBALANCEPOLICIES_H_
#define SRC_PARTITION_REFINEMENT_POLICIES_TWOFMREBALANCEPOLICIES_H_

#include "lib/core/PolicyRegistry.h"

using core::PolicyBase;

namespace partition {
struct RebalancingPolicy : PolicyBase { };

struct GlobalRebalancing : public RebalancingPolicy {
  enum { value = true };
};

struct NoGlobalRebalancing : public RebalancingPolicy {
  enum { value = false };
};
}  // namespace partition

#endif  // SRC_PARTITION_REFINEMENT_POLICIES_TWOFMREBALANCEPOLICIES_H_
