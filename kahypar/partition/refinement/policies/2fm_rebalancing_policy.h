/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "kahypar/meta/policy_registry.h"
#include "kahypar/meta/typelist.h"

namespace partition {
struct RebalancingPolicy : meta::PolicyBase {
 protected:
  RebalancingPolicy() { }
};

struct GlobalRebalancing : public RebalancingPolicy,
                           private std::true_type {
  using std::true_type::operator value_type;
};

struct NoGlobalRebalancing : public RebalancingPolicy,
                             private std::false_type {
  using std::false_type::operator value_type;
};

using RebalancingPolicyClasses = meta::Typelist<GlobalRebalancing,
                                                NoGlobalRebalancing>;
}  // namespace partition
