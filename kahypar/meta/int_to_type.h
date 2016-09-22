/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "kahypar/meta/policy_registry.h"

namespace meta {
template <int v = 0>
struct Int2Type {
  enum { value = v };
};
}  // namespace meta
