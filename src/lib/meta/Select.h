/***************************************************************************
 *  Copyright (C) 2016 Sebastian Schlag <sebastian.schlag@kit.edu>
 **************************************************************************/

#pragma once

namespace meta {
template <bool flag, typename T, typename U>
struct Select {
  using Result = T;
};

template <typename T, typename U>
struct Select<false, T, U>{
  using Result = U;
};
}  // namespace meta
