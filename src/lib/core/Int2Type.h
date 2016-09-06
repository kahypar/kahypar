/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

#include "lib/core/PolicyRegistry.h"

namespace core {
template <int v = 0>
struct Int2Type {
  enum { value = v };
};
}  // namespace core
