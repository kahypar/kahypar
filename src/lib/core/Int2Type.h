/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#ifndef SRC_LIB_CORE_INT2TYPE_H_
#define SRC_LIB_CORE_INT2TYPE_H_

#include "lib/core/PolicyRegistry.h"

namespace core {
template <int v = 0>
struct Int2Type {
  enum { value = v };
};
}  // namespace core

#endif  // SRC_LIB_CORE_INT2TYPE_H_
