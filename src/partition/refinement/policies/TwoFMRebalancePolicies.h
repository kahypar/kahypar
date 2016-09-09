/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "lib/meta/PolicyRegistry.h"

using meta::PolicyBase;

namespace partition {
struct RebalancingPolicy : PolicyBase {
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
}  // namespace partition
